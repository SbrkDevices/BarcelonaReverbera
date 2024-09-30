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

/*
The non-uniform partition scheme is shown here (BS is the block size of the smallest FFT stage):

Method:                  FFT                                FFT                      FFT               FFT         FFT      FFT    FFT  FFT  DIRECT

Block size:		    	 8BS                                8BS                      4BS               4BS         2BS      2BS    BS   BS    2BS   

X (audio in):          X-24-H31                           X-16-23                  X-12-15           X-8-11       X-6-7    X-4-5   X-3  X-2  X-1 X0
     ...   |--------------------------------|--------------------------------|----------------|----------------|--------|--------|----|----|--------|
     ...   |--------------------------------|--------------------------------|----------------|----------------|--------|--------|----|----|--------|
H (IR):                H24-H31                            H16-23                   H12-15            H8-11        H6-7     H4-5    H3   H2   H1  H0
*/

///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <tuple>

#include "ConvolutionReverbCommon.h"

#include "ConvolutionEngineDirectStage.h"
#include "ConvolutionEngineFftStage.h"

///////////////////////////////////////////////////////////////////////////////

#define LONGEST_STAGE_BLOCK_COUNT				((BCNRVRB_IR_MAX_LEN_SAMPLES / BCNRVRB_LONGEST_STAGE_SIZE + 1) - 2) // + 1 for ceiling, -2 because first 2 blocks are covered by smaller block stages

///////////////////////////////////////////////////////////////////////////////

// recursive template to generate stages (doubling block size every time). Stages is a variadic template parameter, representing already generated stages. Generates a new ConvolutionEngineFftStage and appends it to the list of Stages.
template<uint32_t BlockSize, uint32_t LastBlockSize, bool ReplacesFirstStage, typename... Stages>
struct GenerateFftStages
{
    using type = typename GenerateFftStages<BlockSize * 2, LastBlockSize, ReplacesFirstStage, Stages..., ConvolutionEngineFftStage<BlockSize, 2, ReplacesFirstStage>>::type;
};

// specialization for the last stage (ends the recursion when BlockSize equals LastBlockSize). Creates a tuple from the accumulated Stages and the final stage.
template<uint32_t LastBlockSize, bool ReplacesFirstStage, typename... Stages>
struct GenerateFftStages<LastBlockSize, LastBlockSize, ReplacesFirstStage, Stages...>
{
    using type = std::tuple<Stages..., ConvolutionEngineFftStage<LastBlockSize, ReplacesFirstStage ? 2 : LONGEST_STAGE_BLOCK_COUNT, ReplacesFirstStage>>;
};

// alias template to simplify the usage of the GenerateFftStages structure.
template<uint32_t InitialBlockSize, uint32_t LastBlockSize, bool ReplacesFirstStage = false>
using GenerateFftStages_t = typename GenerateFftStages<InitialBlockSize, LastBlockSize, ReplacesFirstStage>::type;

///////////////////////////////////////////////////////////////////////////////

class ConvolutionEngine
{
private:
	uint32_t m_audioProcessingBlockSize = 0; // the general audio processing block size (not the convolution stages' block sizes)
	uint8_t m_numChannels = 2; // 1 for mono, 2 for stereo
	
	ConvolutionEngineDirectStage<BCNRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE> m_directStage;

    GenerateFftStages_t<BCNRVRB_SMALLEST_STAGE_SIZE, BCNRVRB_LONGEST_STAGE_SIZE> m_fftStages;
	/*
		this creates something like:

		std::tuple<
			ConvolutionEngineFftStage<1 * BCNRVRB_SMALLEST_STAGE_SIZE, 2>,
			ConvolutionEngineFftStage<2 * BCNRVRB_SMALLEST_STAGE_SIZE, 2>,
			ConvolutionEngineFftStage<4 * BCNRVRB_SMALLEST_STAGE_SIZE, 2>,
			...
			ConvolutionEngineFftStage<BCNRVRB_LONGEST_STAGE_SIZE, LONGEST_STAGE_BLOCK_COUNT>
		>
	*/

    GenerateFftStages_t<2*BCNRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE, BCNRVRB_LONGEST_STAGE_SIZE, true> m_fftStagesReplacingDirectStage; // when the audio block size is larger than BCNRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE, the direct stage is replaced by an FFT stage with no latency.

    // helper function to iterate over tuple elements and call a member function
    template<typename Func, uint32_t I=0>
    inline void for_each_fft_stage(Func&& func)
	{
        std::apply([&func] (auto&... stage) { (func(stage), ...); }, m_fftStages);
    }

	template<typename Func, uint32_t I=0>
	inline void for_each_fft_stage_replacing_direct_stage(Func&& func)
	{
		std::apply([&func] (auto&... stage) { (func(stage), ...); }, m_fftStagesReplacingDirectStage);
	}

public:
	inline void init(double samplerate, uint32_t audioProcessingBlockSize, uint8_t numChannels, float* ir0[2], float* ir1[2], uint32_t irLenWithoutZeros, uint32_t irLenWithZeros)
	{
		m_audioProcessingBlockSize = audioProcessingBlockSize;
		m_numChannels = numChannels;

		const uint32_t directStageBlockSize = (audioProcessingBlockSize < BCNRVRB_SMALLEST_STAGE_SIZE) ? BCNRVRB_SMALLEST_STAGE_SIZE : audioProcessingBlockSize;

		// align IR length with longest stage's block size:
		const uint32_t irLenPadded = ((irLenWithoutZeros % BCNRVRB_LONGEST_STAGE_SIZE) == 0)
			? irLenWithoutZeros
			: irLenWithoutZeros + BCNRVRB_LONGEST_STAGE_SIZE - (irLenWithoutZeros % BCNRVRB_LONGEST_STAGE_SIZE);

		if ((irLenPadded < irLenWithoutZeros) || (irLenPadded > irLenWithZeros))
		{
			DEBUG_ASSERT(false);
			return;
		}

		if (float(irLenPadded)/float(BCNRVRB_LONGEST_STAGE_SIZE) < 2)
		{
			DEBUG_ASSERT(false);
			return;
		}

		const uint32_t irBlockCountLg = irLenPadded / BCNRVRB_LONGEST_STAGE_SIZE - 2; // -2 because first 2 blocks are covered by smaller blocks
		DEBUG_ASSERT((irBlockCountLg + 2) * BCNRVRB_LONGEST_STAGE_SIZE == irLenPadded);

		if (m_audioProcessingBlockSize <= BCNRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE)
			m_directStage.init(audioProcessingBlockSize, directStageBlockSize, numChannels, ir0, ir1);

		for_each_fft_stage_replacing_direct_stage([samplerate, audioProcessingBlockSize, numChannels, ir0, ir1, irBlockCountLg] (auto& stage)
		{
			stage.init(samplerate, audioProcessingBlockSize, numChannels, ir0, ir1, BCNRVRB_LONGEST_STAGE_SIZE, irBlockCountLg);
		});

        for_each_fft_stage([samplerate, audioProcessingBlockSize, numChannels, ir0, ir1, irBlockCountLg] (auto& stage)
		{
			stage.init(samplerate, audioProcessingBlockSize, numChannels, ir0, ir1, BCNRVRB_LONGEST_STAGE_SIZE, irBlockCountLg);
		});
	}

	inline void exit(void)
	{
		if (m_audioProcessingBlockSize <= BCNRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE)
			m_directStage.exit();

		for_each_fft_stage_replacing_direct_stage([] (auto& stage) { stage.exit(); });

        for_each_fft_stage([] (auto& stage) { stage.exit(); });
	}

	inline void process(const float* __restrict audioIn[2], float* __restrict audioOut[2])
	{
		const uint8_t numChannels = m_numChannels;
		const uint32_t audioProcessingBlockSize = m_audioProcessingBlockSize;

		for (uint32_t ch=0; ch<numChannels; ch++)
			std::memset(audioOut[ch], 0, audioProcessingBlockSize*sizeof(float));

		if (audioProcessingBlockSize <= BCNRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE)
			m_directStage.process(audioIn, audioOut);

		for_each_fft_stage_replacing_direct_stage([audioIn, audioOut] (auto& stage) { stage.process(audioIn, audioOut); });

        for_each_fft_stage([audioIn, audioOut] (auto& stage) { stage.process(audioIn, audioOut); });
	}

	inline bool canUpdateIr(void)
	{
		bool retVal = true;

		if (!m_directStage.canUpdateIr())
			retVal = false;
		
		for_each_fft_stage_replacing_direct_stage([&retVal] (auto& stage)
		{
			if (!stage.canUpdateIr())
				retVal = false;
		});

        for_each_fft_stage([&retVal] (auto& stage)
		{
			if (!stage.canUpdateIr())
				retVal = false;
		});

		return retVal;
	}

	inline void updateIr(uint8_t irIndex)
	{
		m_directStage.updateIr(irIndex);
		
		for_each_fft_stage_replacing_direct_stage([irIndex] (auto& stage) { stage.updateIr(irIndex); });

		for_each_fft_stage([irIndex] (auto& stage) { stage.updateIr(irIndex); });
	}
};

///////////////////////////////////////////////////////////////////////////////
