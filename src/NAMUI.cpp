#include "NAMUI.hpp"
#include <string>
#include <cstring>

START_NAMESPACE_DISTRHO

NAMUI::NAMUI()
    : UI(kUIWidth, kUIHeight),
      fInputLevel(0.0f),
      fOutputLevel(0.0f),
      fEnabled(1.0f),
      fHardBypass(0.0f),
      loadButtonHovered(false)
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
        break;
    case kParameterOutputLevel:
        fOutputLevel = value;
        break;
    case kParameterEnabled:
        fEnabled = value;
        break;
    case kParameterHardBypass:
        fHardBypass = value;
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
    drawControls();
    drawModelInfo();
}

bool NAMUI::onMouse(const MouseEvent& ev)
{
    if (ev.press && ev.button == 1) {
        if (isMouseOverLoadButton(ev.pos.getX(), ev.pos.getY())) {
            // Request file dialog for the model path state
            // DPF will show a file dialog because we set kStateIsFilenamePath
            requestStateFile(kStateKeyModelPath);
            return true;
        }
    }
    return false;
}

bool NAMUI::onMotion(const MotionEvent& ev)
{
    bool newHovered = isMouseOverLoadButton(ev.pos.getX(), ev.pos.getY());
    if (newHovered != loadButtonHovered) {
        loadButtonHovered = newHovered;
        repaint();
    }
    return false;
}

void NAMUI::drawBackground()
{
    const float width = getWidth();
    const float height = getHeight();

    // Dark background
    beginPath();
    fillColor(30, 30, 35);
    rect(0, 0, width, height);
    fill();

    // Title bar
    beginPath();
    fillColor(45, 45, 50);
    rect(0, 0, width, 60);
    fill();

    // Title text
    fontSize(24);
    fillColor(220, 220, 220);
    textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
    text(width / 2, 30, "Neural Amp Modeler", nullptr);
}

void NAMUI::drawControls()
{
    const float width = getWidth();
    const float buttonX = (width - kButtonWidth) / 2;
    const float buttonY = 100;

    // Load Model Button
    beginPath();
    if (loadButtonHovered) {
        fillColor(90, 120, 200);
    } else {
        fillColor(70, 100, 180);
    }
    roundedRect(buttonX, buttonY, kButtonWidth, kButtonHeight, 4);
    fill();

    // Button text
    fontSize(16);
    fillColor(255, 255, 255);
    textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
    text(buttonX + kButtonWidth / 2, buttonY + kButtonHeight / 2, "Load Model", nullptr);

    // Parameter labels and values
    const float paramY = 180;
    const float paramSpacing = 30;

    fontSize(14);
    fillColor(200, 200, 200);
    textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

    char buf[32];

    // Input Level
    text(kPadding, paramY, "Input Level:", nullptr);
    std::snprintf(buf, sizeof(buf), "%.1f dB", fInputLevel);
    textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
    text(width - kPadding, paramY, buf, nullptr);

    // Output Level
    textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
    text(kPadding, paramY + paramSpacing, "Output Level:", nullptr);
    std::snprintf(buf, sizeof(buf), "%.1f dB", fOutputLevel);
    textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
    text(width - kPadding, paramY + paramSpacing, buf, nullptr);

    // Enabled
    textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
    text(kPadding, paramY + paramSpacing * 2, "Enabled:", nullptr);
    textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
    text(width - kPadding, paramY + paramSpacing * 2, fEnabled >= 0.5f ? "Yes" : "No", nullptr);

    // Hard Bypass
    textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
    text(kPadding, paramY + paramSpacing * 3, "Hard Bypass:", nullptr);
    textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
    text(width - kPadding, paramY + paramSpacing * 3, fHardBypass >= 0.5f ? "Yes" : "No", nullptr);
}

void NAMUI::drawModelInfo()
{
    const float width = getWidth();
    const float infoY = getHeight() - 40;

    fontSize(12);
    fillColor(150, 150, 150);
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
        text(width / 2, infoY, "No model loaded", nullptr);
    }
}

bool NAMUI::isMouseOverLoadButton(int x, int y) const
{
    const float width = getWidth();
    const float buttonX = (width - kButtonWidth) / 2;
    const float buttonY = 100;

    return x >= buttonX && x <= buttonX + kButtonWidth &&
           y >= buttonY && y <= buttonY + kButtonHeight;
}

UI* createUI()
{
    return new NAMUI();
}

END_NAMESPACE_DISTRHO
