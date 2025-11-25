#pragma once

#include "DistrhoUI.hpp"
#include "NAMPlugin.hpp"

START_NAMESPACE_DISTRHO

class NAMUI : public UI
{
public:
    NAMUI();
    ~NAMUI() override;

protected:
    // DSP/Plugin Callbacks
    void parameterChanged(uint32_t index, float value) override;
    void stateChanged(const char* key, const char* value) override;

    // Widget Callbacks
    void onNanoDisplay() override;
    bool onMouse(const MouseEvent& ev) override;
    bool onMotion(const MotionEvent& ev) override;

private:
    // UI state
    float fInputLevel;
    float fOutputLevel;
    float fEnabled;
    float fHardBypass;
    std::string modelPath;

    // UI layout
    static constexpr int kUIWidth = 500;
    static constexpr int kUIHeight = 300;
    static constexpr int kButtonWidth = 150;
    static constexpr int kButtonHeight = 40;
    static constexpr int kPadding = 20;

    // Button state
    bool loadButtonHovered;

    // Helper methods
    void drawBackground();
    void drawControls();
    void drawModelInfo();
    bool isMouseOverLoadButton(int x, int y) const;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMUI)
};

END_NAMESPACE_DISTRHO
