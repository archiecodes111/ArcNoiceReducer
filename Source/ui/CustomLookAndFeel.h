#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    CustomLookAndFeel implements the ARC DSP Mastering Hardware aesthetic:
    - Precision light brushed aluminum faceplate (#E8ECEF / #F0F2F5)
    - Dark carbon top bar (#1A1D24)
    - ARC Electric Cyan & Vermilion accents (#00B4D8 / #E63946)
    - High-contrast button styling with guaranteed text legibility (100% white on dark or dark on light)
    - Matte black cylindrical dials with soft drop-shadows & LED jewels
*/
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override = default;

    // Palette tokens
    static const juce::Colour chassisBg;       // #ECEEF1 (Warm brushed aluminum)
    static const juce::Colour chassisBorder;   // #CCD1D9
    static const juce::Colour cardBg;          // #F5F7FA (Elevated control module)
    static const juce::Colour cardBorder;      // #D0D5DD
    static const juce::Colour topBarBg;        // #1E222B (Dark carbon header)
    static const juce::Colour displayBg;       // #F8F9FA (LCD display surface)
    static const juce::Colour displayGrid;     // #E4DBDB (Delicate grid lines)
    static const juce::Colour displayBorder;   // #B8C0CC
    static const juce::Colour textDark;        // #181C24 (Deep charcoal text)
    static const juce::Colour textSecondary;   // #5C6474 (Secondary labels)
    static const juce::Colour textMuted;       // #8A92A0
    static const juce::Colour textWhite;       // #FFFFFF (High-contrast button text)
    static const juce::Colour arcCyan;         // #0096C7 (ARC primary blue/cyan)
    static const juce::Colour arcRed;          // #D90429 (ARC vermilion accent)
    static const juce::Colour arcGreen;        // #10B981 (Active play / success)
    static const juce::Colour arcAmber;        // #F59E0B (Noise profiling amber)
    static const juce::Colour btnDark;         // #242933 (Button background)
    static const juce::Colour btnDarkHover;    // #323946
    static const juce::Colour btnDarkDown;     // #181C23

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g,
                        juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g,
                      int width, int height,
                      bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                           bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;
};
