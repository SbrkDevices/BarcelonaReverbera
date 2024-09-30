#include "BarcelonaReverberaPluginProcessor.h"
#include "BarcelonaReverberaPluginEditor.h"

///////////////////////////////////////////////////////////////////////////////

#define PARAMS_VERSION          (1)

///////////////////////////////////////////////////////////////////////////////

BarcelonaReverberaAudioProcessor::BarcelonaReverberaAudioProcessor(void)
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    )
    , m_params(*this, nullptr, "BarcelonaReverberaParams",
        {
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"decayState", PARAMS_VERSION}, "Decay", 0.0f, 1.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"colorState", PARAMS_VERSION}, "Color", -1.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"dryWetState", PARAMS_VERSION}, "Dry/Wet", -1.0f, 1.0f, 0.0f),
            std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"irIndexState", PARAMS_VERSION}, "IR Index", 1, ConvolutionReverb::getIrCount(), 1)
        }
    )
{
    m_decayParam = m_params.getRawParameterValue("decayState");
    m_colorParam = m_params.getRawParameterValue("colorState");
    m_dryWetParam = m_params.getRawParameterValue("dryWetState");
    m_irIndexParam = m_params.getRawParameterValue("irIndexState");
}

BarcelonaReverberaAudioProcessor::~BarcelonaReverberaAudioProcessor(void)
{
}

///////////////////////////////////////////////////////////////////////////////

void BarcelonaReverberaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_convolutionReverb.init();
}

void BarcelonaReverberaAudioProcessor::releaseResources(void)
{
    m_convolutionReverb.exit();
}

bool BarcelonaReverberaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void BarcelonaReverberaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const float decayControl = (m_decayParam == nullptr) ? 0.5f : static_cast<float>(*m_decayParam);
    const float colorControl = (m_colorParam == nullptr) ? 0.5f : static_cast<float>(*m_colorParam);
    const float dryWetControl = (m_dryWetParam == nullptr) ? 0.5f : static_cast<float>(*m_dryWetParam);
    const int irIndex = (m_irIndexParam == nullptr) ? 0 : (static_cast<int>(*m_irIndexParam) - 1);

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int blockSize = buffer.getNumSamples();
    const double samplerate = getSampleRate();

    // in case we have more outputs than inputs, clear those outputs:
    for (int i=numInputChannels; i<numOutputChannels; i++)
        buffer.clear(i, 0, blockSize);
    
    // in case there are more than 2 channels both on input and output, perform bypass:
    if ((numInputChannels > 2) && (numOutputChannels > 2))
    {
        for (int i=2; i<juce::jmin(numInputChannels, numOutputChannels); i++)
            std::memcpy(buffer.getWritePointer(i), buffer.getReadPointer(i), blockSize*sizeof(float));
    }

    if ((numInputChannels == 0) || (numOutputChannels == 0))
        return;

    const float* inputData[2];
    inputData[0] = buffer.getReadPointer(0);
    inputData[1] = (numInputChannels > 1) ? buffer.getReadPointer(1) : buffer.getReadPointer(0);

    float* outputData[2];
    outputData[0] = m_audioOutputDataBuffer[0];
    outputData[1] = m_audioOutputDataBuffer[1];

    m_convolutionReverb.process(inputData, outputData, (numOutputChannels > 1), samplerate, blockSize, decayControl, colorControl, dryWetControl, irIndex);

    std::memcpy(buffer.getWritePointer(0), outputData[0], blockSize*sizeof(float));
    if (numOutputChannels > 1)
        std::memcpy(buffer.getWritePointer(1), outputData[1], blockSize*sizeof(float));
}

///////////////////////////////////////////////////////////////////////////////

juce::AudioProcessorEditor* BarcelonaReverberaAudioProcessor::createEditor(void)
{
    return new BarcelonaReverberaAudioProcessorEditor(*this, m_params, m_convolutionReverb);
}


///////////////////////////////////////////////////////////////////////////////

void BarcelonaReverberaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = m_params.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BarcelonaReverberaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(m_params.state.getType()))
            m_params.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

///////////////////////////////////////////////////////////////////////////////

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(void)
{
    return new BarcelonaReverberaAudioProcessor();
}

///////////////////////////////////////////////////////////////////////////////
