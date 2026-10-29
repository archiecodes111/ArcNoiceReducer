#include "CustomLookAndFeel.h"
#include <cmath>

const juce::Colour CustomLookAndFeel::chassisBg       = juce::Colour(0xffeceef1);
const juce::Colour CustomLookAndFeel::chassisBorder   = juce::Colour(0xffccd1d9);
const juce::Colour CustomLookAndFeel::cardBg          = juce::Colour(0xfff5f7fa);
const juce::Colour CustomLookAndFeel::cardBorder      = juce::Colour(0xffd0d5dd);
const juce::Colour CustomLookAndFeel::topBarBg        = juce::Colour(0xff1e222b);
const juce::Colour CustomLookAndFeel::displayBg       = juce::Colour(0xfff8f9fa);
const juce::Colour CustomLookAndFeel::displayGrid     = juce::Colour(0xffe4dbdb);
const juce::Colour CustomLookAndFeel::displayBorder   = juce::Colour(0xffb8c0cc);
const juce::Colour CustomLookAndFeel::textDark        = juce::Colour(0xff181c24);
const juce::Colour CustomLookAndFeel::textSecondary   = juce::Colour(0xff5c6474);
const juce::Colour CustomLookAndFeel::textMuted       = juce::Colour(0xff8a92a0);
const juce::Colour CustomLookAndFeel::textWhite       = juce::Colour(0xffffffff);
const juce::Colour CustomLookAndFeel::arcCyan         = juce::Colour(0xff0096c7);
const juce::Colour CustomLookAndFeel::arcRed          = juce::Colour(0xffd90429);
const juce::Colour CustomLookAndFeel::arcGreen        = juce::Colour(0xff10b981);
const juce::Colour CustomLookAndFeel::arcAmber        = juce::Colour(0xfff59e0b);
const juce::Colour CustomLookAndFeel::btnDark         = juce::Colour(0xff242933);
const juce::Colour CustomLookAndFeel::btnDarkHover    = juce::Colour(0xff323946);
const juce::Colour CustomLookAndFeel::btnDarkDown     = juce::Colour(0xff181c23);

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, chassisBg);
    setColour(juce::Label::textColourId, textDark);
    setColour(juce::TextButton::textColourOffId, textWhite);
    setColour(juce::TextButton::textColourOnId, textWhite);
    setColour(juce::ComboBox::backgroundColourId, cardBg);
    setColour(juce::ComboBox::textColourId, textDark);
    setColour(juce::ComboBox::outlineColourId, cardBorder);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colours::white);
    setColour(juce::PopupMenu::textColourId, textDark);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffe6f3f8));
    setColour(juce::PopupMenu::highlightedTextColourId, arcCyan);
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPosProportional,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const auto fullBounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    const float ledDiameter = 5.0f;
    const float ledY = fullBounds.getY() + 2.0f;
    const float centreX = fullBounds.getCentreX();

    // 1. Draw Amber LED above the knob
    const bool isEnabled = slider.isEnabled();
    const bool isActive = (sliderPosProportional > 0.01f && isEnabled);

    if (isActive)
    {
        g.setColour(arcAmber.withAlpha(0.35f));
        g.fillEllipse(centreX - 4.5f, ledY - 2.0f, 9.0f, 9.0f);
        g.setColour(arcAmber);
        g.fillEllipse(centreX - (ledDiameter * 0.5f), ledY, ledDiameter, ledDiameter);
    }
    else
    {
        g.setColour(juce::Colour(0xff554830));
        g.fillEllipse(centreX - (ledDiameter * 0.5f), ledY, ledDiameter, ledDiameter);
        g.setColour(chassisBorder);
        g.drawEllipse(centreX - (ledDiameter * 0.5f), ledY, ledDiameter, ledDiameter, 0.7f);
    }

    // 2. Knob bounds
    const float knobTop = ledY + ledDiameter + 4.0f;
    const float knobAvailableH = fullBounds.getBottom() - knobTop - 2.0f;
    const float knobSize = juce::jmin(fullBounds.getWidth() - 8.0f, knobAvailableH);
    const auto knobRect = juce::Rectangle<float>(centreX - (knobSize * 0.5f), knobTop, knobSize, knobSize);
    const auto knobCentre = knobRect.getCentre();
    const float radius = knobSize * 0.5f;

    // 3. Multi-layer realistic drop shadow beneath knob
    for (int s = 4; s >= 1; --s)
    {
        const float shadowRadius = radius + (s * 1.5f);
        const float alpha = 0.04f * static_cast<float>(5 - s);
        g.setColour(juce::Colours::black.withAlpha(alpha));
        g.fillEllipse(knobCentre.x - shadowRadius, knobCentre.y - shadowRadius + (s * 1.8f),
                      shadowRadius * 2.0f, shadowRadius * 2.0f);
    }

    // 4. Matte Black Knob Cylinder Base
    juce::ColourGradient knobGrad(juce::Colour(0xff2d323b), knobCentre.x - radius * 0.4f, knobCentre.y - radius * 0.4f,
                                  juce::Colour(0xff121418), knobCentre.x + radius * 0.6f, knobCentre.y + radius * 0.6f, true);
    g.setGradientFill(knobGrad);
    g.fillEllipse(knobRect);

    // Subtle dark chassis bevel ring
    g.setColour(juce::Colour(0xff0d0f12));
    g.drawEllipse(knobRect, 1.2f);

    // Inner bevel highlight
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(knobRect.reduced(1.0f), 0.8f);

    // 5. White Indicator Notch Line
    const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const float angle = toAngle - juce::MathConstants<float>::halfPi;
    const float notchStart = radius * 0.25f;
    const float notchEnd = radius * 0.85f;

    const juce::Point<float> p1(knobCentre.x + std::cos(angle) * notchStart,
                                knobCentre.y + std::sin(angle) * notchStart);
    const juce::Point<float> p2(knobCentre.x + std::cos(angle) * notchEnd,
                                knobCentre.y + std::sin(angle) * notchEnd);

    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.drawLine(p1.x, p1.y, p2.x, p2.y, 2.0f);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                             juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const float corner = 4.0f;
    const auto btnId = button.getComponentID();

    // 1. Display Tabs ([ FREQUENCY ] / [ WAVEFORM ])
    if (btnId == "displayTab")
    {
        const bool active = button.getToggleState() || (backgroundColour != juce::Colours::transparentBlack && !backgroundColour.isTransparent());
        if (active)
        {
            g.setColour(arcCyan);
            g.fillRoundedRectangle(bounds, corner);

            // Subtle top highlight
            g.setColour(juce::Colours::white.withAlpha(0.25f));
            g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);
        }
        else
        {
            if (shouldDrawButtonAsHighlighted)
            {
                g.setColour(juce::Colour(0xffe2e8f0));
                g.fillRoundedRectangle(bounds, corner);
            }
        }
        return;
    }

    // 2. Green Bypass Rocker Button (Hardware toggle style)
    if (btnId == "bypassRocker")
    {
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();

        const float pinY = h - 6.0f;
        g.setColour(juce::Colour(0xff333a42));
        for (float px = bounds.getX() + 4.0f; px < bounds.getRight() - 6.0f; px += 4.0f)
            g.fillRect(px, pinY, 2.0f, 6.0f);

        const auto capRect = juce::Rectangle<float>(bounds.getX(), bounds.getY(), w, h - 5.0f);
        const bool active = button.getToggleState();

        const auto capTop = active ? juce::Colour(0xff1e824c) : juce::Colour(0xff27ae60);
        const auto capBot = active ? juce::Colour(0xff145a32) : juce::Colour(0xff1e8449);

        juce::ColourGradient capGrad(capTop, capRect.getCentreX(), capRect.getY(),
                                     capBot, capRect.getCentreX(), capRect.getBottom(), false);
        g.setGradientFill(capGrad);
        g.fillRoundedRectangle(capRect, 3.0f);

        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(capRect.getY() + 1.0f), capRect.getX() + 2.0f, capRect.getRight() - 2.0f);

        g.setColour(juce::Colour(0xff0e381f));
        g.drawRoundedRectangle(capRect, 3.0f, 1.0f);
        return;
    }

    // 3. High-Contrast Studio Buttons
    auto baseCol = backgroundColour.isTransparent() ? btnDark : backgroundColour;

    if (button.getToggleState())
    {
        if (backgroundColour == btnDark || backgroundColour.isTransparent())
            baseCol = baseCol.interpolatedWith(arcCyan, 0.35f);
        else
            baseCol = baseCol.brighter(0.15f);
    }

    if (shouldDrawButtonAsDown)
        baseCol = baseCol.darker(0.20f);
    else if (shouldDrawButtonAsHighlighted)
        baseCol = baseCol.brighter(0.15f);

    // Subtle drop shadow
    g.setColour(juce::Colours::black.withAlpha(0.15f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), corner);

    // Gradient fill
    juce::ColourGradient btnGrad(baseCol.brighter(0.08f), bounds.getX(), bounds.getY(),
                                 baseCol.darker(0.08f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(btnGrad);
    g.fillRoundedRectangle(bounds, corner);

    // Border
    juce::Colour borderCol = shouldDrawButtonAsHighlighted ? juce::Colour(0xff64748b) : juce::Colour(0xff3d4452);
    if (button.getToggleState())
    {
        if (backgroundColour != btnDark && !backgroundColour.isTransparent())
            borderCol = backgroundColour.brighter(0.45f);
        else
            borderCol = arcCyan;
    }
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, corner, button.getToggleState() ? 1.5f : 1.0f);

    // Top highlight bevel for tactile depth
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);
}

void CustomLookAndFeel::drawButtonText(juce::Graphics& g,
                                       juce::TextButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsDown);

    const auto bounds = button.getLocalBounds().toFloat();
    const auto btnId = button.getComponentID();

    // 1. Display Tabs
    if (btnId == "displayTab")
    {
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        const bool active = button.getToggleState() || (button.findColour(juce::TextButton::buttonColourId) != juce::Colours::transparentBlack && !button.findColour(juce::TextButton::buttonColourId).isTransparent());
        if (active)
            g.setColour(textWhite);
        else
            g.setColour(shouldDrawButtonAsHighlighted ? textDark : textSecondary);

        g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
        return;
    }

    // 2. Bypass Rocker
    if (btnId == "bypassRocker")
    {
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(textWhite);
        g.drawText(button.getButtonText(), bounds.withTrimmedBottom(6.0f), juce::Justification::centred);
        return;
    }

    // 3. All studio buttons: Guaranteed 100% Crisp White Bold Text (#FFFFFF)
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(textWhite);
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g,
                                     int width, int height,
                                     bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH);

    const auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
    g.setColour(cardBg);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(cardBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Vector down arrow
    const float arrowX = static_cast<float>(width - 16);
    const float arrowY = static_cast<float>(height) * 0.5f - 2.0f;

    juce::Path arrow;
    arrow.startNewSubPath(arrowX, arrowY);
    arrow.lineTo(arrowX + 4.0f, arrowY + 4.0f);
    arrow.lineTo(arrowX + 8.0f, arrowY);

    g.setColour(box.isEnabled() ? textDark : textMuted);
    g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(juce::Colour(0xfffafbfc));
    g.fillRoundedRectangle(bounds, 5.0f);

    g.setColour(cardBorder);
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                         bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                                         const juce::Drawable* icon, const juce::Colour* textColour)
{
    juce::ignoreUnused(isSeparator, hasSubMenu, shortcutKeyText, icon, textColour);

    if (isHighlighted && isActive)
    {
        g.setColour(juce::Colour(0xffe6f3f8));
        g.fillRect(area);

        g.setColour(arcCyan);
        g.fillRect(area.getX(), area.getY(), 3, area.getHeight());
    }

    g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
    g.setColour(isActive ? (isHighlighted ? arcCyan : textDark) : textMuted);

    const int textOffset = isTicked ? 24 : 12;
    g.drawText(text, area.getX() + textOffset, area.getY(), area.getWidth() - textOffset, area.getHeight(),
               juce::Justification::centredLeft);

    if (isTicked)
    {
        const float cy = static_cast<float>(area.getCentreY());
        const float cx = static_cast<float>(area.getX() + 10);

        juce::Path check;
        check.startNewSubPath(cx - 3.0f, cy);
        check.lineTo(cx, cy + 3.0f);
        check.lineTo(cx + 4.0f, cy - 3.0f);

        g.setColour(arcCyan);
        g.strokePath(check, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}
