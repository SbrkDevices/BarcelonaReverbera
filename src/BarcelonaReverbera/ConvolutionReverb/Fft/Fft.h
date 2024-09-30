#pragma once

#include "ConvolutionReverbCommon.h"
#include "pffft.h"

///////////////////////////////////////////////////////////////////////////////

typedef float cplx_f32[2];

///////////////////////////////////////////////////////////////////////////////

template<bool _isForward, bool _freqDataOrdered>
class Fft
{
private:
	static constexpr uint32_t m_isForward = _isForward;
	static constexpr uint32_t m_freqDataOrdered = _freqDataOrdered;

	static bool m_allInited;

	uint32_t m_fftSize = 0;

	PFFFT_Setup* m_setup = nullptr; // FFT/IFFT setup

	float* m_workData = nullptr; // internal buffer for PFFFT work. Size: m_fftSize

public:
	void init(uint32_t fftSize, float* workData)
	{
		m_fftSize = fftSize;

		DEBUG_ASSERT(m_setup == nullptr);
		DEBUG_ASSERT(m_workData == nullptr);

		m_workData = workData;
		DEBUG_ASSERT(m_workData != nullptr);

		m_setup = pffft_new_setup(fftSize, PFFFT_REAL);
		DEBUG_ASSERT(m_setup != nullptr);
	}

	void exit(void)
	{
		if (m_setup == nullptr)
			return;

		pffft_destroy_setup(m_setup);

		m_setup = nullptr;
		m_workData = nullptr;
	}

	inline void process(float* audioData, cplx_f32* freqBinsData)
	{
		DEBUG_ASSERT(m_setup != nullptr);

		if (m_freqDataOrdered)
		{
			if (m_isForward)
				pffft_transform_ordered(m_setup, audioData, (float *) freqBinsData, m_workData, PFFFT_FORWARD);
			else
				pffft_transform_ordered(m_setup, (float *) freqBinsData, audioData, m_workData, PFFFT_BACKWARD);
		}
		else
		{
			if (m_isForward)
				pffft_transform(m_setup, audioData, (float *) freqBinsData, m_workData, PFFFT_FORWARD);
			else
				pffft_transform(m_setup, (float *) freqBinsData, audioData, m_workData, PFFFT_BACKWARD);
		}
	}

	// freqBinsAccum += (freqBinsDataA * freqBinsDataB) / m_fftSize
	// All arrays are unordered freq bins data
	inline void convolve_accum(cplx_f32* freqBinsAccum, cplx_f32* freqBinsDataA, cplx_f32* freqBinsDataB)
	{
		if (m_freqDataOrdered)
		{
			const uint32_t fftSizeFreqDomain = m_fftSize/2 + 1;
			for (uint32_t i=0; i<fftSizeFreqDomain; i++)
			{
				freqBinsAccum[i][0] += (freqBinsDataA[i][0] * freqBinsDataB[i][0] - freqBinsDataA[i][1] * freqBinsDataB[i][1]) / m_fftSize;
				freqBinsAccum[i][1] += (freqBinsDataA[i][0] * freqBinsDataB[i][1] + freqBinsDataA[i][1] * freqBinsDataB[i][0]) / m_fftSize;
			}
		}
		else
	  		pffft_zconvolve_accumulate(m_setup, (float *) freqBinsDataA, (float *) freqBinsDataB, (float *) freqBinsAccum, 1.0 / m_fftSize);
	}
};

///////////////////////////////////////////////////////////////////////////////