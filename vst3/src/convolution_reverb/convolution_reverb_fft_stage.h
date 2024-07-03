#pragma once

//////////////////////////////////////////////////////////////////////////////

using namespace Steinberg;

//////////////////////////////////////////////////////////////////////////////

#define GET_NEXT_MULTIPLE_OF_4(value) 					(((value) + 3) & ~3) // used to ensure 16-byte memory alignment

#define CONVOLRVRB_GET_FFT_SIZE_TIME_DOMAIN(blockSize)	(2*blockSize)
#define CONVOLRVRB_GET_FFT_SIZE_FREQ_DOMAIN(blockSize)	(blockSize+1)

//////////////////////////////////////////////////////////////////////////////

template<uint32_t _blockSize, uint32_t _blockCountMax, bool _replacesDirectStage>
class ConvolReverbFftStage
{
private:
	static constexpr uint32_t m_blockSize = _blockSize; // the block size of this convolution stage
	static constexpr uint32_t m_blockCountMax = _blockCountMax; // maximum number of blocks in this convolution stage
	static constexpr uint32_t m_fftSizeTimeDomain = CONVOLRVRB_GET_FFT_SIZE_TIME_DOMAIN(m_blockSize); // FFT size (time-domain)
	static constexpr uint32_t m_fftSizeFreqDomain = CONVOLRVRB_GET_FFT_SIZE_FREQ_DOMAIN(m_blockSize); // FFT size (freq-domain)
	static constexpr uint32_t m_fftFreqDomainMultiDimBufSize = GET_NEXT_MULTIPLE_OF_4(m_fftSizeFreqDomain); // FFT size (freq-domain) for multi-dim. arrays
	static constexpr uint32_t m_replacesDirectStage = _replacesDirectStage; // indicates if this FFT stage is used to replace the direct stage (i.e. it is the first stage in the chain). If it is, there is no latency on this stage (convolution is performed on newest audio input)
	static constexpr uint32_t m_numBuffers = m_replacesDirectStage ? 1 : 2; // indicates whether double buffering is done

public:
	inline static constexpr uint32_t getBlockSize(void)
	{
		return m_blockSize;
	}

private:
	uint32_t m_blockCount = m_blockCountMax; // current number of blocks in this convolution stage

	uint32_t m_audioProcessingBlockSize = 0; // the general audio processing block size
	bool m_skipThisStage = false; // when the audio processing block size is greater than this convolution stage's block size, no processing is done on this stage (it is only done on larger stages)

	uint32_t m_convProcessingPointSamples = 0; // the point within m_blockSize when the convolution processing is done

	float* m_ir[2][m_blockCountMax] = {}; // impulse response (partitioned) (stereo)
	alignas(16) float m_audioInputBuffer[m_numBuffers][2][m_fftSizeTimeDomain] = {}; // audio input bufffer (stereo)
	alignas(16) float m_audioOutputBuffer[m_numBuffers][2][m_blockSize] = {}; // audio output buffer (stereo)
	uint32_t m_audioBufferPtr = 0; // position for reading/writing into/from m_audioInputBuffer/m_audioOutputBuffer
	uint8_t m_audioReadWriteBufferIndex = 0; // index for double buffering (read/write) on m_audioInputBuffer/m_audioOutputBuffer

	bool m_bufferIndexProcessed[2] = { false, false };

	alignas(16) cplx_f32 m_AUDIO_IN_BLOCKS[2][m_blockCountMax][m_fftFreqDomainMultiDimBufSize] = {}; // last blocks of audio input (stereo), in freq-domain
	uint32_t m_audioInBlocksWritePtr = 0; // block write pointer for m_AUDIO_IN_BLOCKS

	alignas(16) float m_irBlock[m_fftSizeTimeDomain] = {}; // next block of the IR in time-domain, after processing, ready to FFT it.
	alignas(16) cplx_f32 m_IR_BLOCK[m_fftSizeFreqDomain] = {}; // next block of the IR in freq. domain

	alignas(16) cplx_f32 m_CONV[m_fftSizeFreqDomain] = {}; // accumulator for the convolution result in freq. domain
	alignas(16) float m_conv[m_fftSizeTimeDomain] = {}; // stores the convolution result in time domain

	float m_overlap[2][m_blockSize]; // overlap section (stereo) of the time-domain convolution buffer (saved to be OLA-ed in next convolution)

	alignas(16) float m_dataFftWork[m_fftSizeTimeDomain] = {}; // internal working buffer for FFT/IFFT classes

	DspFft<true, false> m_fft; // forward FFT
	DspFft<false, false> m_ifft; // inverse 

private:
	inline void reset(void)
	{
		m_audioInBlocksWritePtr = 0;

		for (uint32_t ch=0; ch<2; ch++)
			memset(m_AUDIO_IN_BLOCKS[ch], 0, m_blockCount*m_fftFreqDomainMultiDimBufSize*sizeof(cplx_f32));
		memset(m_overlap, 0, sizeof(m_overlap));
		memset(m_audioOutputBuffer, 0, sizeof(m_audioOutputBuffer));

		m_audioBufferPtr = 0;
		m_audioReadWriteBufferIndex = 0;
	}

public:
	void init(uint32_t audioProcessingBlockSize)
	{
		m_audioProcessingBlockSize = audioProcessingBlockSize;
		
		m_convProcessingPointSamples = (m_blockSize > m_audioProcessingBlockSize) ? m_blockSize/2 : m_blockSize;

		m_skipThisStage = m_replacesDirectStage
			? (m_audioProcessingBlockSize != m_blockSize)
			: (m_audioProcessingBlockSize > m_blockSize);

		memset(m_audioOutputBuffer, 0, sizeof(m_audioOutputBuffer));

		for (uint32_t i=0; i<m_blockSize; i++)
		{
			for (uint32_t ch=0; ch<2; ch++)
			{
				for (uint32_t b=0; b<m_numBuffers; b++)
					m_audioInputBuffer[b][ch][m_blockSize + i] = 0.0f; // 2nd half of array: zero padded for FFT IN
			}

			m_irBlock[m_blockSize + i] = 0.0f; // 2nd half of array: zero padded for FFT IN
		}

		m_fft.init(m_fftSizeTimeDomain, m_dataFftWork);
		m_ifft.init(m_fftSizeTimeDomain, m_dataFftWork);

		m_bufferIndexProcessed[0] = true;
		m_bufferIndexProcessed[1] = false;

		reset();
	}

	void exit(void)
	{
		m_fft.exit();
		m_ifft.exit();
	}

	inline void setupIR(float* irL, float* irR, uint32_t longestStageBlockSize, uint32_t longestStageBlockCount)
	{
		m_blockCount = (!m_replacesDirectStage && (longestStageBlockSize == m_blockSize)) ? longestStageBlockCount : m_blockCountMax;

		const uint32_t blockOffset = m_replacesDirectStage ? 0 : 2; // for all the FFT stages (if not replacing direct stage), blocks 0 and 1 are covered by smaller stages (in the case of the smallest FFT stage, they are covered by the direct stage)

		for (uint32_t b=0; b<m_blockCount; b++)
		{
			m_ir[0][b] = &irL[(b + blockOffset)*m_blockSize];
			m_ir[1][b] = &irR[(b + blockOffset)*m_blockSize];
		}

		reset();
	}

	void process(Vst::Sample32** inChannels, Vst::Sample32** outChannels, uint32_t numChannels)
	{
		if (m_skipThisStage)
			return;

		assert(m_bufferIndexProcessed[m_audioReadWriteBufferIndex]);

		float* audioInputBuffer[2] = { nullptr, nullptr};
		float* audioOutputBuffer[2] = { nullptr, nullptr};

		for (uint32_t ch=0; ch<numChannels; ch++)
		{
			audioInputBuffer[ch] = &m_audioInputBuffer[m_audioReadWriteBufferIndex][ch][m_audioBufferPtr];
			audioOutputBuffer[ch] = &m_audioOutputBuffer[m_audioReadWriteBufferIndex][ch][m_audioBufferPtr];

			for (int i=0; i<m_audioProcessingBlockSize; i++)
			{
				audioInputBuffer[ch][i] = inChannels[ch][i];

				if (!m_replacesDirectStage)
					outChannels[ch][i] += audioOutputBuffer[ch][i];
			}
		}

		m_audioBufferPtr += m_audioProcessingBlockSize;

		if (m_audioBufferPtr >= m_blockSize)
			m_bufferIndexProcessed[m_audioReadWriteBufferIndex] = false;

		if (m_audioBufferPtr == m_convProcessingPointSamples)
			processConvolution(numChannels);

		if (m_audioBufferPtr >= m_blockSize)
		{
			assert(m_audioBufferPtr == m_blockSize);
			m_audioBufferPtr = 0;

			if (m_numBuffers == 2)
				m_audioReadWriteBufferIndex = (m_audioReadWriteBufferIndex == 0) ? 1 : 0;
		}

		if (m_replacesDirectStage)
		{
			for (uint32_t ch=0; ch<numChannels; ch++)
			{
				for (int i=0; i<m_audioProcessingBlockSize; i++)
					outChannels[ch][i] += audioOutputBuffer[ch][i];
			}
		}
	}

private:
	inline void processConvolution(uint32_t numChannels)
	{
		const uint8_t audioProcessBufferIndex = (m_numBuffers == 2)
			? (m_audioReadWriteBufferIndex == 0) ? 1 : 0
			: m_audioReadWriteBufferIndex;

		assert(!m_bufferIndexProcessed[audioProcessBufferIndex]);

		for (uint32_t ch=0; ch<numChannels; ch++)
		{
			const float* in = m_audioInputBuffer[audioProcessBufferIndex][ch];
			float* out = m_audioOutputBuffer[audioProcessBufferIndex][ch];

			m_fft.process((float *) in, m_AUDIO_IN_BLOCKS[ch][m_audioInBlocksWritePtr]);

			memset(m_CONV, 0, sizeof(m_CONV));

			for (uint32_t b=0; b<m_blockCount; b++)
			{
				int audioInBlocksReadPtr = int(m_audioInBlocksWritePtr) - int(b);
				if (audioInBlocksReadPtr < 0)
					audioInBlocksReadPtr += m_blockCount;

				processIrBlock(ch, b);

				//m_CONV += m_IR_BLOCK*m_AUDIO_IN_BLOCKS[audioInBlocksReadPtr];
				m_ifft.convolve_accum(m_CONV, m_IR_BLOCK, m_AUDIO_IN_BLOCKS[ch][audioInBlocksReadPtr]);
			}

			m_ifft.process(m_conv, m_CONV);

			for (uint32_t i=0; i<m_blockSize; i++)
				out[i] = m_conv[i] + m_overlap[ch][i]; // 1st half of convolution result is overlapped with 2nd half of previous
		
			// 2nd half of convolution result is saved to be overlapped with next buffer:
			memcpy(m_overlap[ch], &m_conv[m_blockSize], m_blockSize*sizeof(float));
		}

		if (++m_audioInBlocksWritePtr >= m_blockCount)
		{
			assert(m_audioInBlocksWritePtr == m_blockCount);
			m_audioInBlocksWritePtr = 0;
		}

		m_bufferIndexProcessed[audioProcessBufferIndex] = true;
	}

	inline void processIrBlock(uint32_t ch, uint32_t blockIndex)
	{
		const float* ir = m_ir[ch][blockIndex];

		for (uint32_t i=0; i<m_blockSize; i++)
		{
			const float gain = 1.0f;

			// XXX substitute with time-domain IR processing:
			m_irBlock[i] = gain*ir[i]; // 1st half of FFT IN array: IR data at current block
		}

		m_fft.process(m_irBlock, m_IR_BLOCK);
	}
};

//////////////////////////////////////////////////////////////////////////////

