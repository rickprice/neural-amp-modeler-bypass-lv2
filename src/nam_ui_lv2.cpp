#include "nam_ui.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define UI_URI "https://github.com/rickprice/neural-amp-modeler-bypass-lv2#ui"

static LV2UI_Handle
ui_instantiate(const LV2UI_Descriptor* descriptor,
               const char* plugin_uri,
               const char* bundle_path,
               LV2UI_Write_Function write_function,
               LV2UI_Controller controller,
               LV2UI_Widget* widget,
               const LV2_Feature* const* features) {
  try {
    NAM::PluginUI* ui = new NAM::PluginUI();

    if (!ui->initialize(write_function, controller, features)) {
      delete ui;
      return nullptr;
    }

    *widget = (LV2UI_Widget)puglGetNativeView(ui->get_view());

    return (LV2UI_Handle)ui;
  } catch (...) {
    return nullptr;
  }
}

static void
ui_cleanup(LV2UI_Handle handle) {
  NAM::PluginUI* ui = (NAM::PluginUI*)handle;
  delete ui;
}

static void
ui_port_event(LV2UI_Handle handle,
              uint32_t port_index,
              uint32_t buffer_size,
              uint32_t format,
              const void* buffer) {
  NAM::PluginUI* ui = (NAM::PluginUI*)handle;
  ui->port_event(port_index, buffer_size, format, buffer);
}

static int
ui_idle(LV2UI_Handle handle) {
  NAM::PluginUI* ui = (NAM::PluginUI*)handle;
  puglUpdate(puglGetWorld(ui->get_view()), 0.0);
  return 0;
}

static const void*
ui_extension_data(const char* uri) {
  static const LV2UI_Idle_Interface idle = { ui_idle };

  if (!strcmp(uri, LV2_UI__idleInterface)) {
    return &idle;
  }

  return nullptr;
}

static const LV2UI_Descriptor descriptor = {
  UI_URI,
  ui_instantiate,
  ui_cleanup,
  ui_port_event,
  ui_extension_data
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index) {
  if (index == 0) {
    return &descriptor;
  }
  return nullptr;
}
