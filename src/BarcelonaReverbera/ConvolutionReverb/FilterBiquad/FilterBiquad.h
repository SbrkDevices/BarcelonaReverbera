#pragma once

#include "ConvolutionReverbCommon.h"

///////////////////////////////////////////////////////////////////////////////

class FilterBiquad
{
public:
	enum
	{
		kCoeff_b0 = 0,
		kCoeff_b1,
		kCoeff_b2,
		kCoeff_a1,
		kCoeff_a2,

		kCoeff_Count
	};

	enum
	{
		kState_z1 = 0,
		kState_z2,
		
		kState_Count
	};

private:
	alignas(16) double m_state[kState_Count] = {};
	alignas(16) double m_coeffs[kCoeff_Count] = {};

	float m_cutoffFreq_Current = 20000.0f;
	bool m_lowPass = true;

public:
	void init(bool lowPass);
	void exit(void);

private:
	void computeCoefficientsButterworth2ndOrder(double samplerate);

public:
	inline void clearState(void)
	{
		for (uint32_t i=0; i<kState_Count; i++)
			m_state[i] = 0.0;
	}

	inline void reset(void)
	{
		clearState();

		for (uint32_t i=0; i<kCoeff_Count; i++)
			m_coeffs[i] = (i == kCoeff_b0) ? 1.0 : 0.0;
	}

	inline void setTargetFreq(float cutoffFreqTarget, float smoothingFactor, double samplerate)
	{
		DEBUG_ASSERT((cutoffFreqTarget >= 20.0f) && (cutoffFreqTarget <= 20000.0f));

		m_cutoffFreq_Current = DspUtils::expSmoothing(cutoffFreqTarget, m_cutoffFreq_Current, smoothingFactor);

		computeCoefficientsButterworth2ndOrder(samplerate);
	}

	// Direct Form II transposed (float/double for input/output (depends on template param), but always double for internal processing)
	template <typename _InOutFpType = float>
	inline void process(const _InOutFpType* audioInput, _InOutFpType* audioOutput, const uint32_t blockSize)
	{
		double b0 = m_coeffs[kCoeff_b0];
		double b1 = m_coeffs[kCoeff_b1];
		double b2 = m_coeffs[kCoeff_b2];
		double a1 = m_coeffs[kCoeff_a1];
		double a2 = m_coeffs[kCoeff_a2];

		double acc, d0, d1;

		d0 = m_state[kState_z1];
		d1 = m_state[kState_z2];

		for (uint32_t i=0; i<blockSize; i++)
		{
			const double x = audioInput[i];
	/*
			acc = b0 * x + d0;
			d0  = b1 * x - a1 * acc + d1;
			d1  = b2 * x - a2 * acc;
	*/
			acc  = b0 * x;
			acc += d0;

			d0  = b1 * x;
			d0 -= a1 * acc;
			d0 += d1;
			d1  = b2 * x;
			d1 -= a2 * acc;

			audioOutput[i] = acc;
		}

		m_state[kState_z1] = d0;
		m_state[kState_z2] = d1;
	}
};

///////////////////////////////////////////////////////////////////////////////