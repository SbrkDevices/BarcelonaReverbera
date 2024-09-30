#include "FilterBiquad.h"

///////////////////////////////////////////////////////////////////////////////

void FilterBiquad::init(bool lowPass)
{
	m_lowPass = lowPass;

	reset();
}

void FilterBiquad::exit(void)
{

}

///////////////////////////////////////////////////////////////////////////////

void FilterBiquad::computeCoefficientsButterworth2ndOrder(double samplerate)
{
	const double omega_c = 2.0 * M_PI * m_cutoffFreq_Current / double(samplerate);
    const double omega = std::tan(omega_c / 2.0);
    const double omega2 = omega * omega;
    const double sqrt2_omega = std::sqrt(2.0) * omega;

	const double a0 = 1.0 + sqrt2_omega + omega2;

	double b0, b1, b2, a1, a2;

	if (m_lowPass)
	{
		a1 = 2.0 * (omega2 - 1.0) / a0;
    	a2 = (1.0 - sqrt2_omega + omega2) / a0;
    
    	b0 = omega2 / a0;
    	b1 = 2.0 * b0;
    	b2 = b0;
	}
	else
	{
    	a1 = 2.0 * (omega2 - 1.0) / a0;
    	a2 = (1.0 - sqrt2_omega + omega2) / a0;
    
    	b0 = 1.0 / a0;
    	b1 = -2.0 * b0;
    	b2 = b0;
	}

	m_coeffs[kCoeff_b0] = b0;
	m_coeffs[kCoeff_b1] = b1;
	m_coeffs[kCoeff_b2] = b2;
	m_coeffs[kCoeff_a1] = a1;
	m_coeffs[kCoeff_a2] = a2;
}

///////////////////////////////////////////////////////////////////////////////