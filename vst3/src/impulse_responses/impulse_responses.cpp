#include "impulse_responses.h"

#include "impulse_responses/IR_DELTA.h"
#include "impulse_responses/IR_DELAY.h"

//////////////////////////////////////////////////////////////////////////////

char ImpulseResponses::m_irName[IMPULSE_RESPONSES_COUNT][128];

//////////////////////////////////////////////////////////////////////////////

void ImpulseResponses::init(void)
{
	strcpy(m_irName[0], "Delta Function");
	m_irLenWithZeros[0] = sizeof(__IR_DELTA__)/(2*sizeof(float));
	m_irLen[0] =  m_irLenWithZeros[0] - __IR_DELTA_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irPtr[0][ch] = __IR_DELTA__[ch];

	strcpy(m_irName[1], "Delta Function With Delay");
	m_irLenWithZeros[1] = sizeof(__IR_DELAY__)/(2*sizeof(float));
	m_irLen[1] =  m_irLenWithZeros[1] - __IR_DELAY_EXTRA_ZEROS__;
	for (int ch=0; ch<2; ch++)
		m_irPtr[1][ch] = __IR_DELAY__[ch];
}

//////////////////////////////////////////////////////////////////////////////