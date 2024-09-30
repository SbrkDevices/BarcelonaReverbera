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

#include "Fft.h"
#include "DspThread.h"

///////////////////////////////////////////////////////////////////////////////

#define CONVOLUTION_FFT_STAGE_USES_THREAD				(1)

#define ALWAYS_UPDATE_IR_BLOCKS							(1) // if enabled: uses less memory, but more processing required

///////////////////////////////////////////////////////////////////////////////

#define GET_NEXT_MULTIPLE_OF_4(value) 					(((value) + 3) & ~3) // used to ensure 16-byte memory alignment
#define GET_FFT_SIZE_TIME_DOMAIN(blockSize)				(2 * blockSize)
#define GET_FFT_SIZE_FREQ_DOMAIN(blockSize)				(blockSize + 1)

///////////////////////////////////////////////////////////////////////////////

template<uint32_t _blockSize, uint32_t _blockCountMax, bool _replacesDirectStage>
class ConvolutionEngineFftStage
{
private:
	static constexpr uint32_t m_blockSize = _blockSize; // the block size of this convolution stage
	static constexpr uint32_t m_blockCountMax = _blockCountMax; // maximum number of blocks in this convolution stage
	static constexpr uint32_t m_fftSizeTimeDomain = GET_FFT_SIZE_TIME_DOMAIN(m_blockSize); // FFT size (time-domain)
	static constexpr uint32_t m_fftSizeFreqDomain = GET_FFT_SIZE_FREQ_DOMAIN(m_blockSize); // FFT size (freq-domain)
	static constexpr uint32_t m_fftFreqDomainMultiDimBufSize = GET_NEXT_MULTIPLE_OF_4(m_fftSizeFreqDomain); // FFT size (freq-domain) for multi-dim. arrays
	static constexpr uint32_t m_replacesDirectStage = _replacesDirectStage; // indicates if this FFT stage is used to replace the direct stage (i.e. it is the first stage in the chain). If it is, there is no latency on this stage (convolution is performed on newest audio input)
	static constexpr uint32_t m_numBuffers = m_replacesDirectStage ? 1 : 2; // indicates whether double buffering is done

private:
	std::atomic<uint8_t> m_numChannels = 2; // 1 for mono, 2 for stereo
	static_assert(std::atomic<uint8_t>::is_always_lock_free);

	uint32_t m_blockCount = m_blockCountMax; // current number of blocks in this convolution stage

	uint32_t m_audioProcessingBlockSize = 0; // the general audio processing block size
	bool m_skipThisStage = false; // when the audio processing block size is greater than this convolution stage's block size, no processing is done on this stage (it is only done on larger stages)

	uint32_t m_convProcessingPointSamples = 0; // the point within m_blockSize when the convolution processing is done
	bool m_processInThread = false; // indicates whether block processing is done in a separate thread

	float* m_ir[2][2][m_blockCountMax] = {}; // impulse response (partitioned) (2 stereo buffers)
	std::atomic<uint8_t> m_irIndex = 0; // which of the 2 IR buffers is in use
	static_assert(std::atomic<uint8_t>::is_always_lock_free);

	alignas(16) float m_audioInputBuffer[m_numBuffers][2][m_fftSizeTimeDomain] = {}; // audio input bufffer (stereo)
	alignas(16) float m_audioOutputBuffer[m_numBuffers][2][m_blockSize] = {}; // audio output buffer (stereo)
	uint32_t m_audioBufferPtr = 0; // position for reading/writing into/from m_audioInputBuffer/m_audioOutputBuffer
	uint8_t m_audioReadWriteBufferIndex = 0; // index for double buffering (read/write) on m_audioInputBuffer/m_audioOutputBuffer
	std::atomic<uint8_t> m_audioProcessBufferIndex = 1; // index for double buffering (process) on m_audioInputBuffer/m_audioOutputBuffer
	static_assert(std::atomic<uint8_t>::is_always_lock_free);

	alignas(16) cplx_f32 m_AUDIO_IN_BLOCKS[2][m_blockCountMax][m_fftFreqDomainMultiDimBufSize] = {}; // last blocks of audio input (stereo), in freq-domain
	uint32_t m_audioInBlocksWritePtr = 0; // block write pointer for m_AUDIO_IN_BLOCKS

	alignas(16) float m_irBlock[m_fftSizeTimeDomain] = {}; // next block of the IR in time-domain, after processing, ready to FFT it.
# if ALWAYS_UPDATE_IR_BLOCKS
	alignas(16) cplx_f32 m_IR_BLOCK[m_fftSizeFreqDomain] = {}; // next block of the IR in freq. domain
# else
	alignas(16) cplx_f32 m_IR_BLOCKS[2][m_blockCountMax][m_fftFreqDomainMultiDimBufSize] = {}; // IR blocks (stereo) in freq. domain
# endif

	alignas(16) cplx_f32 m_CONV[m_fftSizeFreqDomain] = {}; // accumulator for the convolution result in freq. domain
	alignas(16) float m_conv[m_fftSizeTimeDomain] = {}; // stores the convolution result in time domain

	alignas(16) float m_overlap[2][m_blockSize]; // overlap section (stereo) of the time-domain convolution buffer (saved to be OLA-ed in next convolution)

	alignas(16) float m_dataFftWork[m_fftSizeTimeDomain] = {}; // internal working buffer for FFT/IFFT classes
# if !ALWAYS_UPDATE_IR_BLOCKS
	bool m_mustUpdateIrBlocks = true; // indicates that time-domain IR has changed, so freq-domain blocks must be updated
# endif

	Fft<true, false> m_fft; // forward FFT
	Fft<false, false> m_ifft; // inverse FFT

	DspThread m_thread;

public:
	ConvolutionEngineFftStage(void) : m_thread(juce::String("ConvolutionFftStage_") + juce::String(m_blockSize), [this] () { convolutionInit(); }, [this] () { convolutionExit(); }, [this] () { convolutionProcessOnSignal(); }) {}

public:
	inline void init(double samplerate, uint32_t audioProcessingBlockSize, uint8_t numChannels, float* ir0[2], float* ir1[2], uint32_t longestStageBlockSize, uint32_t longestStageBlockCount)
	{
		m_audioProcessingBlockSize = audioProcessingBlockSize;
		m_numChannels = numChannels;

#	  if CONVOLUTION_FFT_STAGE_USES_THREAD
		m_processInThread = (!m_replacesDirectStage && (m_blockSize > audioProcessingBlockSize));
#	  else
		m_processInThread = false;
#	  endif

		if (m_processInThread)
			m_convProcessingPointSamples = m_blockSize;
		else
			m_convProcessingPointSamples = (m_blockSize > m_audioProcessingBlockSize) ? m_blockSize / 2 : m_blockSize;

		m_skipThisStage = m_replacesDirectStage
			? (m_audioProcessingBlockSize != m_blockSize)
			: (m_audioProcessingBlockSize > m_blockSize);

		m_audioBufferPtr = 0;
		m_audioReadWriteBufferIndex = 0;
		m_audioProcessBufferIndex = 1;

		std::memset(m_audioOutputBuffer, 0, sizeof(m_audioOutputBuffer));

		for (uint32_t i=0; i<m_blockSize; i++)
		{
			for (uint32_t ch=0; ch<2; ch++)
			{
				for (uint32_t b=0; b<m_numBuffers; b++)
					m_audioInputBuffer[b][ch][m_blockSize + i] = 0.0f; // 2nd half of array: zero padded for FFT IN
			}
		}

		m_blockCount = (!m_replacesDirectStage && (longestStageBlockSize == m_blockSize)) ? longestStageBlockCount : m_blockCountMax;

		const uint32_t blockOffset = m_replacesDirectStage ? 0 : 2; // for all the FFT stages (if not replacing direct stage), blocks 0 and 1 are covered by smaller stages (in the case of the smallest FFT stage, they are covered by the direct stage)

		for (uint32_t b=0; b<m_blockCount; b++)
		{
			for (int ch=0; ch<2; ch++)
			{
				m_ir[0][ch][b] = &ir0[ch][(b + blockOffset) * m_blockSize];
				m_ir[1][ch][b] = &ir1[ch][(b + blockOffset) * m_blockSize];
			}
		}

		if (m_processInThread)
		{
#		  if JUCE_MAC
#		   if 0 // XXX does this work on Apple Silicon? Does not work on Intel...
			m_thread.startRealtimeThread(juce::Thread::RealtimeOptions{}.withMaximumProcessingTimeMs(m_blockSize*1000.0/samplerate));
#		   else
			m_thread.startRealtimeThread(juce::Thread::RealtimeOptions{}.withPriority(9));
#		   endif
#		  else
			//m_thread.startRealtimeThread(juce::Thread::RealtimeOptions{}.withPriority(9)); // XXX not working on linux?
			m_thread.startThread(juce::Thread::Priority::highest);
#		  endif
		}
		else
			convolutionInit();
	}

	inline void exit(void)
	{
		if (m_processInThread)
			DEBUG_VERIFY(m_thread.stopThread(1000));
		else
			convolutionExit();
	}

	inline void process(const float* __restrict audioIn[2] , float* __restrict audioOut[2])
	{
		if (m_skipThisStage)
			return;

		const uint8_t numChannels = m_numChannels;
		const uint32_t blockSize = m_blockSize;
		const uint32_t audioProcessingBlockSize = m_audioProcessingBlockSize;
		const uint32_t convProcessingPointSamples = m_convProcessingPointSamples;
		uint32_t audioBufferPtr = m_audioBufferPtr;
		uint8_t audioReadWriteBufferIndex = m_audioReadWriteBufferIndex;
		float* audioInputBuffer[2] = { nullptr, nullptr };
		float* audioOutputBuffer[2] = { nullptr, nullptr };

		for (uint32_t ch=0; ch<numChannels; ch++)
		{
			audioInputBuffer[ch] = &m_audioInputBuffer[audioReadWriteBufferIndex][ch][audioBufferPtr];
			audioOutputBuffer[ch] = &m_audioOutputBuffer[audioReadWriteBufferIndex][ch][audioBufferPtr];

			for (int i=0; i<audioProcessingBlockSize; i++)
			{
				audioInputBuffer[ch][i] = audioIn[ch][i];

				if (!m_replacesDirectStage)
					audioOut[ch][i] += audioOutputBuffer[ch][i];
			}
		}

		audioBufferPtr += audioProcessingBlockSize;

		if (audioBufferPtr == convProcessingPointSamples)
		{
			if (m_processInThread)
			{
				m_audioProcessBufferIndex = audioReadWriteBufferIndex;
				m_thread.notify();
			}
			else
			{
				m_audioProcessBufferIndex = (m_numBuffers == 2)
					? (audioReadWriteBufferIndex == 0) ? 1 : 0
					: audioReadWriteBufferIndex;

				convolutionProcessOnSignal();
			}
		}

		if (audioBufferPtr >= blockSize)
		{
			DEBUG_ASSERT(audioBufferPtr == blockSize);
			audioBufferPtr = 0;

			if (m_numBuffers == 2)
				audioReadWriteBufferIndex = (audioReadWriteBufferIndex == 0) ? 1 : 0;
		}

		if (m_replacesDirectStage)
		{
			for (uint32_t ch=0; ch<numChannels; ch++)
			{
				for (int i=0; i<audioProcessingBlockSize; i++)
					audioOut[ch][i] += audioOutputBuffer[ch][i];
			}
		}

		m_audioReadWriteBufferIndex = audioReadWriteBufferIndex;
		m_audioBufferPtr = audioBufferPtr;
	}

	inline bool canUpdateIr(void)
	{
		if (m_skipThisStage)
			return true;
		else
			return m_audioBufferPtr == (m_convProcessingPointSamples - m_audioProcessingBlockSize); // just before next offline processing (aka next thread wake-up signal)
	}

	inline void updateIr(uint8_t irIndex)
	{
		m_irIndex = irIndex;

#	  if !ALWAYS_UPDATE_IR_BLOCKS
		m_mustUpdateIrBlocks = true;
#	  endif
	}

private:
	void convolutionInit(void)
	{
		m_audioInBlocksWritePtr = 0;

		for (uint32_t i=0; i<m_blockSize; i++)
			m_irBlock[m_blockSize + i] = 0.0f; // 2nd half of array: zero padded for FFT IN

		for (uint32_t ch=0; ch<2; ch++)
		{
			for (uint32_t b=0; b<m_blockCount; b++)
				std::memset(m_AUDIO_IN_BLOCKS[ch][b], 0, m_fftFreqDomainMultiDimBufSize*sizeof(cplx_f32));
			
			std::memset(m_overlap[ch], 0, m_blockSize*sizeof(float));
		}

		m_fft.init(m_fftSizeTimeDomain, m_dataFftWork);
		m_ifft.init(m_fftSizeTimeDomain, m_dataFftWork);

#	  if !ALWAYS_UPDATE_IR_BLOCKS
		m_mustUpdateIrBlocks = true;
#	  endif
	}

	void convolutionExit(void)
	{
		m_fft.exit();
		m_ifft.exit();
	}

	inline void convolutionProcessOnSignal(void)
	{
#	  if !ALWAYS_UPDATE_IR_BLOCKS
		if (m_mustUpdateIrBlocks)
		{
			m_mustUpdateIrBlocks = false;

			convolutionUpdateIrBlocks();
		}
#	  endif

		const uint8_t numChannels = m_numChannels;
		const uint32_t blockCount = m_blockCount;
		const uint32_t blockSize = m_blockSize;
		const uint8_t audioProcessBufferIndex = m_audioProcessBufferIndex;
		const int audioInBlocksWritePtr = static_cast<int>(m_audioInBlocksWritePtr);

		for (uint32_t ch=0; ch<numChannels; ch++)
		{
			const float* in = m_audioInputBuffer[audioProcessBufferIndex][ch];
			float* out = m_audioOutputBuffer[audioProcessBufferIndex][ch];

			m_fft.process((float *) in, m_AUDIO_IN_BLOCKS[ch][audioInBlocksWritePtr]);

			std::memset(m_CONV, 0, sizeof(m_CONV));

			for (uint32_t b=0; b<blockCount; b++)
			{
				int audioInBlocksReadPtr = int(audioInBlocksWritePtr) - int(b);
				if (audioInBlocksReadPtr < 0)
					audioInBlocksReadPtr += blockCount;

#			  if ALWAYS_UPDATE_IR_BLOCKS
				convolutionProcessIrBlock(ch, b);

				//m_CONV += m_IR_BLOCK*m_AUDIO_IN_BLOCKS[audioInBlocksReadPtr];
				m_ifft.convolve_accum(m_CONV, m_IR_BLOCK, m_AUDIO_IN_BLOCKS[ch][audioInBlocksReadPtr]);
#			  else
				//m_CONV += m_IR_BLOCKS[ch][b]*m_AUDIO_IN_BLOCKS[ch][audioInBlocksReadPtr];
				m_ifft.convolve_accum(m_CONV, m_IR_BLOCKS[ch][b], m_AUDIO_IN_BLOCKS[ch][audioInBlocksReadPtr]);
#			  endif
			}

			m_ifft.process(m_conv, m_CONV);

			for (uint32_t i=0; i<blockSize; i++)
				out[i] = m_conv[i] + m_overlap[ch][i]; // 1st half of convolution result is overlapped with 2nd half of previous
		
			// 2nd half of convolution result is saved to be overlapped with next buffer:
			memcpy(m_overlap[ch], &m_conv[blockSize], blockSize*sizeof(float));
		}

		if (++m_audioInBlocksWritePtr >= blockCount)
		{
			DEBUG_ASSERT(m_audioInBlocksWritePtr == blockCount);
			m_audioInBlocksWritePtr = 0;
		}
	}

# if ALWAYS_UPDATE_IR_BLOCKS
	inline void convolutionProcessIrBlock(uint32_t ch, uint32_t blockIndex)
	{
		const float* ir = m_ir[m_irIndex][ch][blockIndex];

		memcpy(m_irBlock, ir, m_blockSize*sizeof(float));

		m_fft.process(m_irBlock, m_IR_BLOCK);
	}
# else
	inline void convolutionUpdateIrBlocks(void)
	{
		const uint8_t numChannels = m_numChannels;
		const uint32_t blockCount = m_blockCount;
		const uint32_t blockSize = m_blockSize;
		float* ir[2] = { m_ir[m_irIndex][0], m_ir[m_irIndex][1] };

		for (uint32_t ch=0; ch<numChannels; ch++)
		{
			for (uint32_t b=0; b<blockCount; b++)
			{
				const float* irBlock = ir[ch][b];

				memcpy(m_irBlock, irBlock, blockSize*sizeof(float)); // 1st half of array: IR data

				m_fft.process(m_irBlock, m_IR_BLOCKS[ch][b]);
			}
		}
	}
# endif
};

///////////////////////////////////////////////////////////////////////////////

