#include <pebble.h>

// Main window and layers
static Window *s_main_window;
static TextLayer *s_time_layer;
static Layer *s_canvas_layer;

// Animation state
static AppTimer *s_animation_timer = NULL;
static const int ANIMATION_DELTA = 50; // Animation update interval in ms
static int s_angle = 0;  // Current rotation angle
static int s_spiral_phase = 0;  // Phase shift for spiral animation

// Spiral parameters
#define NUM_SPIRALS 3
#define SPIRAL_POINTS 180
#define SPIRAL_SPACING 20
static GColor s_spiral_colors[NUM_SPIRALS];

// Initialize spiral colors
static void init_spiral_colors() {
  s_spiral_colors[0] = PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite);
  s_spiral_colors[1] = PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorLightGray);
  s_spiral_colors[2] = PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite);
}

// Function declarations
static void update_time(void);
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void canvas_update_proc(Layer *layer, GContext *ctx);
static void animation_timer_callback(void *data);

// Draw a single spiral
static void draw_spiral(GContext *ctx, GPoint center, int radius, int phase, GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  
  int prev_x = center.x;
  int prev_y = center.y;
  
  for (int i = 0; i < SPIRAL_POINTS; i++) {
    // Calculate spiral coordinates using parametric equations
    float angle = (float)i / 8.0f + (float)s_angle / 30.0f;
    float growth = (float)i / SPIRAL_POINTS;
    float r = growth * radius;
    
    // Add phase shift for animation
    angle += (float)phase / 30.0f;
    
    int x = center.x + (int)(r * cos_lookup(angle * TRIG_MAX_ANGLE / 60) / TRIG_MAX_RATIO);
    int y = center.y + (int)(r * sin_lookup(angle * TRIG_MAX_ANGLE / 60) / TRIG_MAX_RATIO);
    
    if (i > 0) {
      graphics_draw_line(ctx, GPoint(prev_x, prev_y), GPoint(x, y));
    }
    
    prev_x = x;
    prev_y = y;
  }
}

// Initialize animation timer
static void start_animation(void) {
  if (s_animation_timer) {
    app_timer_cancel(s_animation_timer);
  }
  s_animation_timer = app_timer_register(ANIMATION_DELTA, animation_timer_callback, NULL);
}

// Animation timer callback
static void animation_timer_callback(void *data) {
  s_angle = (s_angle + 1) % 360;
  s_spiral_phase = (s_spiral_phase + 2) % 360;
  
  layer_mark_dirty(s_canvas_layer);
  start_animation();
}

// Draw the spirals
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  
  // Clear background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw multiple spirals with different phases and sizes
  for (int i = 0; i < NUM_SPIRALS; i++) {
    int radius = bounds.size.w / 2 - (i * SPIRAL_SPACING);
    int phase = (s_spiral_phase + (i * 120)) % 360;
    draw_spiral(ctx, center, radius, phase, s_spiral_colors[i]);
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Create time layer
  s_time_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  start_animation();
}

static void main_window_unload(Window *window) {
  if (s_animation_timer) {
    app_timer_cancel(s_animation_timer);
    s_animation_timer = NULL;
  }
  
  layer_destroy(s_canvas_layer);
  text_layer_destroy(s_time_layer);
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                      "%H:%M" : "%I:%M", tick_time);

  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init() {
  // Initialize colors
  init_spiral_colors();
  
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  update_time();
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
