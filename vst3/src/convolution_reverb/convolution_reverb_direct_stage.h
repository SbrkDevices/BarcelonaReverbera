#pragma once

//////////////////////////////////////////////////////////////////////////////

using namespace Steinberg;

//////////////////////////////////////////////////////////////////////////////

class ConvolReverbDirectStage
{
private:
	uint32_t m_audioProcessingBlockSize = 0; // the general audio processing block size
	uint32_t m_blockSize2Blocks = 0; // direct convolution stage covers 2 initial blocks

	alignas(16) float m_convAccum[2][2*CONVRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE] = {}; // convolution accumulation buffer (stereo)
	float* m_ir[2] = { nullptr, nullptr }; // impulse response (stereo)

	uint32_t m_currentPos = 0;

private:
	inline void reset(void)
	{
		assert((m_blockSize2Blocks*sizeof(float)) <= sizeof(m_convAccum[0]));
		memset(m_convAccum[0], 0, m_blockSize2Blocks*sizeof(float));
		memset(m_convAccum[1], 0, m_blockSize2Blocks*sizeof(float));

		m_currentPos = 0;
	}

public:
	inline void init(uint32_t audioProcessingBlockSize, uint32_t blockSize)
	{
		m_audioProcessingBlockSize = audioProcessingBlockSize;
		m_blockSize2Blocks = 2*blockSize;

		reset();
	}
	inline void exit(void)
	{

	}

	inline void setupIR(float* irL, float* irR)
	{
		m_ir[0] = irL;
		m_ir[1] = irR;

		reset();
	}

	void process(Vst::Sample32** inChannels, Vst::Sample32** outChannels, uint32_t numChannels)
	{
		for (uint32_t i=0; i<m_audioProcessingBlockSize; i++)
		{
			for (uint32_t j=0; j<m_blockSize2Blocks; j++)
			{
				uint32_t writePtr = m_currentPos + j;
				if (writePtr >= m_blockSize2Blocks)
					writePtr -= m_blockSize2Blocks;

				for (uint32_t ch=0; ch<numChannels; ch++)
					m_convAccum[ch][writePtr] += inChannels[ch][i]*m_ir[ch][j];
			}

			for (uint32_t ch=0; ch<numChannels; ch++)
			{
				outChannels[ch][i] += m_convAccum[ch][m_currentPos];
				m_convAccum[ch][m_currentPos] = 0.0f;
			}

			if (++m_currentPos == m_blockSize2Blocks)
				m_currentPos = 0;
		}
	}
};

//////////////////////////////////////////////////////////////////////////////

