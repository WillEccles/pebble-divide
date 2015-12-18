//
// (c) Will Eccles, 2015
// 
// Type: Watch face
// Name: "Divide"
//

// For copy/pasta l8r on if I need it, here's the em-dash: —
// Note: display is 144x168

#include <pebble.h>
#include "main.h"
#include "math.h"

// Battery indicator colors. I'm a genius for putting them here.
#define BCOLOR_CHARGE GColorVividCerulean // charging color
#define BCOLOR_HIGH GColorIslamicGreen // >50%
#define BCOLOR_MED GColorChromeYellow // >25%, <=50%
#define BCOLOR_LOW GColorDarkCandyAppleRed // <=25%
	
// keys for color codes
#define BGCOLOR_KEY 0
#define TCOLOR_KEY 1
#define BCOLOR_KEY 2
	
// define keys for persistant storage
#define ST_BGCOLOR 0
#define ST_TCOLOR 1
#define ST_BCOLOR 2

// storage for bar color
#define BGCOLOR GColorFromHEX(persist_read_int(ST_BGCOLOR))
#define TCOLOR GColorFromHEX(persist_read_int(ST_TCOLOR))
#define BCOLOR GColorFromHEX(persist_read_int(ST_BCOLOR))
	
static Window *main_window; // main window
static Layer *canvas_layer;
static Layer *battery_layer; // layer for drawing battery bar to
static TextLayer *clock_layer;

static BatteryChargeState battery_state; // used for getting battery value

static GFont clock_font; // custom font for clock face

// load window
static void main_window_load(Window *window) {
	window_set_background_color(window, BGCOLOR);
	
	// create time text layer
	clock_layer = text_layer_create(GRect(0, 0, 144, 140));
	text_layer_set_background_color(clock_layer, BGCOLOR);
	text_layer_set_text_color(clock_layer, TCOLOR);
	text_layer_set_text(clock_layer, "00\n00");
	
	// load custom font
	clock_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PLAY_BOLD_65));
	
	// set text arrangement and such
	text_layer_set_font(clock_layer, clock_font);
	text_layer_set_text_alignment(clock_layer, GTextAlignmentCenter);
	
	// add clock face as child to window
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(clock_layer));
	layer_add_child(window_get_root_layer(window), canvas_layer);
	layer_add_child(window_get_root_layer(window), battery_layer);
	
	update_clock();
}

// Unload window
static void main_window_unload(Window *window) {
	text_layer_destroy(clock_layer);
	layer_destroy(canvas_layer);
	layer_destroy(battery_layer);
}

// recieve, store, and use incoming colors set in the config
static void inbox_recieved_callback(DictionaryIterator *iterator, void *context) {
	
	// read first item in dictionary
	Tuple *t = dict_read_first(iterator);
	
	// for all items
	while(t != NULL) {
		switch(t->key) {
			case BGCOLOR_KEY:
				persist_write_int(ST_BGCOLOR, (int)t->value->int32);
				window_set_background_color(main_window, BGCOLOR);
				text_layer_set_background_color(clock_layer, BGCOLOR);
				APP_LOG(APP_LOG_LEVEL_INFO, "Set background colors (BGCOLOR_KEY)");
				break;
			case TCOLOR_KEY:
				persist_write_int(ST_TCOLOR, (int)t->value->int32);
				text_layer_set_text_color(clock_layer, TCOLOR);
				APP_LOG(APP_LOG_LEVEL_INFO, "Set clock colors (TCOLOR_KEY)");
				break;
			case BCOLOR_KEY:
				persist_write_int(ST_BCOLOR, (int)t->value->int32);
				layer_mark_dirty(canvas_layer);
				APP_LOG(APP_LOG_LEVEL_INFO, "Set bar color (BCOLOR_KEY)");
				break;
		}
		
		// read next if any
		t = dict_read_next(iterator);
	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// update clock layer
static void update_clock() {
	// get a tm structure
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	// create long lived buffer
	static char buffer[] = "00\n00";
	
	// write current minutes into buffer
	if (clock_is_24h_style() == true) {
		// 24-hour time (e.g. 13:45)
		strftime(buffer, sizeof("00\n00"), "%H\n%M", tick_time);
	} else {
		// 12-hour time (e.g. 01:45)
		strftime(buffer, sizeof("00\n00"), "%I\n%M", tick_time);
	}
	
	// display time on text layers
	text_layer_set_text(clock_layer, buffer);
}

// This will update the time every minute to change the clock's text
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_clock();
}

// Layer update proc for the canvas layer used to draw dividing line
static void canvas_update_proc(Layer *layer, GContext *ctx) {
	graphics_context_set_fill_color(ctx, BCOLOR);
	graphics_fill_rect(ctx, GRect(16, 71, 112, 10), 0, GCornerNone);
}

// update the battery layer
static void battery_update_proc(Layer *layer, GContext *ctx) {
	// this should, in theory, change the bar to be the right width and color based on the battery percent
	
	int w = 0;
	int bp = battery_state.charge_percent;
	
	if (battery_state.is_charging == true) {
		w = 144;
		graphics_context_set_fill_color(ctx, BCOLOR_CHARGE);
	}
	else if (battery_state.is_plugged == true && battery_state.charge_percent == 100) {
		w = 144;
		graphics_context_set_fill_color(ctx, BCOLOR_CHARGE);
	}
	else {
		// Set the color of the bar.
		// All BCOLORs are defined at the top of the code, as they should be.
		if (bp >= 50) {
			graphics_context_set_fill_color(ctx, BCOLOR_HIGH);
		}
		if (bp >= 20 && bp < 50) {
			graphics_context_set_fill_color(ctx, BCOLOR_MED);
		}
		if (bp < 20) {
			graphics_context_set_fill_color(ctx, BCOLOR_LOW);
		}
		
		// Set width of bar after colors are set.
		if (bp >= 10) {
			float maths = floorf((((float)bp)/100.0) * 144.0); // NEEDS .0 for the math, which is why it didn't work
			w = (int)maths;
		}
		else {
			w = 6;
		}
	}
	
	graphics_fill_rect(ctx, GRect(0, 154, w, 14), 0, GCornerNone);
	
}

// handle battery state changes
static void battery_handler(BatteryChargeState charge) {
	// updates with battery status
	battery_state = charge;
	layer_mark_dirty(battery_layer);
}

// main initialization
static void init() {
	// set the default values for the colors in storage if they aren't already set
	if (persist_exists(ST_BGCOLOR) == false) {
		persist_write_int(ST_BGCOLOR, 0x000000);
		APP_LOG(APP_LOG_LEVEL_INFO, "Set BGCOLOR persistant data, wasn't found.");
	}
	if (persist_exists(ST_TCOLOR) == false) {
		persist_write_int(ST_TCOLOR, 0xFF0000);
		APP_LOG(APP_LOG_LEVEL_INFO, "Set TCOLOR persistant data, wasn't found.");
	}
	if (persist_exists(ST_BCOLOR) == false) {
		persist_write_int(ST_BCOLOR, 0xFF0000);
		APP_LOG(APP_LOG_LEVEL_INFO, "Set BCOLOR persistant data, wasn't found.");
	}
	
	main_window = window_create(); // make a window, assign to pointer
	canvas_layer = layer_create(GRect(0, 0, 144, 168));
	battery_layer = layer_create(GRect(0, 0, 144, 168));
	
	layer_set_update_proc(canvas_layer, canvas_update_proc);
	layer_set_update_proc(battery_layer, battery_update_proc);
	
	// Window handlers:
	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	// show window on the watch, animated true
	window_stack_push(main_window, true);
	
	// register with tick timer service
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	battery_state_service_subscribe(battery_handler);
	battery_state = battery_state_service_peek();
	
	// register callbacks for AppMessages
	app_message_register_inbox_received(inbox_recieved_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
	
	// open appmessage
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	
}

// deinit app
static void deinit() {
	// destroy window.
	window_destroy(main_window);
}

// main. what do you think it does, silly?
int main(void) {
	init();
	app_event_loop();
	deinit();
}