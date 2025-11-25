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
      fEnabled(1.0f),
      fHardBypass(0.0f),
      currentModel(nullptr),
      sampleRate(getSampleRate()),
      prevDCInput(0.0f),
      prevDCOutput(0.0f),
      previousBypassState(false),
      bypassFadePosition(0.0f),
      delayBufferWritePos(0),
      warmupSamplesRemaining(0),
      fadeIncrement(0.0f),
      warmupSamplesTotal(0),
      targetInputLevel(1.0f),
      targetOutputLevel(1.0f),
      targetBypassGain(0.0f),
      inputLevel(1.0f),
      outputLevel(1.0f),
      maxBufferSize(512)
{
    currentModelPath.reserve(1024);

    // Pre-calculate fade coefficients
    fadeIncrement = 1.0f / ((FADE_TIME_MS / 1000.0f) * static_cast<float>(sampleRate));
    warmupSamplesTotal = static_cast<size_t>((WARMUP_TIME_MS / 1000.0) * sampleRate);

    // Initialize delay buffer
    updateDelayBufferSize();

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
        parameter.name = "Enabled";
        parameter.symbol = "enabled";
        parameter.hints |= kParameterIsBoolean;
        parameter.ranges.def = 1.0f;
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
    warmupSamplesRemaining = 0;

    // Clear delay buffer
    std::fill(inputDelayBuffer.begin(), inputDelayBuffer.end(), 0.0f);
    delayBufferWritePos = 0;
}

void NAMPlugin::deactivate()
{
}

void NAMPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    const float* in = inputs[0];
    float* out = outputs[0];

    // ========== Bypass State Management ==========
    const bool bypassed = fEnabled < 0.5f;
    const bool hardBypassed = fHardBypass >= 0.5f;

    // Detect bypass state change
    if (bypassed != previousBypassState) {
        previousBypassState = bypassed;
        if (!bypassed) {
            warmupSamplesRemaining = warmupSamplesTotal;
        }
    }

    // Hard bypass early exit
    if (bypassed && hardBypassed && bypassFadePosition >= 1.0f) {
        std::copy(in, in + frames, out);
        return;
    }

    // Update bypass fade position
    if (bypassed && bypassFadePosition < 1.0f) {
        bypassFadePosition = std::min(1.0f, bypassFadePosition + (fadeIncrement * frames));
    } else if (!bypassed && bypassFadePosition > 0.0f) {
        if (warmupSamplesRemaining > 0) {
            bypassFadePosition = 1.0f;
            warmupSamplesRemaining = (warmupSamplesRemaining > frames)
                ? (warmupSamplesRemaining - frames) : 0;
        } else {
            bypassFadePosition = std::max(0.0f, bypassFadePosition - (fadeIncrement * frames));
        }
    }

    targetBypassGain = bypassFadePosition;

    // ========== Calculate Target Gain Values ==========
    float modelInputAdjustmentDB = 0.0f;
    float modelOutputAdjustmentDB = 0.0f;

    if (currentModel != nullptr) {
        modelInputAdjustmentDB = currentModel->GetRecommendedInputDBAdjustment();
        modelOutputAdjustmentDB = currentModel->GetRecommendedOutputDBAdjustment();
    }

    targetInputLevel = std::pow(10.0f, (fInputLevel + modelInputAdjustmentDB) * 0.05f);
    targetOutputLevel = std::pow(10.0f, (fOutputLevel + modelOutputAdjustmentDB) * 0.05f);

    // ========== Apply Input Gain ==========
    const float smoothCoeff = SMOOTH_COEFF;
    float inGain = inputLevel;

    for (uint32_t i = 0; i < frames; i++) {
        inGain += smoothCoeff * (targetInputLevel - inGain);
        out[i] = in[i] * inGain;
    }
    inputLevel = inGain;

    // ========== Store to Delay Buffer ==========
    const size_t delaySize = inputDelayBuffer.size();
    size_t writePos = delayBufferWritePos;

    for (uint32_t i = 0; i < frames; i++) {
        inputDelayBuffer[writePos] = out[i];
        writePos++;
        if (writePos >= delaySize)
            writePos = 0;
    }
    delayBufferWritePos = writePos;

    // ========== Process Neural Model ==========
    if (currentModel != nullptr) {
        currentModel->Process(out, out, frames);
    }

    // ========== Apply Output Gain and Mix with Dry ==========
    size_t readPos = (delayBufferWritePos + delaySize - maxBufferSize - frames) % delaySize;

    float outGain = outputLevel;
    float mixGain = targetBypassGain;

    for (uint32_t i = 0; i < frames; i++) {
        // Smooth gains
        outGain += smoothCoeff * (targetOutputLevel - outGain);
        mixGain += smoothCoeff * (targetBypassGain - mixGain);

        // Calculate wet/dry mix
        const float wetGain = (mixGain > 0.95f) ? 0.0f : (1.0f - mixGain);
        const float dryGain = 1.0f - wetGain;

        // Mix signals
        const float wet = out[i] * outGain * wetGain;
        const float dry = inputDelayBuffer[readPos] * dryGain;
        out[i] = wet + dry;

        readPos++;
        if (readPos >= delaySize)
            readPos = 0;
    }

    outputLevel = outGain;
}

void NAMPlugin::sampleRateChanged(double newSampleRate)
{
    sampleRate = newSampleRate;

    // Recalculate fade coefficients
    fadeIncrement = 1.0f / ((FADE_TIME_MS / 1000.0f) * static_cast<float>(sampleRate));
    warmupSamplesTotal = static_cast<size_t>((WARMUP_TIME_MS / 1000.0) * sampleRate);

    // Update delay buffer
    updateDelayBufferSize();
}

void NAMPlugin::updateDelayBufferSize()
{
    size_t fadeTimeSamples = static_cast<size_t>((FADE_TIME_MS / 1000.0) * sampleRate);
    size_t delayBufferSize = fadeTimeSamples + maxBufferSize;

    inputDelayBuffer.resize(delayBufferSize, 0.0f);
    delayBufferWritePos = 0;
}

void NAMPlugin::loadModel(const char* path)
{
    if (!path || std::strlen(path) == 0) {
        currentModel.reset();
        currentModelPath.clear();
        return;
    }

    try {
        auto newModel = NeuralAudio::NeuralModel::CreateFromFile(path);
        if (newModel != nullptr) {
            currentModel.reset(newModel);
            currentModelPath = path;
        }
    } catch (const std::exception& e) {
        // Model loading failed
        currentModel.reset();
        currentModelPath.clear();
    }
}

Plugin* createPlugin()
{
    return new NAMPlugin();
}

END_NAMESPACE_DISTRHO
