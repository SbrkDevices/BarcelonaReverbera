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

#pragma once

#include "ConvolutionEngine.h"
#include "SamplerateConverter.h"
#include "FilterBiquad.h"
#include "IrBuffersAutoGenerated.h"

///////////////////////////////////////////////////////////////////////////////

class ConvolutionReverb
{
public:
	ConvolutionReverb(void);

	void init(void);
	void exit(void);

    void process(const float* audioIn[2] __restrict, float* audioOut[2] __restrict, bool stereo, double samplerate, int blockSize, float decayControl, float colorControl, float dryWetControl, int irIndex);

private:
	void reconfigure(void);
	void updateIr(void);

public:
	static constexpr int getIrCount(void)
	{
		return IrBuffers::getIrCount();
	}

	inline char* getIrName(int irIndex)
	{
		return m_impulseResponses.getIrName(irIndex);
	}

private:
	inline float getParamVolumeControltodB(float volumeControl)
	{
		return (volumeControl > 0.000001) ? 60 * log10(volumeControl) : BCNRVRB_MIN_DB;
	}

	inline float getParamFilterControlToLpfFc(float filterFcControl)
	{
		return std::exp((filterFcControl) * BCNRVRB_COLOR_LPF_FREQ_RANGE + BCNRVRB_COLOR_LPF_FREQ_LOGMIN);
	}
	inline float getParamFilterControlToHpfFc(float filterFcControl)
	{
		return std::exp((filterFcControl) * BCNRVRB_COLOR_HPF_FREQ_RANGE + BCNRVRB_COLOR_HPF_FREQ_LOGMIN);
	}

	inline float getParamArrayValueInterpolated(float posLinear, float* array)
	{
		DEBUG_ASSERT((posLinear >= 0.0f) && (posLinear <= 1.0f));

		const float index = posLinear * (BCNRVRB_PARAM_INTERPOL_ARRAY_LEN - 1);
		const uint32_t indexInt = uint32_t(index);
		const float mu = index - indexInt;

		const float y0 = array[indexInt];
		const float y1 = (indexInt == (BCNRVRB_PARAM_INTERPOL_ARRAY_LEN - 1)) ? 1.0f : array[indexInt + 1];

		return DspUtils::linearInterpolate(y0, y1, mu);
	}

	inline float getLogTen0to1FromLin0to1(float lin0to1, double decades)
	{
		DEBUG_ASSERT((lin0to1 >= 0.0f) && (lin0to1 <= 1.0f));

		return float((std::pow(10, decades * double(lin0to1)) - 1.0) / (std::pow(10, decades) - 1.0));
	}

	inline float getVolumeFromControl(float volumeControl)
	{
		DEBUG_ASSERT((volumeControl >= 0.0f) && (volumeControl <= 1.0f));

		return getParamArrayValueInterpolated(volumeControl, m_arrayVolumeInterp);
	}

	inline float getFilterLpfFcFromControl(float filterFcControl)
	{
		DEBUG_ASSERT((filterFcControl >= 0.0f) && (filterFcControl <= 1.0f));

		return getParamArrayValueInterpolated(filterFcControl, m_arrayFilterLpfFcInterp);
	}

	inline float getFilterHpfFcFromControl(float filterFcControl)
	{
		DEBUG_ASSERT((filterFcControl >= 0.0f) && (filterFcControl <= 1.0f));

		return getParamArrayValueInterpolated(filterFcControl, m_arrayFilterHpfFcInterp);
	}

	inline float getDecayFromDecayControl(float decayControl)
	{
		DEBUG_ASSERT((decayControl >= 0.0f) && (decayControl <= 1.0f));

		return getParamArrayValueInterpolated(decayControl, m_arrayDecayInterp);
	}

private:
	uint32_t m_blockSize = 16;
	float m_samplerate = BCNRVRB_DEFAULT_IR_SAMPLERATE;
	uint8_t m_numChannels = 2;

	int m_irIndex = -1;

	IrBuffers m_impulseResponses;
	ConvolutionEngine m_convolutionEngine;

    alignas(16) float m_audioDry[2][BCNRVRB_MAX_BLOCK_SIZE] = {};
    alignas(16) float m_audioReverbIn[2][BCNRVRB_MAX_BLOCK_SIZE] = {};

	uint32_t m_irLen = 0;
	alignas(16) float m_irPreProcessed[2][BCNRVRB_IR_MAX_LEN_SAMPLES] = {}; // 1 stereo buffer
	alignas(16) float m_irPostProcessed[2][2][BCNRVRB_IR_MAX_LEN_SAMPLES] = {}; // 2 stereo buffers
	std::atomic<uint8_t> m_irUpdateIndex = 0; // indicates which IR buffer is currently being updated
	static_assert(std::atomic<uint8_t>::is_always_lock_free);

	float m_dryCurrent = 0.0f;
	float m_wetCurrent = 0.0f;
	float m_arrayVolumeInterp[BCNRVRB_PARAM_INTERPOL_ARRAY_LEN] = {};
	uint32_t m_dryWetSamplesBetweenRecalculate = BCNRVRB_MIN_BLOCK_SIZE;
	uint32_t m_dryWetRecalculateTimesPerBlock = 1;
	float m_dryWetSmoothingFactor = 0.0f;

	std::atomic<float> m_colorControl = 0.0f;
	FilterBiquad m_filterLPF[2], m_filterHPF[2];
	float m_arrayFilterLpfFcInterp[BCNRVRB_PARAM_INTERPOL_ARRAY_LEN] = {};
	float m_arrayFilterHpfFcInterp[BCNRVRB_PARAM_INTERPOL_ARRAY_LEN] = {};

	std::atomic<float> m_decayControl = 0.0f;
	float m_arrayDecayInterp[BCNRVRB_PARAM_INTERPOL_ARRAY_LEN] = {};
	float m_decayCurrent = 1.0f;

	static_assert(std::atomic<float>::is_always_lock_free);

	float m_colorAndDecaySmoothingFactor = 0.0f;

	DspThread m_thread;

	std::atomic<bool> m_updatingIr = false;
	static_assert(std::atomic<bool>::is_always_lock_free);
};

///////////////////////////////////////////////////////////////////////////////