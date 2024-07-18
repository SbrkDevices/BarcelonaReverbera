#include "samplerate_converter.h"

#include <cassert>
#include <cmath>

//////////////////////////////////////////////////////////////////////////////

void SamplerateConverter::convert(double samplerateIn, double samplerateOut, float* audioIn[2], float* audioOut[2], uint32_t audioInLength, uint32_t audioOutLengthMax, uint32_t& audioOutLengthActual)
{
	const bool maxQuality = true; // XXX too slow to execute?

	SRC_DATA converterData;

	converterData.src_ratio = samplerateOut/samplerateIn;

	for (int ch=0; ch<2; ch++)
	{
		converterData.data_in = audioIn[ch];
		converterData.data_out = audioOut[ch];
		converterData.input_frames = audioInLength;
		converterData.output_frames = audioOutLengthMax;
		converterData.end_of_input = 1;
				
		const int res = src_simple(&converterData, maxQuality ? SRC_SINC_BEST_QUALITY : SRC_SINC_MEDIUM_QUALITY, 1);
		assert(res == 0);

		audioOutLengthActual = converterData.output_frames_gen;
	}
}

//////////////////////////////////////////////////////////////////////////////
