#pragma once

#include <JuceHeader.h>
#include "BarcelonaReverberaPluginProcessor.h"
#include "ImageDescriptions.h"

///////////////////////////////////////////////////////////////////////////////

// our custom slider:

class BarcelonaReverberaSliderFillFromCenterLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
};

class BarcelonaReverberaSliderFillFromLeftLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
};

// out custom combobox:

class BarcelonaReverberaComboBoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BarcelonaReverberaComboBoxLookAndFeel(void);

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, const bool isSeparator, const bool isActive, const bool isHighlighted, const bool isTicked, const bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* const textColourToUse) override;

    juce::Font getComboBoxFont(juce::ComboBox& comboBox) override
    {
        return juce::FontOptions(juce::Typeface::createSystemTypefaceFor(
            BinaryData::NewsCycleRegular_ttf, BinaryData::NewsCycleRegular_ttfSize)).withHeight(30);
    }
};

///////////////////////////////////////////////////////////////////////////////

class BarcelonaReverberaAudioProcessorEditor
    : public juce::AudioProcessorEditor
{
public:
    BarcelonaReverberaAudioProcessorEditor(BarcelonaReverberaAudioProcessor& p, juce::AudioProcessorValueTreeState& vts, ConvolutionReverb& convolutionReverb);
    ~BarcelonaReverberaAudioProcessorEditor(void) override;

    void paint(juce::Graphics&) override;
    void resized(void) override;

private:
    BarcelonaReverberaSliderFillFromCenterLookAndFeel m_barcelonaReverberaSliderFillFromCenterLookAndFeel;
    BarcelonaReverberaSliderFillFromLeftLookAndFeel m_barcelonaReverberaSliderFillFromLeftLookAndFeel;
    BarcelonaReverberaComboBoxLookAndFeel m_barcelonaReverberaComboBoxLookAndFeel;

    char* m_irImagePtr = nullptr;
    int m_irImageSize = 0;

    juce::Label m_labelIrDescription;

    juce::Slider m_decaySlider;
    juce::Slider m_colorSlider;
    juce::Slider m_dryWetSlider;
    juce::ComboBox m_irIndexComboBox;
    juce::Label m_decayLabel;
    juce::Label m_colorLabel;
    juce::Label m_dryWetLabel;

    juce::AudioProcessorValueTreeState& m_valueTreeState;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_decaySliderAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_colorSliderAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> m_dryWetSliderAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> m_irIndexComboBoxAtt;

    ConvolutionReverb& m_convolutionReverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarcelonaReverberaAudioProcessorEditor)
};

///////////////////////////////////////////////////////////////////////////////
