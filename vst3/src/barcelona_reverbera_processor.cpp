#include <cassert>

#include "barcelona_reverbera_processor.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

//////////////////////////////////////////////////////////////////////////////

using namespace Steinberg;

//////////////////////////////////////////////////////////////////////////////

BarcelonaReverberaProcessor::BarcelonaReverberaProcessor()
{
	setControllerClass(kBarcelonaReverberaControllerUID);

	m_impulseResponses.init();

	memset(m_samplerateConvertedIr, 0, sizeof(m_samplerateConvertedIr));
}

BarcelonaReverberaProcessor::~BarcelonaReverberaProcessor()
{
}

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaProcessor::initialize(FUnknown* context)
{
	tresult result = AudioEffect::initialize(context);
	if (result != kResultOk)
		return result;

	//--- create Audio IO ------
	addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	/* If you don't need an event bus, you can remove the next line */
	addEventInput(STR16("Event In"), 1);

	m_convolutionReverb.init(16); // XXX can we know block size already?
	
	return kResultOk;
}

tresult PLUGIN_API BarcelonaReverberaProcessor::terminate()
{
	m_convolutionReverb.exit();

	return AudioEffect::terminate();
}

tresult PLUGIN_API BarcelonaReverberaProcessor::setActive(TBool state)
{
	return AudioEffect::setActive(state);
}

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaProcessor::process(Vst::ProcessData& data)
{
    if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();

        for (int32 index=0; index<numParamsChanged; index++)
        {
            Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(index);
            
			if (paramQueue)
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();

                switch (paramQueue->getParameterId())
                {
                    case BarcelonaReverberaParams::kParamSizeId:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                            paramSize = value;
                        break;

                    case BarcelonaReverberaParams::kParamColorId:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                            paramColor = value;
                        break;

                    case BarcelonaReverberaParams::kParamDryWetId:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                            paramDryWet = value;
                        break;
					
					case BarcelonaReverberaParams::kParamSelectedIR:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
							paramSelectedIR = std::min<int>(IMPULSE_RESPONSES_COUNT*value, IMPULSE_RESPONSES_COUNT - 1);
						break;

					case BarcelonaReverberaParams::kBypassId:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                            paramBypass = (value > 0.5f);
                        break;
                }
            }
        }
    }

    if (data.numInputs == 0 || data.numOutputs == 0)
        return kResultOk; // nothing to do

	const int32 numChannels = data.inputs[0].numChannels;
	const int32 blockSizeSamples = data.numSamples;
	const int32 samplerate = (data.processContext != nullptr) ? int32(data.processContext->sampleRate) : -1; // XXX how to get samplerate if processContext is nullptr?

	bool samplerateChange = false;
	bool blockSizeChange = false;
	bool selectedIrChange = false;

	if (paramSelectedIR != m_irIndex)
		selectedIrChange = true;
	if (blockSizeSamples != m_blockSize)
		blockSizeChange = true;
	if (samplerate != m_samplerate)
		samplerateChange = true;
	
	m_irIndex = paramSelectedIR;
	m_blockSize = blockSizeSamples;
	m_samplerate = samplerate;

	if ((m_blockSize == 0) || (numChannels < 1))
		return kResultOk;

	assert(m_blockSize >= BARCELONA_REVERBERA_MIN_BLOCK_SIZE);
	assert(m_blockSize <= BARCELONA_REVERBERA_MAX_BLOCK_SIZE);

	if (blockSizeChange)
		m_convolutionReverb.init(m_blockSize);

	if (samplerateChange)
	{
		if (m_samplerate != DEFAULT_IR_SAMPLERATE)
		{
			if ((m_samplerate < MIN_SAMPLERATE) || (m_samplerate > MAX_SAMPLERATE))
				m_samplerate = -1;
			else
			{
				float* irAudioIn[2] = { m_impulseResponses.getIrPtr(m_irIndex, 0), m_impulseResponses.getIrPtr(m_irIndex, 1) };
				float* irAudioOut[2] = { m_samplerateConvertedIr[0], m_samplerateConvertedIr[1] };

				SamplerateConverter::convert(DEFAULT_IR_SAMPLERATE, m_samplerate, irAudioIn, irAudioOut, m_impulseResponses.getIrLen(m_irIndex), BARCELONA_REVERBERA_IR_MAX_LEN_SAMPLES, m_samplerateConvertedIrLength);
			}
		}

		selectedIrChange = true;
	}

	if (selectedIrChange && (m_samplerate > 0))
	{
		if (m_samplerate == DEFAULT_IR_SAMPLERATE)
		{
			const uint32_t irLenWithoutZeros = m_impulseResponses.getIrLen(m_irIndex);
			const uint32_t irLenWithZeros = m_impulseResponses.getIrLenWithZeros(m_irIndex);

			m_irLen = irLenWithoutZeros + m_blockSize - (irLenWithoutZeros % m_blockSize);
			assert(m_irLen <= irLenWithZeros);
			assert(double(m_irLen)/double(m_blockSize) == double(m_irLen/m_blockSize));

			m_convolutionReverb.setupIR(m_impulseResponses.getIrPtr(m_irIndex, 0), m_impulseResponses.getIrPtr(m_irIndex, 1), m_irLen);
		}
		else
		{
			const uint32_t irLenBlockAligned = m_samplerateConvertedIrLength + m_blockSize - (m_samplerateConvertedIrLength % m_blockSize);
			assert(irLenBlockAligned <= BARCELONA_REVERBERA_IR_MAX_LEN_SAMPLES);
			m_convolutionReverb.setupIR(m_samplerateConvertedIr[0], m_samplerateConvertedIr[1], irLenBlockAligned);
		}
	}

	Vst::Sample32** inChannels = data.inputs[0].channelBuffers32;
	Vst::Sample32** outChannels = data.outputs[0].channelBuffers32;

	bool silenceFlag = (data.inputs[0].silenceFlags != 0);

	if (silenceFlag && !m_silenceFlag)
		m_silenceFlagCounter = 0;

	m_silenceFlag = silenceFlag;

	if (m_silenceFlag || (m_samplerate < 0))
	{
		if (m_silenceFlagCounter < m_irLen)
			m_silenceFlagCounter += m_blockSize;
		else
		{
		    data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

			for (int ch=0; ch<numChannels; ch++)
				memset(outChannels[ch], 0, m_blockSize*sizeof(float));
			
			return kResultOk;
		}
	}

	data.outputs[0].silenceFlags = 0;

	assert((paramDryWet >= 0.0f) && (paramDryWet <= 1.0f));
	const float dry = (paramDryWet < 0.4) ? 1.0f : (1.0f - paramDryWet)/0.6f;
	const float wet = (paramDryWet > 0.6) ? 1.0f : paramDryWet/0.6f;

	const uint32_t convolutionChannels = (numChannels > 2) ? 2 : numChannels;
	m_convolutionReverb.process(inChannels, outChannels, convolutionChannels);

	for (int ch=0; ch<numChannels; ch++)
	{	
		Vst::Sample32* in = inChannels[ch];
		Vst::Sample32* out = outChannels[ch];

		if (ch >= 2)
		{
			for (int i=0; i<m_blockSize; i++)
				out[i] = in[i];
		
			continue;
		}

		for (uint32_t i=0; i<m_blockSize; i++)
			out[i] = dry*in[i] + wet*out[i]*0.1f; // XXX for now, using constant gain of 0.1
	}

    return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API BarcelonaReverberaProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////

tresult PLUGIN_API BarcelonaReverberaProcessor::setState(IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	float savedParamSize = 0.f;
	if (streamer.readFloat(savedParamSize) == false)
		return kResultFalse;

	float savedParamColor = 0.f;
	if (streamer.readFloat(savedParamColor) == false)
		return kResultFalse;

	float savedParamDryWet = 0.f;
	if (streamer.readFloat(savedParamDryWet) == false)
		return kResultFalse;
	
	int32 savedParamSelectedIR = 0;
	if (streamer.readInt32(savedParamSelectedIR) == false)
		return kResultFalse;

	int32 savedBypass = 0;
	if (streamer.readInt32(savedBypass) == false)
		return kResultFalse;

	paramSize = savedParamSize;
	paramColor = savedParamColor;
	paramDryWet = savedParamDryWet;
	paramSelectedIR = savedParamSelectedIR;
	paramBypass = savedBypass > 0;

	return kResultOk;
}

tresult PLUGIN_API BarcelonaReverberaProcessor::getState(IBStream* state)
{
	float toSaveParamSize = paramSize;
	float toSaveParamColor = paramColor;
	float toSaveParamDryWet = paramDryWet;
	int32 toSaveParamSelectedIR = paramSelectedIR;
	int32 toSaveBypass = paramBypass ? 1 : 0;

	IBStreamer streamer(state, kLittleEndian);
	streamer.writeFloat(toSaveParamSize);
	streamer.writeFloat(toSaveParamColor);
	streamer.writeFloat(toSaveParamDryWet);
	streamer.writeInt32(toSaveParamSelectedIR);
	streamer.writeInt32(toSaveBypass);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////