// BarcelonaReverbera - A Non-Uniform Partitioned Convolution Reverb VST3 Plugin
// Copyright (C) 2024 sbrk devices
//
// This file is part of BarcelonaReverbera.
//
// BarcelonaReverbera is free software: you can use it and/or modify it for
// educational and non-commercial purposes only under the terms of the
// Custom Non-Commercial License.
//
// You should have received a copy of the Custom Non-Commercial License
// along with this program. If not, see https://github.com/SbrkDevices/BarcelonaReverbera.
//
// For more information, please contact dani@sbrkdevices.com.

///////////////////////////////////////////////////////////////////////////////

#include "ConvolutionReverb.h"

///////////////////////////////////////////////////////////////////////////////

ConvolutionReverb::ConvolutionReverb(void) : m_thread(juce::String("IrUpdater"), [this] () { }, [this] () { }, [this] () { updateIr(); }) {}

void ConvolutionReverb::init(void)
{
	for (int i=0; i<BCNRVRB_PARAM_INTERPOL_ARRAY_LEN; i++)
	{
		const double valLin = double(i) / double(BCNRVRB_PARAM_INTERPOL_ARRAY_LEN - 1);
	
		m_arrayVolumeInterp[i] = DspUtils::dB2Lin(getParamVolumeControltodB(valLin), BCNRVRB_MIN_DB);

		m_arrayFilterLpfFcInterp[i] = getParamFilterControlToLpfFc(valLin);
		m_arrayFilterHpfFcInterp[i] = getParamFilterControlToHpfFc(valLin);

		m_arrayDecayInterp[i] = BCNRVRB_DECAY_MIN + getLogTen0to1FromLin0to1(valLin, BCNRVRB_DECAY_KNOB_DECADES) * (1.0f - BCNRVRB_DECAY_MIN);
	}

	for (int ch=0; ch<2; ch++)
	{
		m_filterLPF[ch].init(true);
		m_filterHPF[ch].init(false);
	}
}

void ConvolutionReverb::exit(void)
{
	m_convolutionEngine.exit();

	for (int ch=0; ch<2; ch++)
	{
		m_filterLPF[ch].exit();
		m_filterHPF[ch].exit();
	}

	if (m_thread.isThreadRunning())
		DEBUG_VERIFY(m_thread.stopThread(2000));
}

///////////////////////////////////////////////////////////////////////////////

void ConvolutionReverb::process(const float* __restrict audioIn[2], float* __restrict audioOut[2], bool stereo, double samplerate, int blockSize, float decayControl, float colorControl, float dryWetControl, int irIndex)
{
	const uint32_t numChannels = stereo ? 2 : 1;

	DEBUG_ASSERT(irIndex < getIrCount());
	DEBUG_ASSERT(blockSize >= BCNRVRB_MIN_BLOCK_SIZE);
	DEBUG_ASSERT(blockSize <= BCNRVRB_MAX_BLOCK_SIZE);
	DEBUG_ASSERT(samplerate <= BCNRVRB_MAX_SAMPLERATE);

	// check for unsupported conditions:
	if ((blockSize < BCNRVRB_MIN_BLOCK_SIZE) || (blockSize > BCNRVRB_MAX_BLOCK_SIZE) || (!DspUtils::isPowOf2(blockSize)) || (samplerate > BCNRVRB_MAX_SAMPLERATE))
	{
		for (uint32_t ch=0; ch<numChannels; ch++)
			std::memcpy(audioOut[ch], audioIn[ch], blockSize*sizeof(float));
		
		return;
	}

	const bool paramChanges =
		((m_irIndex != irIndex) || (m_samplerate != samplerate) || (m_blockSize != blockSize) || (m_numChannels != numChannels));
	
	m_irIndex = irIndex;
	m_samplerate = samplerate;
	m_blockSize = blockSize;
	m_numChannels = numChannels;

	if (paramChanges)
		reconfigure();

	const float dryTarget = getVolumeFromControl((dryWetControl < 0.0f) ? 1.0f : (1.0f - dryWetControl));
	const float wetTarget = getVolumeFromControl((dryWetControl > 0.0f) ? 1.0f : (1.0f + dryWetControl));

	const uint32_t dryWetRecalculateTimesPerBlock = m_dryWetRecalculateTimesPerBlock;
	const float dryWetSmoothingFactor = m_dryWetSmoothingFactor;
	const uint32_t dryWetSamplesBetweenRecalculate = m_dryWetSamplesBetweenRecalculate;
	float dryCurrent = m_dryCurrent;
	float wetCurrent = m_wetCurrent;
	float dry, dryInc;
	float wet, wetInc;

	for (uint32_t i=0; i<dryWetRecalculateTimesPerBlock; i++)
	{
		DspUtils::smoothParameter(dryTarget, dryCurrent, dry, dryInc, dryWetSmoothingFactor, dryWetSamplesBetweenRecalculate);
		DspUtils::smoothParameter(wetTarget, wetCurrent, wet, wetInc, dryWetSmoothingFactor, dryWetSamplesBetweenRecalculate);

		for (uint32_t j=0; j<dryWetSamplesBetweenRecalculate; j++)
		{
			const uint32_t index = i * dryWetSamplesBetweenRecalculate + j;

			for (uint8_t ch=0; ch<numChannels; ch++)
			{
				m_audioDry[ch][index] = audioIn[ch][index]*dry;
				m_audioReverbIn[ch][index] = audioIn[ch][index]*wet;
			}

			dry += dryInc;
			wet += wetInc;
		}
	}

	if (m_convolutionEngine.canUpdateIr() && !m_updatingIr) // will be true every BCNRVRB_LONGEST_STAGE_SIZE samples
	{
		m_convolutionEngine.updateIr(m_irUpdateIndex);	

		m_irUpdateIndex = (m_irUpdateIndex == 0) ? 1 : 0;

		m_decayControl = decayControl;
		m_colorControl = colorControl;

		m_updatingIr = true;
		m_thread.notify();
	}

	const float* audioReverbIn[2] = { m_audioReverbIn[0], m_audioReverbIn[1] };

	m_convolutionEngine.process(audioReverbIn, audioOut);

	for (uint32_t i=0; i<blockSize; i++)
	{
		for (uint8_t ch=0; ch<2; ch++)
			audioOut[ch][i] += m_audioDry[ch][i];
	}

	m_dryCurrent = dryCurrent;
	m_wetCurrent = wetCurrent;
}

///////////////////////////////////////////////////////////////////////////////

void ConvolutionReverb::reconfigure(void)
{
	if (m_thread.isThreadRunning())
		DEBUG_VERIFY(m_thread.stopThread(2000));

# if JUCE_MAC
#  if 0 // XXX does this work on Apple Silicon? Does not work on Intel...
	m_thread.startRealtimeThread(juce::Thread::RealtimeOptions{}.withMaximumProcessingTimeMs(BCNRVRB_LONGEST_STAGE_SIZE*1000.0/m_samplerate));
#  else
	m_thread.startRealtimeThread(juce::Thread::RealtimeOptions{}.withPriority(8));
#  endif
# else
	//m_thread.startRealtimeThread(juce::Thread::RealtimeOptions{}.withPriority(8)); // XXX not working on linux?
	m_thread.startThread(juce::Thread::Priority::high);
# endif

	m_dryWetRecalculateTimesPerBlock = m_blockSize / m_dryWetSamplesBetweenRecalculate;
	m_dryWetSmoothingFactor = DspUtils::getTimeConstantMs(BCNRVRB_DRYWET_SMOOTH_LEN_MS, float(m_samplerate/m_dryWetSamplesBetweenRecalculate));
	m_colorAndDecaySmoothingFactor = DspUtils::getTimeConstantMs(BCNRVRB_DECAY_COLOR_SMOOTH_LEN_MS, float(m_samplerate/float(BCNRVRB_LONGEST_STAGE_SIZE)));

	m_convolutionEngine.exit();

	float* irPreProcessed[2] = { m_irPreProcessed[0], m_irPreProcessed[1] };
	float* irPostProcessed0[2] = { m_irPostProcessed[0][0], m_irPostProcessed[0][1] };
	float* irPostProcessed1[2] = { m_irPostProcessed[1][0], m_irPostProcessed[1][1] };

	for (int ch=0; ch<2; ch++)
	{
		std::memset(irPreProcessed[ch], 0, BCNRVRB_IR_MAX_LEN_SAMPLES*sizeof(float));
		std::memset(irPostProcessed0[ch], 0, BCNRVRB_IR_MAX_LEN_SAMPLES*sizeof(float));
		std::memset(irPostProcessed1[ch], 0, BCNRVRB_IR_MAX_LEN_SAMPLES*sizeof(float));
	}

	m_irLen = m_impulseResponses.getIrLen(m_irIndex);
	uint32_t irLenWithZeros = m_impulseResponses.getIrLenWithZeros(m_irIndex);
	const uint32_t numExtraZeros = irLenWithZeros - m_irLen;

	if (m_samplerate == BCNRVRB_DEFAULT_IR_SAMPLERATE)
	{
		DEBUG_ASSERT(irLenWithZeros <= BCNRVRB_IR_MAX_LEN_SAMPLES);

		for (int ch=0; ch<2; ch++)
			memcpy(irPreProcessed[ch], m_impulseResponses.getIrAudioBuffer(m_irIndex, ch), irLenWithZeros*sizeof(float));
	}
	else
	{
		const float* irAudioIn[2] = { m_impulseResponses.getIrAudioBuffer(m_irIndex, 0), m_impulseResponses.getIrAudioBuffer(m_irIndex, 1) };

		SamplerateConverter::convert(BCNRVRB_DEFAULT_IR_SAMPLERATE, m_samplerate, irAudioIn, irPreProcessed, m_impulseResponses.getIrLen(m_irIndex), BCNRVRB_IR_MAX_LEN_SAMPLES, m_irLen);

		if (m_irLen < BCNRVRB_IR_MIN_LEN_SAMPLES)
			m_irLen = BCNRVRB_IR_MIN_LEN_SAMPLES;

		irLenWithZeros = m_irLen + numExtraZeros;

		if (irLenWithZeros > BCNRVRB_IR_MAX_LEN_SAMPLES)
			irLenWithZeros = BCNRVRB_IR_MAX_LEN_SAMPLES;
	}

	{ // IR normalization (post-size, pre-color): (what matters is the IR level and its length)
		double sumSquares = 0.0f;

		for (int ch=0; ch<m_numChannels; ch++)
		{
			for (uint32_t i=0; i<m_irLen; i++)
			{
				const float sample = irPreProcessed[ch][i];
				sumSquares += sample*sample;
			}
		}

		sumSquares /= static_cast<double>(m_numChannels);

		if (sumSquares > 1e-7f)
		{
			const float normalizationFactor = 0.65f / std::sqrt(sumSquares); // set experimentally. JUCE uses 0.125f

			for (int ch=0; ch<m_numChannels; ch++)
			{
				for (uint32_t i=0; i<m_irLen; i++)
					irPreProcessed[ch][i] *= normalizationFactor;
			}
		}
	}

	m_convolutionEngine.init(m_samplerate, m_blockSize, m_numChannels, irPostProcessed0, irPostProcessed1, m_irLen, irLenWithZeros);

	for (int ch=0; ch<2; ch++)
	{
		m_filterLPF[ch].reset();
		m_filterHPF[ch].reset();
	}
}

///////////////////////////////////////////////////////////////////////////////

void ConvolutionReverb::updateIr(void)
{
	const uint8_t numChannels = m_numChannels;
	const uint32_t irLen = m_irLen;

	float* irPostProcessed[2] = { m_irPostProcessed[m_irUpdateIndex][0], m_irPostProcessed[m_irUpdateIndex][1] };

# if 0 // temporary: no processing

	for (int ch=0; ch<numChannels; ch++)
	{
		for (uint32_t i=0; i<irLen; i++)
			irPostProcessed[ch][i] = m_irPreProcessed[ch][i];
	}

# else

	{ // decay processing:
		const float decayTarget = getDecayFromDecayControl(m_decayControl);

		m_decayCurrent = DspUtils::expSmoothing(decayTarget, m_decayCurrent, m_colorAndDecaySmoothingFactor);

		const uint32_t decayCutPointSamples = irLen*m_decayCurrent;

		const float decayEnvSmoothingTimeSamplesMax = 1.5f * m_samplerate; // 1.5 seconds
		const float decayEnvSmoothingTimeSamples = juce::jmin(decayCutPointSamples * BCNRVRB_DECAY_ENVELOPE_PERCENTAGE, decayEnvSmoothingTimeSamplesMax);
		const float decayEnvSmoothingFactor = DspUtils::getTimeConstantSamples(decayEnvSmoothingTimeSamples);

		float decayGainCurrent = 1.0f;
	
		for (uint32_t i=0; i<irLen; i++)
		{
			const float decayGainTarget = (i < decayCutPointSamples) ? 1.0f : 0.0f;
			decayGainCurrent = DspUtils::expSmoothing(decayGainTarget, decayGainCurrent, decayEnvSmoothingFactor);

			for (int ch=0; ch<numChannels; ch++)
				irPostProcessed[ch][i] = m_irPreProcessed[ch][i] * decayGainCurrent;
		}
	}

	{ // color processing:
		const bool filterIsLowPass = (m_colorControl <= 0.0f);
		const float filterFc = filterIsLowPass
			? getFilterLpfFcFromControl(1.0f + m_colorControl)
			: getFilterHpfFcFromControl(m_colorControl);

		const float filterLpfCutoff = filterIsLowPass ? filterFc : 20000.0f;
		const float filterHpfCutoff = filterIsLowPass ? 20.0f : filterFc;

		for (int ch=0; ch<numChannels; ch++)
		{
			m_filterLPF[ch].clearState();
			m_filterLPF[ch].setTargetFreq(filterLpfCutoff, m_colorAndDecaySmoothingFactor, m_samplerate);
			m_filterLPF[ch].process(irPostProcessed[ch], irPostProcessed[ch], irLen);

			m_filterHPF[ch].clearState();
			m_filterHPF[ch].setTargetFreq(filterHpfCutoff, m_colorAndDecaySmoothingFactor, m_samplerate);
			m_filterHPF[ch].process(irPostProcessed[ch], irPostProcessed[ch], irLen);
		}
	}

# endif

	m_updatingIr = false;
}

///////////////////////////////////////////////////////////////////////////////
