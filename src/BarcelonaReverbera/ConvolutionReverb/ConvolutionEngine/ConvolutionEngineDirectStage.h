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

///////////////////////////////////////////////////////////////////////////////

template<uint32_t _maxBlockSize>
class ConvolutionEngineDirectStage
{
private:
	static constexpr uint32_t m_maxBlockSize = _maxBlockSize;

private:
	uint32_t m_audioProcessingBlockSize = 0; // the general audio processing block size
	uint32_t m_blockSize2Blocks = 0; // direct convolution stage covers 2 initial blocks
	uint8_t m_numChannels = 2; // 1 for mono, 2 for stereo

	alignas(16) float m_convAccum[2][2 * m_maxBlockSize] = {}; // convolution accumulation buffer (stereo)
	float* m_ir[2][2] = { { nullptr, nullptr } , { nullptr, nullptr } }; // impulse response (2 stereo buffers)
	std::atomic<uint8_t> m_irIndex = 0; // which of the 2 IR buffers is in use
	static_assert(std::atomic<uint8_t>::is_always_lock_free);

	uint32_t m_currentPos = 0;

public:
	inline void init(uint32_t audioProcessingBlockSize, uint32_t blockSize, uint8_t numChannels, float* ir0[2], float* ir1[2])
	{
		m_audioProcessingBlockSize = audioProcessingBlockSize;
		m_blockSize2Blocks = 2 * blockSize;
		m_numChannels = numChannels;
		m_currentPos = 0;

		for (int ch=0; ch<2; ch++)
		{
			m_ir[0][ch] = ir0[ch];
			m_ir[1][ch] = ir1[ch];
		}

		DEBUG_ASSERT((m_blockSize2Blocks*sizeof(float)) <= sizeof(m_convAccum[0]));
		std::memset(m_convAccum[0], 0, m_blockSize2Blocks*sizeof(float));
		std::memset(m_convAccum[1], 0, m_blockSize2Blocks*sizeof(float));
	}
	inline void exit(void)
	{
	}

	inline void process(const float* __restrict audioIn[2], float* __restrict audioOut[2])
	{
		const uint8_t numChannels = m_numChannels;
		const uint32_t audioProcessingBlockSize = m_audioProcessingBlockSize;
		const uint32_t blockSize2Blocks = m_blockSize2Blocks;
		float* ir[2] = { m_ir[m_irIndex][0], m_ir[m_irIndex][1] };
		uint32_t currentPos = m_currentPos;
	
		for (uint32_t i=0; i<audioProcessingBlockSize; i++)
		{
			for (uint32_t j=0; j<blockSize2Blocks; j++)
			{
				uint32_t writePtr = currentPos + j;
				if (writePtr >= blockSize2Blocks)
					writePtr -= blockSize2Blocks;

				for (uint32_t ch=0; ch<numChannels; ch++)
					m_convAccum[ch][writePtr] += audioIn[ch][i]*ir[ch][j];
			}

			for (uint32_t ch=0; ch<numChannels; ch++)
			{
				audioOut[ch][i] += m_convAccum[ch][currentPos];
				m_convAccum[ch][currentPos] = 0.0f;
			}

			if (++currentPos == blockSize2Blocks)
				currentPos = 0;
		}

		m_currentPos = currentPos;
	}

	inline bool canUpdateIr(void)
	{
		return true; // no offline processing
	}

	inline void updateIr(uint8_t irIndex)
	{
		m_irIndex = irIndex;
	}
};

///////////////////////////////////////////////////////////////////////////////

