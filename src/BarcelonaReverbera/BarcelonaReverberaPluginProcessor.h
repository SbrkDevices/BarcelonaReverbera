#pragma once

#include <JuceHeader.h>
#include "ConvolutionReverb.h"

///////////////////////////////////////////////////////////////////////////////

class BarcelonaReverberaAudioProcessor  : public juce::AudioProcessor
{
public:
    BarcelonaReverberaAudioProcessor();
    ~BarcelonaReverberaAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources(void) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor(void) override;
    bool hasEditor(void) const override { return true; }

    const juce::String getName(void) const override { return JucePlugin_Name; }

    bool acceptsMidi(void) const override { return false; }
    bool producesMidi(void) const override { return false; }
    bool isMidiEffect(void) const override { return false; }
    double getTailLengthSeconds(void) const override { return BCNRVRB_MAX_IR_LEN_SECONDS; }

    int getNumPrograms(void) override { return 1; }
    int getCurrentProgram(void) override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    juce::AudioProcessorValueTreeState m_params;
    std::atomic<float>* m_decayParam = nullptr;
    std::atomic<float>* m_colorParam = nullptr;
    std::atomic<float>* m_dryWetParam = nullptr;
    std::atomic<float>* m_irIndexParam  = nullptr;

    ConvolutionReverb m_convolutionReverb;

    alignas(16) float m_audioOutputDataBuffer[2][BCNRVRB_MAX_BLOCK_SIZE] = {}; // needed because process' audio input & output buffers might be the same

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarcelonaReverberaAudioProcessor)
};

///////////////////////////////////////////////////////////////////////////////
