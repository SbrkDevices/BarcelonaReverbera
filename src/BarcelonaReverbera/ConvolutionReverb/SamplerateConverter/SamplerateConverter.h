#pragma once

#include "ConvolutionReverbCommon.h"

///////////////////////////////////////////////////////////////////////////////

class SamplerateConverter
{
public:
	static void convert(double samplerateIn, double samplerateOut, const float* audioIn[2], float* audioOut[2], uint32_t audioInLength, uint32_t audioOutLengthMax, uint32_t& audioOutLengthActual);
};

///////////////////////////////////////////////////////////////////////////////