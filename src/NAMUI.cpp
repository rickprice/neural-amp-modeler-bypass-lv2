#include "NAMUI.hpp"
#include <string>
#include <cstring>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

START_NAMESPACE_DISTRHO

NAMUI::NAMUI()
    : UI(kUIWidth, kUIHeight),
      fInputLevel(0.0f),
      fOutputLevel(0.0f),
      fEnabled(1.0f),
      fHardBypass(0.0f),
      inputKnob(150, 150, 80, -20.0f, 20.0f, 0.0f, "Input", kParameterInputLevel),
      outputKnob(450, 150, 80, -20.0f, 20.0f, 0.0f, "Output", kParameterOutputLevel),
      enabledButton(120, 270, 120, 35, true, "Enabled", kParameterEnabled),
      bypassButton(360, 270, 120, 35, false, "Hard Bypass", kParameterHardBypass),
      loadButton(220, 320, 160, 40, "Load Model")
{
    setGeometryConstraints(kUIWidth, kUIHeight, true);
}

NAMUI::~NAMUI()
{
}

void NAMUI::parameterChanged(uint32_t index, float value)
{
    switch (index) {
    case kParameterInputLevel:
        fInputLevel = value;
        inputKnob.value = value;
        break;
    case kParameterOutputLevel:
        fOutputLevel = value;
        outputKnob.value = value;
        break;
    case kParameterEnabled:
        fEnabled = value;
        enabledButton.value = (value >= 0.5f);
        break;
    case kParameterHardBypass:
        fHardBypass = value;
        bypassButton.value = (value >= 0.5f);
        break;
    }
    repaint();
}

void NAMUI::stateChanged(const char* key, const char* value)
{
    if (std::strcmp(key, kStateKeyModelPath) == 0) {
        modelPath = value ? value : "";
        repaint();
    }
}

void NAMUI::onNanoDisplay()
{
    drawBackground();
    drawKnob(inputKnob);
    drawKnob(outputKnob);
    drawToggleButton(enabledButton);
    drawToggleButton(bypassButton);
    drawButton(loadButton);
    drawModelInfo();
}

bool NAMUI::onMouse(const MouseEvent& ev)
{
    const float mx = ev.pos.getX();
    const float my = ev.pos.getY();

    if (ev.press && ev.button == 1) {
        // Check knobs
        if (inputKnob.contains(mx, my)) {
            inputKnob.dragging = true;
            inputKnob.dragStartY = my;
            inputKnob.dragStartValue = inputKnob.value;
            return true;
        }
        if (outputKnob.contains(mx, my)) {
            outputKnob.dragging = true;
            outputKnob.dragStartY = my;
            outputKnob.dragStartValue = outputKnob.value;
            return true;
        }

        // Check toggle buttons
        if (enabledButton.contains(mx, my)) {
            enabledButton.value = !enabledButton.value;
            setParameterValue(kParameterEnabled, enabledButton.value ? 1.0f : 0.0f);
            repaint();
            return true;
        }
        if (bypassButton.contains(mx, my)) {
            bypassButton.value = !bypassButton.value;
            setParameterValue(kParameterHardBypass, bypassButton.value ? 1.0f : 0.0f);
            repaint();
            return true;
        }

        // Check load button
        if (loadButton.contains(mx, my)) {
            requestStateFile(kStateKeyModelPath);
            return true;
        }
    } else if (!ev.press && ev.button == 1) {
        // Release knobs
        if (inputKnob.dragging) {
            inputKnob.dragging = false;
            return true;
        }
        if (outputKnob.dragging) {
            outputKnob.dragging = false;
            return true;
        }
    }

    return false;
}

bool NAMUI::onMotion(const MotionEvent& ev)
{
    const float mx = ev.pos.getX();
    const float my = ev.pos.getY();
    bool needsRepaint = false;

    // Handle knob dragging
    if (inputKnob.dragging) {
        float delta = (inputKnob.dragStartY - my) * 0.5f;
        float newValue = inputKnob.dragStartValue + delta;
        newValue = std::max(inputKnob.min, std::min(inputKnob.max, newValue));
        if (newValue != inputKnob.value) {
            inputKnob.value = newValue;
            setParameterValue(kParameterInputLevel, newValue);
            needsRepaint = true;
        }
    }
    if (outputKnob.dragging) {
        float delta = (outputKnob.dragStartY - my) * 0.5f;
        float newValue = outputKnob.dragStartValue + delta;
        newValue = std::max(outputKnob.min, std::min(outputKnob.max, newValue));
        if (newValue != outputKnob.value) {
            outputKnob.value = newValue;
            setParameterValue(kParameterOutputLevel, newValue);
            needsRepaint = true;
        }
    }

    // Update hover states
    bool inputHovered = inputKnob.contains(mx, my);
    bool outputHovered = outputKnob.contains(mx, my);
    bool enabledHovered = enabledButton.contains(mx, my);
    bool bypassHovered = bypassButton.contains(mx, my);
    bool loadHovered = loadButton.contains(mx, my);

    if (inputKnob.hovered != inputHovered) {
        inputKnob.hovered = inputHovered;
        needsRepaint = true;
    }
    if (outputKnob.hovered != outputHovered) {
        outputKnob.hovered = outputHovered;
        needsRepaint = true;
    }
    if (enabledButton.hovered != enabledHovered) {
        enabledButton.hovered = enabledHovered;
        needsRepaint = true;
    }
    if (bypassButton.hovered != bypassHovered) {
        bypassButton.hovered = bypassHovered;
        needsRepaint = true;
    }
    if (loadButton.hovered != loadHovered) {
        loadButton.hovered = loadHovered;
        needsRepaint = true;
    }

    if (needsRepaint) {
        repaint();
    }

    return false;
}

void NAMUI::drawBackground()
{
    const float width = getWidth();
    const float height = getHeight();

    // Main background gradient
    beginPath();
    Paint bg = linearGradient(0, 0, 0, height,
                              Color(35, 35, 40, 255),
                              Color(25, 25, 30, 255));
    fillPaint(bg);
    rect(0, 0, width, height);
    fill();

    // Title bar with gradient
    beginPath();
    Paint titleBg = linearGradient(0, 0, 0, 70,
                                   Color(50, 50, 60, 255),
                                   Color(40, 40, 50, 255));
    fillPaint(titleBg);
    rect(0, 0, width, 70);
    fill();

    // Title bar bottom edge
    beginPath();
    strokeColor(60, 60, 70, 255);
    strokeWidth(1.0f);
    moveTo(0, 70);
    lineTo(width, 70);
    stroke();

    // Title text
    fontSize(28);
    fillColor(220, 220, 230);
    textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
    text(width / 2, 35, "Neural Amp Modeler", nullptr);
}

void NAMUI::drawKnob(const Knob& knob)
{
    const float radius = knob.size / 2;
    const float norm = knob.getNormalizedValue();

    // Calculate angle (-140 to +140 degrees, with 0 at top)
    const float startAngle = -140.0f * M_PI / 180.0f;
    const float endAngle = 140.0f * M_PI / 180.0f;
    const float angle = startAngle + norm * (endAngle - startAngle);

    // Draw shadow
    beginPath();
    circle(knob.x + 2, knob.y + 2, radius);
    fillColor(0, 0, 0, 60);
    fill();

    // Draw knob background
    beginPath();
    circle(knob.x, knob.y, radius);
    Paint knobBg = radialGradient(knob.x - radius * 0.3f, knob.y - radius * 0.3f,
                                  radius * 0.5f, radius * 1.2f,
                                  Color(knob.hovered ? 75 : 65, knob.hovered ? 75 : 65, knob.hovered ? 85 : 75, 255),
                                  Color(35, 35, 45, 255));
    fillPaint(knobBg);
    fill();

    // Draw knob outline
    beginPath();
    circle(knob.x, knob.y, radius);
    strokeColor(80, 80, 90, 255);
    strokeWidth(1.5f);
    stroke();

    // Draw value arc
    beginPath();
    arc(knob.x, knob.y, radius - 4, -M_PI * 0.5f + startAngle, -M_PI * 0.5f + angle, CW);
    strokeColor(90, 140, 220, 255);
    strokeWidth(3.0f);
    stroke();

    // Draw center indicator line
    const float indicatorLength = radius * 0.6f;
    const float indicatorX = knob.x + std::cos(angle - M_PI * 0.5f) * indicatorLength;
    const float indicatorY = knob.y + std::sin(angle - M_PI * 0.5f) * indicatorLength;

    beginPath();
    moveTo(knob.x, knob.y);
    lineTo(indicatorX, indicatorY);
    strokeColor(200, 200, 210, 255);
    strokeWidth(2.5f);
    stroke();

    // Draw center dot
    beginPath();
    circle(knob.x, knob.y, 3);
    fillColor(90, 140, 220, 255);
    fill();

    // Draw label
    fontSize(14);
    fillColor(200, 200, 210);
    textAlign(ALIGN_CENTER | ALIGN_TOP);
    text(knob.x, knob.y + radius + 8, knob.label, nullptr);

    // Draw value
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f dB", knob.value);
    fontSize(12);
    fillColor(150, 150, 160);
    textAlign(ALIGN_CENTER | ALIGN_TOP);
    text(knob.x, knob.y + radius + 26, buf, nullptr);
}

void NAMUI::drawToggleButton(const ToggleButton& button)
{
    // Draw button background
    beginPath();
    roundedRect(button.x, button.y, button.width, button.height, 4);

    if (button.value) {
        if (button.hovered) {
            fillColor(100, 160, 240, 255);
        } else {
            fillColor(80, 140, 220, 255);
        }
    } else {
        if (button.hovered) {
            fillColor(60, 60, 70, 255);
        } else {
            fillColor(50, 50, 60, 255);
        }
    }
    fill();

    // Draw button outline
    beginPath();
    roundedRect(button.x, button.y, button.width, button.height, 4);
    if (button.value) {
        strokeColor(110, 170, 250, 255);
    } else {
        strokeColor(70, 70, 80, 255);
    }
    strokeWidth(1.5f);
    stroke();

    // Draw text
    fontSize(14);
    if (button.value) {
        fillColor(255, 255, 255);
    } else {
        fillColor(150, 150, 160);
    }
    textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
    text(button.x + button.width / 2, button.y + button.height / 2, button.label, nullptr);
}

void NAMUI::drawButton(const Button& button)
{
    // Draw button background
    beginPath();
    roundedRect(button.x, button.y, button.width, button.height, 5);

    if (button.hovered) {
        Paint btnBg = linearGradient(button.x, button.y, button.x, button.y + button.height,
                                     Color(90, 140, 220, 255),
                                     Color(70, 120, 200, 255));
        fillPaint(btnBg);
    } else {
        Paint btnBg = linearGradient(button.x, button.y, button.x, button.y + button.height,
                                     Color(70, 120, 200, 255),
                                     Color(60, 100, 180, 255));
        fillPaint(btnBg);
    }
    fill();

    // Draw button outline
    beginPath();
    roundedRect(button.x, button.y, button.width, button.height, 5);
    strokeColor(90, 140, 220, 255);
    strokeWidth(2.0f);
    stroke();

    // Draw text
    fontSize(16);
    fillColor(255, 255, 255);
    textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
    text(button.x + button.width / 2, button.y + button.height / 2, button.label, nullptr);
}

void NAMUI::drawModelInfo()
{
    const float width = getWidth();
    const float infoY = getHeight() - 30;

    // Draw info panel background
    beginPath();
    rect(kPadding, infoY - 25, width - 2 * kPadding, 40);
    fillColor(40, 40, 50, 200);
    fill();

    // Draw info panel outline
    beginPath();
    rect(kPadding, infoY - 25, width - 2 * kPadding, 40);
    strokeColor(60, 60, 70, 255);
    strokeWidth(1.0f);
    stroke();

    fontSize(11);
    fillColor(180, 180, 190);
    textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

    if (!modelPath.empty()) {
        // Extract filename from path
        size_t lastSlash = modelPath.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos)
            ? modelPath.substr(lastSlash + 1)
            : modelPath;

        std::string displayText = "Model: " + filename;
        text(width / 2, infoY, displayText.c_str(), nullptr);
    } else {
        fillColor(140, 140, 150);
        text(width / 2, infoY, "No model loaded - click 'Load Model' to select a .nam file", nullptr);
    }
}

UI* createUI()
{
    return new NAMUI();
}

END_NAMESPACE_DISTRHO
