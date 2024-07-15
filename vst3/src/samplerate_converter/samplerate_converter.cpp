#include "samplerate_converter.h"

#include <cassert>

//////////////////////////////////////////////////////////////////////////////

void SamplerateConverter::init(void)
{
	assert(m_converter == nullptr);

	const bool maxQuality = true; // XXX too slow?

	int libsamplerateError = 0;
	m_converter = src_new(maxQuality ? SRC_SINC_BEST_QUALITY : SRC_SINC_MEDIUM_QUALITY, 1, &libsamplerateError);
}

void SamplerateConverter::exit(void)
{
	if (m_converter == nullptr)
		return;
	
	src_delete(m_converter);

	m_converter = nullptr;
}

//////////////////////////////////////////////////////////////////////////////

void SamplerateConverter::convert(double samplerateIn, double samplerateOut, float* audioIn[2], float* audioOut[2], uint32_t audioInLength, uint32_t& audioOutLength) // samplerateIn: IR's samplerate ; samplerateOut: current samplerate
{
	m_converterData.src_ratio = samplerateOut/samplerateIn;

	for (int ch=0; ch<2; ch++)
	{
		m_converterData.data_in = audioIn[ch];
		m_converterData.data_out = audioOut[ch];
		m_converterData.input_frames = audioInLength;
		m_converterData.output_frames = floor(double(audioInLength)*samplerateOut/samplerateIn);
		m_converterData.end_of_input = 1;
				
		const int res = src_process(m_converter, &m_converterData);
		assert(res == 0);

		audioOutLength = m_converterData.output_frames_gen;
	}
}

//////////////////////////////////////////////////////////////////////////////
