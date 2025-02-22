// a non-empty file so git doesn't ignore the directory
// the cli doesn't seem to check beyond that there _is_
// a src/ directory, so this is enough to make it work
#include <pebble.h>

// Declare persistent storage keys
#define SETTINGS_KEY 1

// Main window and layers
static Window *s_main_window;
static TextLayer *s_time_layer;
static Layer *s_canvas_layer;

// Animation state
static int s_animation_counter = 0;
static AppTimer *s_animation_timer = NULL;
static const int ANIMATION_DELTA = 50; // Faster animation update interval in ms
static const int NUM_WAVES = 3; // Number of sine waves to draw
static const int WAVE_OFFSETS[] = {0, 120, 240}; // Phase offsets for each wave
static GColor WAVE_COLORS[3]; // Colors for each wave

// Initialize wave colors based on platform capabilities
static void init_wave_colors() {
  WAVE_COLORS[0] = PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite);
  WAVE_COLORS[1] = PBL_IF_COLOR_ELSE(GColorRed, GColorLightGray);
  WAVE_COLORS[2] = GColorWhite;
}

// Function declarations
static void update_time(void);
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void canvas_update_proc(Layer *layer, GContext *ctx);
static void animation_timer_callback(void *data);

// Initialize animation timer
static void start_animation(void) {
  // Cancel any existing timer
  if (s_animation_timer) {
    app_timer_cancel(s_animation_timer);
  }
  
  // Schedule new timer
  s_animation_timer = app_timer_register(ANIMATION_DELTA, animation_timer_callback, NULL);
}

// Animation timer callback
static void animation_timer_callback(void *data) {
  // Increment counter
  s_animation_counter = (s_animation_counter + 1) % 360;
  
  // Mark canvas dirty to trigger redraw
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
  
  // Schedule next animation frame
  start_animation();
}

// Draw multiple sine waves with time-based effects
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // Get layer dimensions and time
  GRect bounds = layer_get_bounds(layer);
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Calculate the center y position
  int center_y = bounds.size.h / 2;
  
  // Base amplitude varies with minutes (creates a breathing effect)
  float minute_factor = (float)tick_time->tm_min / 60.0f;
  float base_amplitude = bounds.size.h / 4;
  float time_amplitude = base_amplitude * (0.8f + 0.2f * sin_lookup(DEG_TO_TRIGANGLE(minute_factor * 360)) / TRIG_MAX_RATIO);
  
  // Draw multiple waves
  for (int wave = 0; wave < NUM_WAVES; wave++) {
    // Set wave properties
    graphics_context_set_stroke_color(ctx, WAVE_COLORS[wave]);
    graphics_context_set_stroke_width(ctx, 2);
    
    // Vary frequency based on hours (creates different wave patterns throughout the day)
    float hour_factor = (float)tick_time->tm_hour / 24.0f;
    float frequency = 8.0f + 4.0f * sin_lookup(DEG_TO_TRIGANGLE(hour_factor * 360)) / TRIG_MAX_RATIO;
    
    // Calculate amplitude for this wave
    float wave_amplitude = time_amplitude * (0.7f + 0.3f * wave);
    
    // Draw the wave
    int prev_y = -1;
    for (int x = 0; x < bounds.size.w; x += 2) { // Step by 2 for better performance
      // Calculate wave point with phase offset
      float rads = (float)(x + s_animation_counter + WAVE_OFFSETS[wave]) / frequency;
      int y = center_y + (sin_lookup(DEG_TO_TRIGANGLE(rads)) * wave_amplitude) / TRIG_MAX_RATIO;
      
      // Draw line segment
      if (prev_y != -1) {
        graphics_draw_line(ctx, GPoint(x-2, prev_y), GPoint(x, y));
      }
      prev_y = y;
    }
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the canvas layer for the sine wave
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Start the animation timer
  start_animation();
}

static void main_window_unload(Window *window) {
  // Stop the animation timer
  if (s_animation_timer) {
    app_timer_cancel(s_animation_timer);
    s_animation_timer = NULL;
  }
  
  // Destroy layers
  layer_destroy(s_canvas_layer);
  text_layer_destroy(s_time_layer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                        "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init() {
  // Initialize wave colors
  init_wave_colors();
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Set window background color
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
  update_time();
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
