#include "SamplerateConverter.h"
#include <JuceHeader.h>

///////////////////////////////////////////////////////////////////////////////

class ResamplingAudioSource : public juce::AudioSource
{
public:
    ResamplingAudioSource(const float* sourceAudio, uint32_t numSamples)
		: m_sourceAudio(sourceAudio), m_numSamples(numSamples) {}

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {}

    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();

        const int numSamples = std::min(bufferToFill.numSamples, m_numSamples - m_currentSample);

		bufferToFill.buffer->copyFrom(0, bufferToFill.startSample, &m_sourceAudio[m_currentSample], numSamples);

        m_currentSample += numSamples;
    }

private:
    const float* m_sourceAudio;
    int m_numSamples;
    int m_currentSample = 0;
};

///////////////////////////////////////////////////////////////////////////////

void SamplerateConverter::convert(double samplerateIn, double samplerateOut, const float* audioIn[2], float* audioOut[2], uint32_t audioInLength, uint32_t audioOutLengthMax, uint32_t& audioOutLengthActual)
{
	DEBUG_ASSERT(samplerateOut > 0.0);

    const double samplerateRatioOutToIn = samplerateOut / samplerateIn;
	const double samplesInPerOutputSample = samplerateIn / samplerateOut;

	audioOutLengthActual = static_cast<uint32_t>(std::ceil(audioInLength * samplerateRatioOutToIn));

	if (audioOutLengthActual > audioOutLengthMax)
		audioOutLengthActual = audioOutLengthMax;

	for (int ch=0; ch<2; ch++)
	{
		DEBUG_ASSERT(audioIn[ch] != nullptr);
		DEBUG_ASSERT(audioOut[ch] != nullptr);

		ResamplingAudioSource sourceAudio(audioIn[ch], audioInLength);
    	juce::ResamplingAudioSource resampler(&sourceAudio, false, 1);

    	resampler.setResamplingRatio(samplesInPerOutputSample);
    	resampler.prepareToPlay(audioInLength, samplerateOut);

		juce::AudioBuffer<float> resampledBuffer(&audioOut[ch], 1, audioOutLengthActual);
    	juce::AudioSourceChannelInfo resampledInfo(&resampledBuffer, 0, resampledBuffer.getNumSamples());

    	resampler.getNextAudioBlock(resampledInfo);
	}
}

///////////////////////////////////////////////////////////////////////////////
