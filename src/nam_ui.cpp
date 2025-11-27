#include "nam_ui.h"
#include <cmath>
#include <cstring>
#include <cairo/cairo.h>
#include <lv2/atom/util.h>

#define MODEL_URI "https://github.com/rickprice/neural-amp-modeler-bypass-lv2#model"

namespace NAM {

PluginUI::PluginUI()
    : world(nullptr), view(nullptr), write_function(nullptr),
      controller(nullptr), map(nullptr), active_widget(nullptr),
      drag_start_value(0.0f), drag_start_y(0.0),
      input_level(0.0f), output_level(0.0f), enabled(1.0f), hard_bypass(0.0f) {

  // Initialize widget positions (x, y, width, height)
  model_button = {20, 50, 480, 45, ControlType::ModelButton, 0, false, false};
  input_slider = {20, 120, 230, 80, ControlType::InputSlider, 0, false, false};
  output_slider = {270, 120, 230, 80, ControlType::OutputSlider, 0, false, false};
  enabled_toggle = {20, 230, 230, 55, ControlType::EnabledToggle, 1, false, false};
  hard_bypass_toggle = {270, 230, 230, 55, ControlType::HardBypassToggle, 0, false, false};
}

PluginUI::~PluginUI() {
  if (view) {
    puglFreeView(view);
  }
  if (world) {
    puglFreeWorld(world);
  }
}

bool PluginUI::initialize(LV2UI_Write_Function write_fn,
                          LV2UI_Controller ctrl,
                          const LV2_Feature* const* features) {
  this->write_function = write_fn;
  this->controller = ctrl;

  PuglNativeView parent = 0;

  // Get features
  for (int i = 0; features[i]; i++) {
    if (!strcmp(features[i]->URI, LV2_URID__map)) {
      map = (LV2_URID_Map*)features[i]->data;
    } else if (!strcmp(features[i]->URI, LV2_UI__parent)) {
      parent = (PuglNativeView)features[i]->data;
    }
  }

  if (!map) {
    return false;
  }

  // Map URIDs
  atom_Path = map->map(map->handle, LV2_ATOM__Path);
  atom_URID = map->map(map->handle, LV2_ATOM__URID);
  atom_Float = map->map(map->handle, LV2_ATOM__Float);
  atom_Object = map->map(map->handle, LV2_ATOM__Object);
  patch_Set = map->map(map->handle, LV2_PATCH__Set);
  patch_Get = map->map(map->handle, LV2_PATCH__Get);
  patch_property = map->map(map->handle, LV2_PATCH__property);
  patch_value = map->map(map->handle, LV2_PATCH__value);
  model_uri = map->map(map->handle, MODEL_URI);

  // Create Pugl world and view
  world = puglNewWorld(PUGL_MODULE, 0);
  view = puglNewView(world);

  puglSetWorldString(world, PUGL_CLASS_NAME, "NeuralAmpModelerUI");
  puglSetSizeHint(view, PUGL_DEFAULT_SIZE, UI_WIDTH, UI_HEIGHT);
  puglSetSizeHint(view, PUGL_MIN_SIZE, UI_WIDTH, UI_HEIGHT);
  puglSetSizeHint(view, PUGL_MAX_SIZE, UI_WIDTH, UI_HEIGHT);
  puglSetBackend(view, puglCairoBackend());
  puglSetHandle(view, this);
  puglSetEventFunc(view, onEvent);
  puglSetViewHint(view, PUGL_RESIZABLE, PUGL_FALSE);

  // Embed in host window
  if (parent) {
    puglSetParent(view, parent);
  }

  PuglStatus status = puglRealize(view);
  if (status != PUGL_SUCCESS) {
    return false;
  }

  puglShow(view, PUGL_SHOW_RAISE);

  // Request current model path from plugin
  request_current_model();

  return true;
}

void PluginUI::port_event(uint32_t port_index, uint32_t buffer_size,
                          uint32_t format, const void* buffer) {
  // Handle atom messages (model path updates)
  if (port_index == 1 && format == atom_Object) {
    const LV2_Atom_Object* obj = (const LV2_Atom_Object*)buffer;

    if (obj->body.otype == patch_Set) {
      // Extract the model path from the patch:Set message
      const LV2_Atom* property = NULL;
      const LV2_Atom* value = NULL;

      lv2_atom_object_get(obj, patch_property, &property, patch_value, &value, 0);

      if (property && property->type == atom_URID &&
          ((const LV2_Atom_URID*)property)->body == model_uri &&
          value && value->type == atom_Path) {
        const char* path = (const char*)(value + 1);
        fprintf(stderr, "[NAM UI] Received model path from plugin: %s\n", path);
        current_model_path = path;
        puglObscureView(view);
      }
    }
    return;
  }

  // Update port values when they change
  switch (port_index) {
    case 4: // input_level
      input_level = *(const float*)buffer;
      input_slider.value = input_level;
      puglObscureView(view);
      break;
    case 5: // output_level
      output_level = *(const float*)buffer;
      output_slider.value = output_level;
      puglObscureView(view);
      break;
    case 6: // enabled
      enabled = *(const float*)buffer;
      enabled_toggle.value = enabled;
      puglObscureView(view);
      break;
    case 7: // hard_bypass
      hard_bypass = *(const float*)buffer;
      hard_bypass_toggle.value = hard_bypass;
      puglObscureView(view);
      break;
  }
}

PuglStatus PluginUI::onEvent(PuglView* view, const PuglEvent* event) {
  PluginUI* ui = (PluginUI*)puglGetHandle(view);

  switch (event->type) {
    case PUGL_EXPOSE: {
      cairo_t* cr = (cairo_t*)puglGetContext(view);
      ui->draw(cr);
      return PUGL_SUCCESS;
    }
    case PUGL_MOTION: {
      ui->handle_motion(event->motion.x, event->motion.y);
      return PUGL_SUCCESS;
    }
    case PUGL_BUTTON_PRESS: {
      fprintf(stderr, "[NAM UI] Button press: button=%d x=%f y=%f\n",
              event->button.button, event->button.x, event->button.y);
      ui->handle_button_press(event->button.x, event->button.y, event->button.button);
      return PUGL_SUCCESS;
    }
    case PUGL_BUTTON_RELEASE: {
      fprintf(stderr, "[NAM UI] Button release: button=%d\n", event->button.button);
      ui->handle_button_release(event->button.x, event->button.y, event->button.button);
      return PUGL_SUCCESS;
    }
    case PUGL_CLOSE: {
      return PUGL_SUCCESS;
    }
    case PUGL_CONFIGURE: {
      fprintf(stderr, "[NAM UI] Configure: %dx%d\n",
              (int)event->configure.width, (int)event->configure.height);
      return PUGL_SUCCESS;
    }
    default:
      break;
  }

  return PUGL_SUCCESS;
}

void PluginUI::draw(cairo_t* cr) {
  // Clear background
  cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);
  cairo_paint(cr);

  // Draw title
  cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
  cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 18);
  cairo_move_to(cr, 20, 30);
  cairo_show_text(cr, "Neural Amp Modeler");

  // Draw widgets
  draw_button(cr, model_button, current_model_path.empty() ? "Load Model..." : current_model_path.c_str());
  draw_slider(cr, input_slider, "Input Level", -20.0f, 20.0f);
  draw_slider(cr, output_slider, "Output Level", -20.0f, 20.0f);
  draw_toggle(cr, enabled_toggle, "Enabled");
  draw_toggle(cr, hard_bypass_toggle, "Hard Bypass");

  // Draw info text
  cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
  cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 10);
  cairo_move_to(cr, 20, 315);
  cairo_show_text(cr, "Supported formats: .nam, .json (NAM/AIDA-X models)");
  cairo_move_to(cr, 20, 335);
  cairo_show_text(cr, "Input/Output: dB adjustment relative to model calibration");
}

void PluginUI::draw_button(cairo_t* cr, const Widget& w, const char* label) {
  // Background
  if (w.dragging) {
    cairo_set_source_rgb(cr, 0.35, 0.5, 0.6); // Pressed state
  } else if (w.hover) {
    cairo_set_source_rgb(cr, 0.3, 0.4, 0.5); // Hover state
  } else {
    cairo_set_source_rgb(cr, 0.25, 0.25, 0.25); // Normal state
  }
  cairo_rectangle(cr, w.x, w.y, w.width, w.height);
  cairo_fill(cr);

  // Border
  if (w.dragging) {
    cairo_set_source_rgb(cr, 0.5, 0.7, 0.8); // Bright border when pressed
  } else {
    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
  }
  cairo_set_line_width(cr, w.dragging ? 2.0 : 1.5);
  cairo_rectangle(cr, w.x, w.y, w.width, w.height);
  cairo_stroke(cr);

  // Text
  cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
  cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 13);

  cairo_text_extents_t extents;
  cairo_text_extents(cr, label, &extents);

  // Truncate text if too long
  std::string display_text = label;
  if (extents.width > w.width - 20) {
    // Show filename only
    const char* filename = strrchr(label, '/');
    if (filename) {
      display_text = filename + 1;
      cairo_text_extents(cr, display_text.c_str(), &extents);
    }
  }

  cairo_move_to(cr, w.x + (w.width - extents.width) / 2, w.y + (w.height + extents.height) / 2);
  cairo_show_text(cr, display_text.c_str());
}

void PluginUI::draw_slider(cairo_t* cr, const Widget& w, const char* label, float min, float max) {
  // Label
  cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
  cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 12);
  cairo_move_to(cr, w.x, w.y + 12);
  cairo_show_text(cr, label);

  // Value display
  char value_str[32];
  snprintf(value_str, sizeof(value_str), "%.1f dB", w.value);
  cairo_text_extents_t extents;
  cairo_text_extents(cr, value_str, &extents);
  cairo_move_to(cr, w.x + w.width - extents.width, w.y + 12);
  cairo_show_text(cr, value_str);

  // Slider track
  float track_y = w.y + 30;
  float track_height = 20;

  cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
  cairo_rectangle(cr, w.x, track_y, w.width, track_height);
  cairo_fill(cr);

  // Slider fill
  float normalized = (w.value - min) / (max - min);
  normalized = fmaxf(0.0f, fminf(1.0f, normalized));

  cairo_set_source_rgb(cr, 0.3, 0.5, 0.7);
  cairo_rectangle(cr, w.x, track_y, w.width * normalized, track_height);
  cairo_fill(cr);

  // Slider handle
  float handle_x = w.x + w.width * normalized;
  cairo_set_source_rgb(cr, w.hover ? 0.8 : 0.6, w.hover ? 0.8 : 0.6, w.hover ? 0.8 : 0.6);
  cairo_rectangle(cr, handle_x - 3, track_y - 2, 6, track_height + 4);
  cairo_fill(cr);

  // Border
  cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
  cairo_set_line_width(cr, 1.0);
  cairo_rectangle(cr, w.x, track_y, w.width, track_height);
  cairo_stroke(cr);
}

void PluginUI::draw_toggle(cairo_t* cr, const Widget& w, const char* label) {
  bool is_on = w.value >= 0.5f;

  // Background
  if (w.hover) {
    cairo_set_source_rgb(cr, is_on ? 0.4 : 0.3, is_on ? 0.5 : 0.3, is_on ? 0.4 : 0.3);
  } else {
    cairo_set_source_rgb(cr, is_on ? 0.3 : 0.25, is_on ? 0.45 : 0.25, is_on ? 0.35 : 0.25);
  }
  cairo_rectangle(cr, w.x, w.y, w.width, w.height);
  cairo_fill(cr);

  // Border
  cairo_set_source_rgb(cr, is_on ? 0.5 : 0.4, is_on ? 0.6 : 0.4, is_on ? 0.5 : 0.4);
  cairo_set_line_width(cr, 2.0);
  cairo_rectangle(cr, w.x, w.y, w.width, w.height);
  cairo_stroke(cr);

  // Text
  cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
  cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 14);

  std::string display = std::string(label) + (is_on ? ": ON" : ": OFF");
  cairo_text_extents_t extents;
  cairo_text_extents(cr, display.c_str(), &extents);
  cairo_move_to(cr, w.x + (w.width - extents.width) / 2, w.y + (w.height + extents.height) / 2);
  cairo_show_text(cr, display.c_str());
}

Widget* PluginUI::get_widget_at(double x, double y) {
  Widget* widgets[] = {&model_button, &input_slider, &output_slider, &enabled_toggle, &hard_bypass_toggle};

  for (Widget* w : widgets) {
    if (x >= w->x && x <= w->x + w->width && y >= w->y && y <= w->y + w->height) {
      return w;
    }
  }

  return nullptr;
}

void PluginUI::handle_motion(double x, double y) {
  if (active_widget && active_widget->dragging) {
    if (active_widget->type == ControlType::InputSlider ||
        active_widget->type == ControlType::OutputSlider) {
      // Calculate new value based on horizontal drag
      double delta = (x - active_widget->x) / active_widget->width;
      float new_value = -20.0f + delta * 40.0f; // Map to -20 to +20 dB
      new_value = fmaxf(-20.0f, fminf(20.0f, new_value));

      active_widget->value = new_value;

      // Send to plugin
      if (active_widget->type == ControlType::InputSlider) {
        send_control_value(4, new_value);
      } else {
        send_control_value(5, new_value);
      }

      puglObscureView(view);
    }
    return;
  }

  // Update hover states
  Widget* widgets[] = {&model_button, &input_slider, &output_slider, &enabled_toggle, &hard_bypass_toggle};
  bool redraw = false;

  for (Widget* w : widgets) {
    bool was_hover = w->hover;
    w->hover = (x >= w->x && x <= w->x + w->width && y >= w->y && y <= w->y + w->height);
    if (was_hover != w->hover) {
      redraw = true;
    }
  }

  if (redraw) {
    puglObscureView(view);
  }
}

void PluginUI::handle_button_press(double x, double y, uint32_t button) {
  fprintf(stderr, "[NAM UI] handle_button_press called: x=%f y=%f button=%d\n", x, y, button);

  if (button != 0) {
    fprintf(stderr, "[NAM UI] Ignoring non-left-click button %d\n", button);
    return; // Only handle left click (button 0 in Pugl)
  }

  Widget* w = get_widget_at(x, y);
  if (!w) {
    fprintf(stderr, "[NAM UI] No widget found at %f,%f\n", x, y);
    return;
  }

  fprintf(stderr, "[NAM UI] Widget found: type=%d\n", (int)w->type);

  switch (w->type) {
    case ControlType::ModelButton:
      fprintf(stderr, "[NAM UI] Model button clicked! Opening file dialog...\n");
      w->dragging = true; // Visual feedback
      puglObscureView(view);
      open_file_dialog();
      w->dragging = false;
      puglObscureView(view);
      fprintf(stderr, "[NAM UI] File dialog closed. Current model: %s\n", current_model_path.c_str());
      break;

    case ControlType::InputSlider:
    case ControlType::OutputSlider:
      w->dragging = true;
      active_widget = w;
      drag_start_value = w->value;
      drag_start_y = y;
      handle_motion(x, y); // Update value immediately
      break;

    case ControlType::EnabledToggle:
    case ControlType::HardBypassToggle: {
      w->value = (w->value >= 0.5f) ? 0.0f : 1.0f;
      uint32_t port = (w->type == ControlType::EnabledToggle) ? 6 : 7;
      send_control_value(port, w->value);
      puglObscureView(view);
      break;
    }

    default:
      break;
  }
}

void PluginUI::handle_button_release(double x, double y, uint32_t button) {
  if (active_widget) {
    active_widget->dragging = false;
    active_widget = nullptr;
  }
}

void PluginUI::send_control_value(uint32_t port, float value) {
  fprintf(stderr, "[NAM UI] send_control_value: port=%d value=%f\n", port, value);
  if (write_function) {
    write_function(controller, port, sizeof(float), 0, &value);
  } else {
    fprintf(stderr, "[NAM UI] ERROR: write_function is NULL!\n");
  }
}

void PluginUI::open_file_dialog() {
  fprintf(stderr, "[NAM UI] open_file_dialog() called\n");

  // Try zenity first, then kdialog as fallback
  const char* commands[] = {
    "zenity --file-selection --title='Select NAM Model' "
    "--file-filter='Model Files (*.nam *.json *.aidax)' --file-filter='*.nam' --file-filter='*.json' --file-filter='*.aidax' 2>/dev/null",

    "kdialog --getopenfilename ~ '*.nam *.json *.aidax|Model Files' 2>/dev/null",

    nullptr
  };

  for (int i = 0; commands[i] != nullptr; i++) {
    fprintf(stderr, "[NAM UI] Trying command %d: %s\n", i, commands[i]);
    FILE* fp = popen(commands[i], "r");
    if (!fp) continue;

    char path[1024];
    bool got_path = false;

    if (fgets(path, sizeof(path), fp)) {
      // Remove trailing newline
      size_t len = strlen(path);
      if (len > 0 && path[len-1] == '\n') {
        path[len-1] = '\0';
      }

      if (strlen(path) > 0) {
        current_model_path = path;
        send_model_path(path);
        got_path = true;
        puglObscureView(view);
      }
    }

    pclose(fp);

    if (got_path) {
      return; // Success
    }
  }

  // If we get here, no dialog worked - set a message
  current_model_path = "[Click to select model file]";
  puglObscureView(view);
}

void PluginUI::send_model_path(const char* path) {
  if (!write_function) return;

  fprintf(stderr, "[NAM UI] Sending model path to plugin: %s\n", path);

  // Build an atom message to set the model path
  uint8_t buffer[1024];
  LV2_Atom_Forge forge;
  lv2_atom_forge_init(&forge, map);
  lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

  LV2_Atom_Forge_Frame frame;
  LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object(&forge, &frame, 0, patch_Set);

  lv2_atom_forge_key(&forge, patch_property);
  lv2_atom_forge_urid(&forge, model_uri);
  lv2_atom_forge_key(&forge, patch_value);
  lv2_atom_forge_path(&forge, path, strlen(path) + 1);

  lv2_atom_forge_pop(&forge, &frame);

  // Send to plugin via control port (port 0)
  write_function(controller, 0, lv2_atom_total_size(msg),
                 map->map(map->handle, LV2_ATOM__eventTransfer), msg);
}

void PluginUI::request_current_model() {
  if (!write_function) return;

  fprintf(stderr, "[NAM UI] Requesting current model path from plugin\n");

  // Build a patch:Get message to request the current model path
  uint8_t buffer[256];
  LV2_Atom_Forge forge;
  lv2_atom_forge_init(&forge, map);
  lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

  LV2_Atom_Forge_Frame frame;
  LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object(&forge, &frame, 0, patch_Get);

  lv2_atom_forge_key(&forge, patch_property);
  lv2_atom_forge_urid(&forge, model_uri);

  lv2_atom_forge_pop(&forge, &frame);

  // Send to plugin via control port (port 0)
  write_function(controller, 0, lv2_atom_total_size(msg),
                 map->map(map->handle, LV2_ATOM__eventTransfer), msg);
}

} // namespace NAM
