#pragma once

#include <string>

// LV2
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/patch/patch.h>
#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>

// Pugl
#include <pugl/pugl.h>
#include <pugl/cairo.h>

// Cairo forward declaration
typedef struct _cairo cairo_t;

#define UI_WIDTH 520
#define UI_HEIGHT 400

namespace NAM {

enum class ControlType {
  None,
  ModelButton,
  InputSlider,
  OutputSlider,
  EnabledToggle,
  HardBypassToggle
};

struct Widget {
  float x, y, width, height;
  ControlType type;
  float value;
  bool hover;
  bool dragging;
};

class PluginUI {
public:
  PluginUI();
  ~PluginUI();

  bool initialize(LV2UI_Write_Function write_function,
                  LV2UI_Controller controller, const LV2_Feature* const* features);

  void port_event(uint32_t port_index, uint32_t buffer_size,
                  uint32_t format, const void* buffer);

  PuglView* get_view() const { return view; }

  // Pugl callbacks
  static PuglStatus onEvent(PuglView* view, const PuglEvent* event);

private:
  void draw(cairo_t* cr);
  void handle_motion(double x, double y);
  void handle_button_press(double x, double y, uint32_t button);
  void handle_button_release(double x, double y, uint32_t button);

  void draw_button(cairo_t* cr, const Widget& w, const char* label);
  void draw_slider(cairo_t* cr, const Widget& w, const char* label, float min, float max);
  void draw_toggle(cairo_t* cr, const Widget& w, const char* label);

  void send_control_value(uint32_t port, float value);
  void open_file_dialog();
  void send_model_path(const char* path);
  void request_current_model();

  Widget* get_widget_at(double x, double y);

  PuglWorld* world;
  PuglView* view;

  LV2UI_Write_Function write_function;
  LV2UI_Controller controller;
  LV2_URID_Map* map;

  // URIs
  LV2_URID atom_Path;
  LV2_URID atom_URID;
  LV2_URID atom_Float;
  LV2_URID atom_Object;
  LV2_URID patch_Set;
  LV2_URID patch_Get;
  LV2_URID patch_property;
  LV2_URID patch_value;
  LV2_URID model_uri;

  // Widgets
  Widget model_button;
  Widget input_slider;
  Widget output_slider;
  Widget enabled_toggle;
  Widget hard_bypass_toggle;

  Widget* active_widget;
  float drag_start_value;
  double drag_start_y;

  std::string current_model_path;

  // Port values
  float input_level;
  float output_level;
  float enabled;
  float hard_bypass;
};

} // namespace NAM
