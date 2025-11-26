#include "NAMPlugin.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

START_NAMESPACE_DISTRHO

NAMPlugin::NAMPlugin()
    : Plugin(kParameterCount, 0, 1), // parameters, programs, states
      fInputLevel(0.0f),
      fOutputLevel(0.0f),
      fEnabled(1.0f),  // Default: enabled (active)
      fHardBypass(0.0f),
      currentModel(nullptr),
      sampleRate(getSampleRate()),
      prevDCInput(0.0f),
      prevDCOutput(0.0f),
      previousBypassState(false),
      bypassFadePosition(0.0f),
      // delayBufferWritePos(0),
      // warmupSamplesRemaining(0),
      fadeIncrement(0.0f),
      // warmupSamplesTotal(0),
      targetInputLevel(1.0f),
      targetOutputLevel(1.0f),
      targetBypassGain(0.0f),
      inputLevel(1.0f),
      outputLevel(1.0f),
      maxBufferSize(4096)
{
    currentModelPath.reserve(1024);

    // Set default max buffer size
    NeuralAudio::NeuralModel::SetDefaultMaxAudioBufferSize(maxBufferSize);
}

NAMPlugin::~NAMPlugin()
{
}

void NAMPlugin::initParameter(uint32_t index, Parameter& parameter)
{
    parameter.hints = kParameterIsAutomatable;

    switch (index) {
    case kParameterInputLevel:
        parameter.name = "Input Level";
        parameter.symbol = "input_level";
        parameter.unit = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -20.0f;
        parameter.ranges.max = 20.0f;
        break;

    case kParameterOutputLevel:
        parameter.name = "Output Level";
        parameter.symbol = "output_level";
        parameter.unit = "dB";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -20.0f;
        parameter.ranges.max = 20.0f;
        break;

    case kParameterEnabled:
        // DPF will override name to "Enabled" and symbol to "lv2_enabled"
        // when using kParameterDesignationBypass
        // Semantics: 1.0 = enabled (active), 0.0 = disabled (bypassed)
        parameter.hints |= kParameterIsBoolean;
        parameter.ranges.def = 1.0f;  // Default: enabled (active)
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.designation = kParameterDesignationBypass;
        break;

    case kParameterHardBypass:
        parameter.name = "Hard Bypass";
        parameter.symbol = "hard_bypass";
        parameter.hints |= kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        break;
    }
}

void NAMPlugin::initState(uint32_t index, State& state)
{
    if (index == 0) {
        state.key = kStateKeyModelPath;
        state.label = "Model Path";
        state.description = "Path to the neural model file";
        state.hints = kStateIsFilenamePath;
        state.defaultValue = "";
    }
}

float NAMPlugin::getParameterValue(uint32_t index) const
{
    switch (index) {
    case kParameterInputLevel:
        return fInputLevel;
    case kParameterOutputLevel:
        return fOutputLevel;
    case kParameterEnabled:
        return fEnabled;
    case kParameterHardBypass:
        return fHardBypass;
    default:
        return 0.0f;
    }
}

void NAMPlugin::setParameterValue(uint32_t index, float value)
{
    std::fprintf(stderr, "NAM DSP: setParameterValue index=%u value=%f\n", index, value);
    switch (index) {
    case kParameterInputLevel:
        fInputLevel = value;
        break;
    case kParameterOutputLevel:
        fOutputLevel = value;
        break;
    case kParameterEnabled:
        fEnabled = value;
        std::fprintf(stderr, "NAM DSP: Enabled set to %f (1=active, 0=bypassed)\n", value);
        break;
    case kParameterHardBypass:
        fHardBypass = value;
        break;
    }
}

String NAMPlugin::getState(const char* key) const
{
    if (std::strcmp(key, kStateKeyModelPath) == 0) {
        return String(currentModelPath.c_str());
    }
    return String();
}

void NAMPlugin::setState(const char* key, const char* value)
{
    if (std::strcmp(key, kStateKeyModelPath) == 0) {
        loadModel(value);
    }
}

void NAMPlugin::activate()
{
    // Reset processing state
    prevDCInput = 0.0f;
    prevDCOutput = 0.0f;
    previousBypassState = false;
    bypassFadePosition = 0.0f;
}

void NAMPlugin::deactivate()
{
}

void NAMPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    const float* in = inputs[0];
    float* out = outputs[0];

    // Debug buffer size
    static int debugCounter7 = 0;
    static uint32_t lastFrames = 0;
    if (frames != lastFrames) {
        std::fprintf(stderr, "NAM DSP: Buffer size changed to %u (maxBufferSize=%d)\n", frames, maxBufferSize);
        lastFrames = frames;
    }

    // Debug input level
    static int debugCounter5 = 0;
    if (debugCounter5++ % 100 == 0) {
        float maxInput = 0.0f;
        for (uint32_t i = 0; i < frames; i++) {
            maxInput = std::max(maxInput, std::abs(in[i]));
        }
        std::fprintf(stderr, "NAM DSP: Raw input max sample = %f, fEnabled = %f, frames = %u\n", maxInput, fEnabled, frames);
    }

    // Check enabled state: 1.0 = enabled (active), 0.0 = disabled (bypassed)
    const bool bypassed = fEnabled < 0.5f;

    // If bypassed, just copy input to output
    if (bypassed) {
        std::copy(in, in + frames, out);
        return;
    }

    // ========== Calculate Target Gain Values ==========
    float modelInputAdjustmentDB = 0.0f;
    float modelOutputAdjustmentDB = 0.0f;

    if (currentModel != nullptr) {
        modelInputAdjustmentDB = currentModel->GetRecommendedInputDBAdjustment();
        modelOutputAdjustmentDB = currentModel->GetRecommendedOutputDBAdjustment();
        static int debugCounter = 0;
        if (debugCounter++ % 100 == 0) {
            std::fprintf(stderr, "NAM DSP: Model adjustments - input=%f dB, output=%f dB\n",
                        modelInputAdjustmentDB, modelOutputAdjustmentDB);
        }
    }

    targetInputLevel = std::pow(10.0f, (fInputLevel + modelInputAdjustmentDB) * 0.05f);
    targetOutputLevel = std::pow(10.0f, (fOutputLevel + modelOutputAdjustmentDB) * 0.05f);

    static int debugCounter2 = 0;
    if (debugCounter2++ % 100 == 0) {
        std::fprintf(stderr, "NAM DSP: Target gains - input=%f, output=%f (current: in=%f, out=%f)\n",
                    targetInputLevel, targetOutputLevel, inputLevel, outputLevel);
    }

    // ========== Apply Input Gain ==========
    const float smoothCoeff = SMOOTH_COEFF;
    float inGain = inputLevel;

    for (uint32_t i = 0; i < frames; i++) {
        inGain += smoothCoeff * (targetInputLevel - inGain);
        out[i] = in[i] * inGain;
    }
    inputLevel = inGain;

    // ========== Process Neural Model ==========
    if (currentModel != nullptr) {
        static int debugCounter3 = 0;
        if (debugCounter3++ % 100 == 0) {
            float maxIn = 0.0f;
            for (uint32_t i = 0; i < frames; i++) {
                maxIn = std::max(maxIn, std::abs(out[i]));
            }
            std::fprintf(stderr, "NAM DSP: Before model processing, max sample = %f\n", maxIn);
        }

        // currentModel->Process(out, out, frames);

        static int debugCounter4 = 0;
        if (debugCounter4++ % 100 == 0) {
            float maxOut = 0.0f;
            for (uint32_t i = 0; i < frames; i++) {
                maxOut = std::max(maxOut, std::abs(out[i]));
            }
            std::fprintf(stderr, "NAM DSP: After model processing, max sample = %f\n", maxOut);
        }
    }

    // ========== Apply Output Gain ==========
    float outGain = outputLevel;

    for (uint32_t i = 0; i < frames; i++) {
        outGain += smoothCoeff * (targetOutputLevel - outGain);
        out[i] *= outGain;
    }

    outputLevel = outGain;

    // Debug final output
    static int debugCounter6 = 0;
    if (debugCounter6++ % 100 == 0) {
        float maxFinal = 0.0f;
        for (uint32_t i = 0; i < frames; i++) {
            maxFinal = std::max(maxFinal, std::abs(out[i]));
        }
        std::fprintf(stderr, "NAM DSP: FINAL OUTPUT max sample = %f\n", maxFinal);
    }

    std::copy(in, in + frames, out);
}

void NAMPlugin::sampleRateChanged(double newSampleRate)
{
    sampleRate = newSampleRate;
}

void NAMPlugin::loadModel(const char* path)
{
    if (!path || std::strlen(path) == 0) {
        std::fprintf(stderr, "NAM DSP: Clearing model\n");
        currentModel.reset();
        currentModelPath.clear();
        return;
    }

    std::fprintf(stderr, "NAM DSP: Attempting to load model from: %s\n", path);
    try {
        auto newModel = NeuralAudio::NeuralModel::CreateFromFile(path);
        if (newModel != nullptr) {
            currentModel.reset(newModel);
            currentModelPath = path;
            std::fprintf(stderr, "NAM DSP: Model loaded successfully\n");

            // Reset processing state when model changes
            prevDCInput = 0.0f;
            prevDCOutput = 0.0f;
        } else {
            std::fprintf(stderr, "NAM DSP: Model creation returned nullptr\n");
        }
    } catch (const std::exception& e) {
        // Model loading failed
        std::fprintf(stderr, "NAM DSP: Model loading failed: %s\n", e.what());
        currentModel.reset();
        currentModelPath.clear();
    }
}

Plugin* createPlugin()
{
    return new NAMPlugin();
}

END_NAMESPACE_DISTRHO
