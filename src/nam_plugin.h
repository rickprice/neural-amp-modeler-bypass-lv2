#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>
#include <vector>

// LV2
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/options/options.h>
#include <lv2/patch/patch.h>
#include <lv2/state/state.h>
#include <lv2/units/units.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>

#include <NeuralAudio/NeuralModel.h>

#define PLUGIN_URI "https://github.com/rickprice/neural-amp-modeler-bypass-lv2"
#define MODEL_URI PLUGIN_URI "#model"

namespace NAM {
static constexpr unsigned int MAX_FILE_NAME = 1024;

enum LV2WorkType { kWorkTypeLoad, kWorkTypeSwitch, kWorkTypeFree };

struct LV2LoadModelMsg {
  LV2WorkType type;
  char path[MAX_FILE_NAME];
};

struct LV2SwitchModelMsg {
  LV2WorkType type;
  char path[MAX_FILE_NAME];
  NeuralAudio::NeuralModel *model;
};

struct LV2FreeModelMsg {
  LV2WorkType type;
  NeuralAudio::NeuralModel *model;
};

class Plugin {
public:
  struct Ports {
    const LV2_Atom_Sequence *control;
    LV2_Atom_Sequence *notify;
    const float *audio_in;
    float *audio_out;
    float *input_level;
    float *output_level;
    float *enabled;
    float *hard_bypass;
  };

  Ports ports = {};

  double sampleRate;

  LV2_URID_Map *map = nullptr;
  LV2_Log_Logger logger = {};
  LV2_Worker_Schedule *schedule = nullptr;

  NeuralAudio::NeuralModel *currentModel = nullptr;
  std::string currentModelPath;
  float prevDCInput = 0;
  float prevDCOutput = 0;

  // Target values for gain smoothing
  float targetInputLevel = 1.0f;
  float targetOutputLevel = 1.0f;

  // Smoothing coefficient for all gain transitions
  static constexpr float SMOOTH_COEFF = 0.001f;

  Plugin();
  ~Plugin();

  bool initialize(double rate, const LV2_Feature *const *features) noexcept;
  void set_max_buffer_size(int size) noexcept;
  void process(uint32_t n_samples) noexcept;

  void write_current_path();
  void send_recommended_levels();

  static uint32_t options_get(LV2_Handle instance, LV2_Options_Option *options);
  static uint32_t options_set(LV2_Handle instance,
                              const LV2_Options_Option *options);

  static LV2_Worker_Status work(LV2_Handle instance,
                                LV2_Worker_Respond_Function respond,
                                LV2_Worker_Respond_Handle handle, uint32_t size,
                                const void *data);
  static LV2_Worker_Status work_response(LV2_Handle instance, uint32_t size,
                                         const void *data);

  static LV2_State_Status save(LV2_Handle instance,
                               LV2_State_Store_Function store,
                               LV2_State_Handle handle, uint32_t flags,
                               const LV2_Feature *const *features);
  static LV2_State_Status restore(LV2_Handle instance,
                                  LV2_State_Retrieve_Function retrieve,
                                  LV2_State_Handle handle, uint32_t flags,
                                  const LV2_Feature *const *features);

private:
  struct URIs {
    LV2_URID atom_Object;
    LV2_URID atom_Float;
    LV2_URID atom_Int;
    LV2_URID atom_Path;
    LV2_URID atom_URID;
    LV2_URID bufSize_maxBlockLength;
    LV2_URID patch_Set;
    LV2_URID patch_Get;
    LV2_URID patch_property;
    LV2_URID patch_value;
    LV2_URID units_frame;
    LV2_URID model_Path;
    LV2_URID recommended_input;
    LV2_URID recommended_output;
  };

  URIs uris = {};

  LV2_Atom_Forge atom_forge = {};
  LV2_Atom_Forge_Frame sequence_frame;

  float inputLevel =
      1.0f; // Initialize to unity gain to avoid silence on startup
  float outputLevel =
      1.0f; // Initialize to unity gain to avoid silence on startup
  int32_t maxBufferSize = 512;

  bool send_recommended_levels_flag = false;
  bool send_model_path_flag = false;

  // Buffer to save input for soft bypass (supports in-place processing)
  std::vector<float> savedInputBuffer;
};
} // namespace NAM
