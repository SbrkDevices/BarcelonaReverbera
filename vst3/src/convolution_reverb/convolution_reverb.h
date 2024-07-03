#pragma once

#include <tuple>

#include "barcelona_reverbera_cids.h"

#include "convolution_reverb_direct_stage.h"
#include "convolution_reverb_fft_stage.h"

/*

The non-uniform partition scheme is shown here (BS is the block size of the smallest FFT stage):

Method:                  FFT                                FFT                      FFT               FFT         FFT      FFT    FFT  FFT  DIRECT

Block size:		    	 8BS                                8BS                      4BS               4BS         2BS      2BS    BS   BS    2BS   

X (audio in):          X-24-H31                           X-16-23                  X-12-15           X-8-11       X-6-7    X-4-5   X-3  X-2  X-1 X0
     ...   |--------------------------------|--------------------------------|----------------|----------------|--------|--------|----|----|--------|
     ...   |--------------------------------|--------------------------------|----------------|----------------|--------|--------|----|----|--------|
H (IR):                H24-H31                            H16-23                   H12-15            H8-11        H6-7     H4-5    H3   H2   H1  H0
*/

//////////////////////////////////////////////////////////////////////////////

#define CONVOL_REVERB_LONGEST_STAGE_BLOCK_COUNT		((BARCELONA_REVERBERA_IR_MAX_LEN_SAMPLES/CONVRVRB_LONGEST_STAGE_SIZE + 1) - 2) // + 1 for ceiling, -2 because first 2 blocks are covered by smaller block stages

//////////////////////////////////////////////////////////////////////////////

// recursive template to generate stages (doubling block size every time). Stages is a variadic template parameter, representing already generated stages. Generates a new ConvolReverbFftStage and appends it to the list of Stages.
template<uint32_t BlockSize, uint32_t LastBlockSize, bool ReplacesFirstStage, typename... Stages>
struct GenerateFftStages
{
    using type = typename GenerateFftStages<BlockSize * 2, LastBlockSize, ReplacesFirstStage, Stages..., ConvolReverbFftStage<BlockSize, 2, ReplacesFirstStage>>::type;
};

// specialization for the last stage (ends the recursion when BlockSize equals LastBlockSize). Creates a tuple from the accumulated Stages and the final stage.
template<uint32_t LastBlockSize, bool ReplacesFirstStage, typename... Stages>
struct GenerateFftStages<LastBlockSize, LastBlockSize, ReplacesFirstStage, Stages...>
{
    using type = std::tuple<Stages..., ConvolReverbFftStage<LastBlockSize, ReplacesFirstStage ? 2 : CONVOL_REVERB_LONGEST_STAGE_BLOCK_COUNT, ReplacesFirstStage>>;
};

// alias template to simplify the usage of the GenerateFftStages structure.
template<uint32_t InitialBlockSize, uint32_t LastBlockSize, bool ReplacesFirstStage = false>
using GenerateFftStages_t = typename GenerateFftStages<InitialBlockSize, LastBlockSize, ReplacesFirstStage>::type;

//////////////////////////////////////////////////////////////////////////////

class ConvolutionReverb
{
private:
	uint32_t m_audioProcessingBlockSize = 0; // the general audio processing block size
	
	ConvolReverbDirectStage m_directStage;

    GenerateFftStages_t<CONVRVRB_SMALLEST_STAGE_SIZE, CONVRVRB_LONGEST_STAGE_SIZE> m_fftStages;
	/*
		this creates something like:

		std::tuple<
			ConvolReverbFftStage<1 * CONVRVRB_SMALLEST_STAGE_SIZE, 2>,
			ConvolReverbFftStage<2 * CONVRVRB_SMALLEST_STAGE_SIZE, 2>,
			ConvolReverbFftStage<4 * CONVRVRB_SMALLEST_STAGE_SIZE, 2>,
			...
			ConvolReverbFftStage<CONVRVRB_LONGEST_STAGE_SIZE, CONVOL_REVERB_LONGEST_STAGE_BLOCK_COUNT>
		>
	*/

    GenerateFftStages_t<2*CONVRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE, CONVRVRB_LONGEST_STAGE_SIZE, true> m_fftStagesReplacingDirectStage; // when the audio block size is larger than CONVRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE, the direct stage is replaced by an FFT stage with no latency.

    // helper function to iterate over tuple elements and call a member function
    template<typename Func, uint32_t I=0>
    inline void for_each_fft_stage(Func&& func)
	{
        std::apply([&func](auto&... stage) { (func(stage), ...); }, m_fftStages);
    }

	template<typename Func, uint32_t I=0>
	inline void for_each_fft_stage_replacing_direct_stage(Func&& func)
	{
		std::apply([&func](auto&... stage) { (func(stage), ...); }, m_fftStagesReplacingDirectStage);
	}

public:
	inline void init(uint32_t audioProcessingBlockSize)
	{
		exit();

		m_audioProcessingBlockSize = audioProcessingBlockSize;

		const uint32_t directStageBlockSize = (audioProcessingBlockSize < CONVRVRB_SMALLEST_STAGE_SIZE) ? CONVRVRB_SMALLEST_STAGE_SIZE : audioProcessingBlockSize;

		if (m_audioProcessingBlockSize <= CONVRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE)
			m_directStage.init(audioProcessingBlockSize, directStageBlockSize);

		for_each_fft_stage_replacing_direct_stage([audioProcessingBlockSize](auto& stage) { stage.init(audioProcessingBlockSize); });

        for_each_fft_stage([audioProcessingBlockSize](auto& stage) { stage.init(audioProcessingBlockSize); });
	}

	inline void setupIR(float* irL, float* irR, uint32_t irLen)
	{
		// align IR length with longest blocks count:
		const uint32_t irLenPadded = irLen = irLen + CONVRVRB_LONGEST_STAGE_SIZE - (irLen % CONVRVRB_LONGEST_STAGE_SIZE);

		// set the added extra samples to zero:
		assert(irLenPadded >= irLen);
		for (uint32_t i=irLen; i<irLenPadded; i++)
		{
			irL[i] = 0.0f;
			irR[i] = 0.0f;
		}

		const uint32_t irBlockCountLg = irLenPadded/CONVRVRB_LONGEST_STAGE_SIZE - 2; // -2 because first 2 blocks are covered by smaller blocks
		assert((irBlockCountLg + 2)*CONVRVRB_LONGEST_STAGE_SIZE == irLenPadded);

		m_directStage.setupIR(irL, irR);

		for_each_fft_stage_replacing_direct_stage([irL, irR, irBlockCountLg](auto& stage)
		{
			stage.setupIR(irL, irR, CONVRVRB_LONGEST_STAGE_SIZE, irBlockCountLg);
		});

		for_each_fft_stage([irL, irR, irBlockCountLg](auto& stage)
		{
			stage.setupIR(irL, irR, CONVRVRB_LONGEST_STAGE_SIZE, irBlockCountLg);
		});
	}

	inline void exit(void)
	{
		if (m_audioProcessingBlockSize <= CONVRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE)
			m_directStage.exit();

		for_each_fft_stage_replacing_direct_stage([](auto& stage) { stage.exit(); });

        for_each_fft_stage([](auto& stage) { stage.exit(); });
	}

	inline void process(Vst::Sample32** inChannels, Vst::Sample32** outChannels, uint32_t numChannels)
	{
		for (uint32_t ch=0; ch<numChannels; ch++)
			memset(outChannels[ch], 0, m_audioProcessingBlockSize*sizeof(float));

		if (m_audioProcessingBlockSize <= CONVRVRB_DIRECT_STAGE_MAX_BLOCK_SIZE)
			m_directStage.process(inChannels, outChannels, numChannels);

		for_each_fft_stage_replacing_direct_stage([inChannels, outChannels, numChannels](auto& stage) { stage.process(inChannels, outChannels, numChannels); });

        for_each_fft_stage([inChannels, outChannels, numChannels](auto& stage) { stage.process(inChannels, outChannels, numChannels); });
	}
};

//////////////////////////////////////////////////////////////////////////////
