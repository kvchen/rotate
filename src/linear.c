#include "linear.h"

#include "pebble.h"

#include "string.h"
#include "stdlib.h"

Layer *simple_bg_layer;

static GPath *minute_arrow;
static GPath *hour_arrow;
Layer *hands_layer;
Window *window;

static void bg_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&bounds);
  const int16_t tick_radius = bounds.size.w / 2 - 8;

  // fills background in black
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  // interval ticks
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    int16_t tick_angle = TRIG_MAX_ANGLE * i / 12;

    GPoint tick;
    tick.y = (int16_t)(-cos_lookup(tick_angle) * (int32_t)tick_radius / TRIG_MAX_RATIO) + center.y;
    tick.x = (int16_t)(sin_lookup(tick_angle) * (int32_t)tick_radius / TRIG_MAX_RATIO) + center.x;
    int16_t tick_size = (i % 3 == 0) * 2 + 2;

    graphics_fill_circle(ctx, tick, tick_size);
  }

}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&bounds);
  const int16_t secondLength = bounds.size.w / 2 - 8;
  GPoint secondTick;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  int16_t sec = t->tm_sec;
  int16_t secondRadius = (sec % 5 == 0) * 2 + ((sec % 15 == 0)*3) + 1;

  int16_t secondAngle = TRIG_MAX_ANGLE * sec / 60;
  secondTick.y = (int16_t)(-cos_lookup(secondAngle) * (int32_t)secondLength / TRIG_MAX_RATIO) + center.y;
  secondTick.x = (int16_t)(sin_lookup(secondAngle) * (int32_t)secondLength / TRIG_MAX_RATIO) + center.x;

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, secondTick, secondRadius);

  // minute/hour hand

  gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, hour_arrow);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 10);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, minute_arrow);
  gpath_draw_outline(ctx, minute_arrow);
  graphics_fill_circle(ctx, center, 7);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // init layers
  simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, simple_bg_layer);

  // init hands
  hands_layer = layer_create(bounds);
  layer_set_update_proc(hands_layer, hands_update_proc);
  layer_add_child(window_layer, hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(simple_bg_layer);
  layer_destroy(hands_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  // init hand paths
  minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  center.y += 8; // Dunno why this is necessary D:
  gpath_move_to(minute_arrow, center);
  gpath_move_to(hour_arrow, center);

  // Push the window onto the stack
  const bool animated = true;
  window_stack_push(window, animated);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit(void) {
  gpath_destroy(minute_arrow);
  gpath_destroy(hour_arrow);

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
