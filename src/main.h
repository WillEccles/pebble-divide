#pragma once
#include <pebble.h>
	
	// not sure this contains all methods, but the ones it needs to contain are there
static void init();
static void deinit();
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void update_clock();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void canvas_update_proc(Layer *layer, GContext *ctx);
static void battery_update_proc(Layer *layer, GContext *ctx);
static void battery_handler(BatteryChargeState charge);