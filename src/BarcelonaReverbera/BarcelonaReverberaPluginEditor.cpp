#include "BarcelonaReverberaPluginProcessor.h"
#include "BarcelonaReverberaPluginEditor.h"

///////////////////////////////////////////////////////////////////////////////

using namespace juce;

///////////////////////////////////////////////////////////////////////////////

// This function draws a rotary slider. It is very similar to the one found in juce_LookAndFeel_V4.cpp, with a few differences:
//   - starts filling the slider from the center
//   - background circle makes 360º
//   - thinner line

void BarcelonaReverberaSliderFillFromCenterLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider)
{
    auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour(Slider::rotarySliderFillColourId);

    auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);

    auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = jmin(5.76f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, 0.0f, 2.0f * MathConstants<float>::pi, true);

    g.setColour(outline);
    g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, (rotaryStartAngle+rotaryEndAngle)/2, toAngle, true);

        g.setColour(fill);
        g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    auto thumbWidth = lineW * 2.0f;
    Point<float> thumbPoint(bounds.getCentreX() + arcRadius * std::cos(toAngle - MathConstants<float>::halfPi),
                            bounds.getCentreY() + arcRadius * std::sin(toAngle - MathConstants<float>::halfPi));

    g.setColour(slider.findColour(Slider::thumbColourId));
    g.fillEllipse(Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}

// This function draws a rotary slider. It is very similar to the one found in juce_LookAndFeel_V4.cpp, with a few differences:
//   - background circle makes 360º
//   - thinner line

void BarcelonaReverberaSliderFillFromLeftLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider)
{
    auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour(Slider::rotarySliderFillColourId);

    auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);

    auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = jmin(5.76f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, 0.0f, 2.0f * MathConstants<float>::pi, true);

    g.setColour(outline);
    g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);

        g.setColour(fill);
        g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    auto thumbWidth = lineW * 2.0f;
    Point<float> thumbPoint(bounds.getCentreX() + arcRadius * std::cos(toAngle - MathConstants<float>::halfPi),
                            bounds.getCentreY() + arcRadius * std::sin(toAngle - MathConstants<float>::halfPi));

    g.setColour(slider.findColour(Slider::thumbColourId));
    g.fillEllipse(Rectangle<float> (thumbWidth, thumbWidth).withCentre(thumbPoint));
}

///////////////////////////////////////////////////////////////////////////////

// Set colors for pop-up menu:

BarcelonaReverberaComboBoxLookAndFeel::BarcelonaReverberaComboBoxLookAndFeel(void)
{
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(BCNRVRB_COLOR_BLACK));
    setColour(juce::PopupMenu::textColourId, juce::Colour(BCNRVRB_COLOR_WHITE));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(BCNRVRB_COLOR_PURPLE));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(BCNRVRB_COLOR_BLACK));
}

// This function draws a combo box. It is very similar to the one found in juce_LookAndFeel_V4.cpp, with a few differences:
//   - no corner radius

void BarcelonaReverberaComboBoxLookAndFeel::drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box)
{
    auto cornerSize = 0.0f;
    Rectangle<int> boxBounds(0, 0, width, height);

    g.setColour(box.findColour(ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

    g.setColour(box.findColour(ComboBox::outlineColourId));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

    Rectangle<int> arrowZone(width - 30, 0, 20, height);
    Path path;
    path.startNewSubPath((float) arrowZone.getX() + 3.0f, (float) arrowZone.getCentreY() - 2.0f);
    path.lineTo((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
    path.lineTo((float) arrowZone.getRight() - 3.0f, (float) arrowZone.getCentreY() - 2.0f);

    g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
    g.strokePath(path, PathStrokeType(2.0f));
}

// This function draws the pop-up menu of a combo box. It is very similar to the one found in juce_LookAndFeel_V4.cpp, with a few differences:
//   - text is justified to the center

void BarcelonaReverberaComboBoxLookAndFeel::drawPopupMenuItem(Graphics& g, const Rectangle<int>& area, const bool isSeparator, const bool isActive, const bool isHighlighted, const bool isTicked, const bool hasSubMenu, const String& text, const String& shortcutKeyText, const Drawable* icon, const Colour* const textColourToUse)
{
    if (isSeparator)
    {
        auto r = area.reduced(5, 0);
        r.removeFromTop(roundToInt(((float) r.getHeight() * 0.5f) - 0.5f));

        g.setColour(findColour(PopupMenu::textColourId).withAlpha(0.3f));
        g.fillRect(r.removeFromTop(1));
    }
    else
    {
        auto textColour = (textColourToUse == nullptr ? findColour(PopupMenu::textColourId) : *textColourToUse);

        auto r = area.reduced(1);

        if (isHighlighted && isActive)
        {
            g.setColour(findColour(PopupMenu::highlightedBackgroundColourId));
            g.fillRect(r);

            g.setColour(findColour(PopupMenu::highlightedTextColourId));
        }
        else
            g.setColour(textColour.withMultipliedAlpha(isActive ? 1.0f : 0.5f));

        r.reduce(jmin(5, area.getWidth() / 20), 0);

        auto font = getPopupMenuFont();

        auto maxFontHeight = (float) r.getHeight() / 1.3f;

        if (font.getHeight() > maxFontHeight)
            font.setHeight(maxFontHeight);

        g.setFont(font);

        auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).toFloat();

        if (icon != nullptr)
        {
            icon->drawWithin(g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
            r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));
        }
        else if (isTicked)
        {
            auto tick = getTickShape(1.0f);
            g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
        }

        if (hasSubMenu)
        {
            auto arrowH = 0.6f * getPopupMenuFont().getAscent();

            auto x = static_cast<float>(r.removeFromRight((int) arrowH).getX());
            auto halfH = static_cast<float>(r.getCentreY());

            Path path;
            path.startNewSubPath(x, halfH - arrowH * 0.5f);
            path.lineTo(x + arrowH * 0.6f, halfH);
            path.lineTo(x, halfH + arrowH * 0.5f);

            g.strokePath(path, PathStrokeType(2.0f));
        }

        r.removeFromRight(3);
        g.drawFittedText(text, r, Justification::centred, 1);

        if (shortcutKeyText.isNotEmpty())
        {
            auto f2 = font;
            f2.setHeight(f2.getHeight() * 0.75f);
            f2.setHorizontalScale(0.95f);
            g.setFont(f2);

            g.drawText(shortcutKeyText, r, Justification::centredRight, true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

BarcelonaReverberaAudioProcessorEditor::BarcelonaReverberaAudioProcessorEditor(BarcelonaReverberaAudioProcessor& p, AudioProcessorValueTreeState& vts, ConvolutionReverb& convolutionReverb) : AudioProcessorEditor(&p), m_decaySlider("Decay"), m_colorSlider("Color"), m_dryWetSlider("Dry/Wet"), m_irIndexComboBox("IR"), m_valueTreeState(vts), m_convolutionReverb(convolutionReverb)
{
    setSize(700, 600);

    m_decaySlider.setRange(0.0, 1.0, 1.0);
    m_colorSlider.setRange(-1.0, 1.0, 0.0);
    m_dryWetSlider.setRange(-1.0, 1.0, 0.0);

    for (int i=0; i<ConvolutionReverb::getIrCount(); i++)
        m_irIndexComboBox.addItem(m_convolutionReverb.getIrName(i), i + 1);

    m_irIndexComboBox.onChange = [this] ()
    {
        const int irIndex = m_irIndexComboBox.getSelectedId() - 1;
        m_irImagePtr = (char*) IrBuffers::getIrImgPtr(irIndex);
        m_irImageSize = IrBuffers::getIrImgSize(irIndex);

        m_labelIrDescription.setText(ImageDescriptions::getDescription(irIndex), juce::dontSendNotification);

        repaint();
    };
    
    m_decaySliderAtt.reset(new AudioProcessorValueTreeState::SliderAttachment(m_valueTreeState, "decayState", m_decaySlider));
    m_colorSliderAtt.reset(new AudioProcessorValueTreeState::SliderAttachment(m_valueTreeState, "colorState", m_colorSlider));
    m_dryWetSliderAtt.reset(new AudioProcessorValueTreeState::SliderAttachment(m_valueTreeState, "dryWetState", m_dryWetSlider));
    m_irIndexComboBoxAtt.reset(new AudioProcessorValueTreeState::ComboBoxAttachment(m_valueTreeState, "irIndexState", m_irIndexComboBox));

    for (int i=0; i<3; i++)
    {
        Slider* slider = (i == 0) ? &m_decaySlider : ((i == 1) ? &m_colorSlider : &m_dryWetSlider);

        if (slider == &m_decaySlider)
            slider->setLookAndFeel(&m_barcelonaReverberaSliderFillFromLeftLookAndFeel);
        else
            slider->setLookAndFeel(&m_barcelonaReverberaSliderFillFromCenterLookAndFeel);
    
        slider->setSliderStyle(Slider::RotaryVerticalDrag);
        slider->setTextBoxStyle(Slider::TextBoxBelow, false, 90, 20);
        slider->setColour(Slider::backgroundColourId, Colour(BCNRVRB_COLOR_GREY_LIGHT));
        slider->setColour(Slider::thumbColourId, Colour(BCNRVRB_COLOR_PURPLE));
        slider->setColour(Slider::trackColourId, Colour(BCNRVRB_COLOR_GREY_DARK));
        slider->setColour(Slider::rotarySliderFillColourId, Colour(BCNRVRB_COLOR_PURPLE));
        slider->setColour(Slider::rotarySliderOutlineColourId, Colour(BCNRVRB_COLOR_GREY_DARK));
        slider->setColour(Slider::textBoxTextColourId, Colour(BCNRVRB_COLOR_WHITE));
        slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    }

    m_irIndexComboBox.setLookAndFeel(&m_barcelonaReverberaComboBoxLookAndFeel);
    m_irIndexComboBox.setTooltip("Choose Impulse Response");
    m_irIndexComboBox.setJustificationType(Justification::centred);
    m_irIndexComboBox.setColour(ComboBox::backgroundColourId, Colour(BCNRVRB_COLOR_PURPLE));
    m_irIndexComboBox.setColour(ComboBox::textColourId, Colour(BCNRVRB_COLOR_BLACK));
    m_irIndexComboBox.setColour(ComboBox::outlineColourId, Colour(BCNRVRB_COLOR_PURPLE));
    m_irIndexComboBox.setColour(ComboBox::buttonColourId, Colour(BCNRVRB_COLOR_BLACK));
    m_irIndexComboBox.setColour(ComboBox::arrowColourId, Colour(BCNRVRB_COLOR_BLACK));
    m_irIndexComboBox.setColour(ComboBox::focusedOutlineColourId, Colour(BCNRVRB_COLOR_PURPLE));

    auto ourFont = Typeface::createSystemTypefaceFor(BinaryData::NewsCycleRegular_ttf, BinaryData::NewsCycleRegular_ttfSize);
    const float knobLabelFontWeight = 30.0f;
    m_decayLabel.setText("Decay", juce::dontSendNotification);
    m_decayLabel.setFont(FontOptions(ourFont).withHeight(knobLabelFontWeight));
    m_decayLabel.setJustificationType(Justification::centred);
    m_colorLabel.setText("Color", juce::dontSendNotification);
    m_colorLabel.setFont(FontOptions(ourFont).withHeight(knobLabelFontWeight));
    m_colorLabel.setJustificationType(Justification::centred);
    m_dryWetLabel.setText("Dry/Wet", juce::dontSendNotification);
    m_dryWetLabel.setFont(FontOptions(ourFont).withHeight(knobLabelFontWeight));
    m_dryWetLabel.setJustificationType(Justification::centred);
    
    m_labelIrDescription.setBounds(217.0, 304.0, 470.0, 100.0);
    m_labelIrDescription.setJustificationType(juce::Justification::topLeft);
    m_labelIrDescription.setFont(FontOptions(ourFont).withHeight(24.0f));
 
    addAndMakeVisible(m_decaySlider);
    addAndMakeVisible(m_colorSlider);
    addAndMakeVisible(m_dryWetSlider);
    addAndMakeVisible(m_irIndexComboBox);
    addAndMakeVisible(m_decayLabel);
    addAndMakeVisible(m_colorLabel);
    addAndMakeVisible(m_dryWetLabel);
    addAndMakeVisible(m_labelIrDescription);
}

BarcelonaReverberaAudioProcessorEditor::~BarcelonaReverberaAudioProcessorEditor(void)
{
}

///////////////////////////////////////////////////////////////////////////////

void BarcelonaReverberaAudioProcessorEditor::paint(Graphics& g)
{
    if (m_irImagePtr != nullptr)
        g.drawImageAt(ImageCache::getFromMemory(m_irImagePtr, m_irImageSize).rescaled(700, 408), 0, 0);
}

void BarcelonaReverberaAudioProcessorEditor::resized(void)
{
    const int comboBoxVerticalPos = 246;
    const int comboBoxWidth = 541;
    const int comboBoxHeight = 39;

    m_irIndexComboBox.setBounds((getWidth() - comboBoxWidth) / 2.0f, comboBoxVerticalPos, comboBoxWidth, comboBoxHeight);

    const int knobVerticalPos = 448;
    const int knobWidth = 110;
    const int knobHeight = 134;
    const int knobSidesMargin = 99;

    m_decaySlider.setBounds(knobSidesMargin, knobVerticalPos, knobWidth, knobHeight);
    m_colorSlider.setBounds(getWidth() / 2.0f - knobWidth / 2.0f, knobVerticalPos, knobWidth, knobHeight);
    m_dryWetSlider.setBounds(getWidth() - (knobSidesMargin + knobWidth), knobVerticalPos, knobWidth, knobHeight);

    const int knobLabelVerticalPos = 412;
    const int knobLabelHeight = 40;

    m_decayLabel.setBounds(knobSidesMargin, knobLabelVerticalPos, knobWidth, knobLabelHeight);
    m_colorLabel.setBounds(getWidth() / 2.0f - knobWidth / 2.0f, knobLabelVerticalPos, knobWidth, knobLabelHeight);
    m_dryWetLabel.setBounds(getWidth() - (knobSidesMargin + knobWidth), knobLabelVerticalPos, knobWidth, knobLabelHeight);
}

///////////////////////////////////////////////////////////////////////////////
