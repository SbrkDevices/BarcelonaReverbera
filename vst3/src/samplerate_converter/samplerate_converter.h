#pragma once

#include <stdint.h>
#include "samplerate.h"

//////////////////////////////////////////////////////////////////////////////

class SamplerateConverter
{
private:
	SRC_STATE* m_converter = nullptr;
	SRC_DATA m_converterData;

public:
	void init(void);
	void exit(void);

	void convert(double samplerateIn, double samplerateOut, float* audioIn[2], float* audioOut[2], uint32_t audioInLength, uint32_t& audioOutLength);
};

//////////////////////////////////////////////////////////////////////////////