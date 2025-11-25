#pragma once

#include "DistrhoUI.hpp"
#include "NAMPlugin.hpp"
#include <cmath>

START_NAMESPACE_DISTRHO

// Simple knob widget structure
struct Knob {
    float x, y, size;
    float min, max, value;
    const char* label;
    uint32_t paramIndex;
    bool dragging;
    int dragStartY;
    float dragStartValue;
    bool hovered;

    Knob(float x_, float y_, float size_, float min_, float max_, float value_,
         const char* label_, uint32_t paramIndex_)
        : x(x_), y(y_), size(size_), min(min_), max(max_), value(value_),
          label(label_), paramIndex(paramIndex_), dragging(false),
          dragStartY(0), dragStartValue(0.0f), hovered(false) {}

    bool contains(float mx, float my) const {
        float dx = mx - x;
        float dy = my - y;
        return (dx*dx + dy*dy) <= (size*size/4);
    }

    float getNormalizedValue() const {
        return (value - min) / (max - min);
    }

    void setNormalizedValue(float norm) {
        value = min + norm * (max - min);
        value = std::max(min, std::min(max, value));
    }
};

// Simple toggle button structure
struct ToggleButton {
    float x, y, width, height;
    bool value;
    const char* label;
    uint32_t paramIndex;
    bool hovered;

    ToggleButton(float x_, float y_, float width_, float height_,
                 bool value_, const char* label_, uint32_t paramIndex_)
        : x(x_), y(y_), width(width_), height(height_), value(value_),
          label(label_), paramIndex(paramIndex_), hovered(false) {}

    bool contains(float mx, float my) const {
        return mx >= x && mx <= x + width &&
               my >= y && my <= y + height;
    }
};

// Simple button structure
struct Button {
    float x, y, width, height;
    const char* label;
    bool hovered;

    Button(float x_, float y_, float width_, float height_, const char* label_)
        : x(x_), y(y_), width(width_), height(height_),
          label(label_), hovered(false) {}

    bool contains(float mx, float my) const {
        return mx >= x && mx <= x + width &&
               my >= y && my <= y + height;
    }
};

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
    static constexpr int kUIWidth = 600;
    static constexpr int kUIHeight = 400;
    static constexpr int kPadding = 20;

    // Widgets
    Knob inputKnob;
    Knob outputKnob;
    ToggleButton enabledButton;
    ToggleButton bypassButton;
    Button loadButton;

    // Helper methods
    void drawBackground();
    void drawKnob(const Knob& knob);
    void drawToggleButton(const ToggleButton& button);
    void drawButton(const Button& button);
    void drawModelInfo();

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NAMUI)
};

END_NAMESPACE_DISTRHO
