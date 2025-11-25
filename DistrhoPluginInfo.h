#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Frederick Price"
#define DISTRHO_PLUGIN_NAME  "Neural Amp Modeler"
#define DISTRHO_PLUGIN_URI   "http://github.com/rickprice/neural-amp-modeler-bypass-lv2"
#define DISTRHO_PLUGIN_CLAP_ID "com.pricemail.NeuralAmpModeler"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_NUM_INPUTS    1
#define DISTRHO_PLUGIN_NUM_OUTPUTS   1
#define DISTRHO_PLUGIN_WANT_STATE    1
#define DISTRHO_PLUGIN_WANT_FULL_STATE 1
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  0
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0

// UI settings
#define DISTRHO_UI_USE_NANOVG        1
#define DISTRHO_UI_DEFAULT_WIDTH     500
#define DISTRHO_UI_DEFAULT_HEIGHT    300

// LV2 specific
#define DISTRHO_PLUGIN_LV2_CATEGORY  "lv2:SimulatorPlugin"

// VST3 specific
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Fx|Distortion"

// CLAP specific
#define DISTRHO_PLUGIN_CLAP_FEATURES "audio-effect", "distortion"

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
