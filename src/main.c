#include <pebble.h>

#define W_KEY 1
#define W_TEMP 2
#define W_ICON 3
#define W_CITY 4
#define VIBE_BT 5
#define VIBE_HOURLY 6
#define S_STANDBY_I 7
#define S_INFO_I 8
#define S_RU_LANG 9
#define S_AUTO 10
#define ICON_BT 11
#define ICON_BATTERY 12

#define STORAGE_KEY 99

typedef struct persist {
    int w_key;
    bool vibe_bt;
    bool vibe_hourly;
    bool s_standby_i;
    bool s_info_i;
    bool s_ru_lang;
    bool s_auto;
    bool icon_bt;
    bool icon_battery;
} __attribute__((__packed__)) persist;

persist settings = {
    .w_key = 588409,
    .vibe_bt = true,
    .vibe_hourly = false,
    .s_standby_i = false,
    .s_info_i = false,
    .s_ru_lang = true,
    .s_auto = false,
    .icon_bt = true,
    .icon_battery = true
};

static Window *window;

static GBitmap *bitmap[13];

static Layer *standby_layer;
static Layer *info_layer;

int current_screen = 0;
AppTimer *standby_timer = NULL;

int cond_t = 99;
int cond_icon = 0;
char cond_city[32];

static bool send_request() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    if (iter == NULL) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Iter is NULL.");
        return false;
vibes_double_pulse();
    };

    Tuplet value0 = TupletInteger(W_KEY, settings.w_key);
    dict_write_tuplet(iter, &value0);

    dict_write_end(iter);
    app_message_outbox_send();

vibes_short_pulse();
 
    return true;
}

void in_received_handler(DictionaryIterator *received, void *context) {
    Tuple *key_tuple = dict_find(received, W_KEY);
    Tuple *temp_tuple = dict_find(received, W_TEMP);
    Tuple *icon_tuple = dict_find(received, W_ICON);
    Tuple *city_tuple = dict_find(received, W_CITY);
    Tuple *vibe_bt_tuple = dict_find(received, VIBE_BT);
    Tuple *vibe_hourly_tuple = dict_find(received, VIBE_HOURLY);
    Tuple *s_standby_i_tuple = dict_find(received, S_STANDBY_I);
    Tuple *s_info_i_tuple = dict_find(received, S_INFO_I);
    Tuple *s_ru_lang_tuple = dict_find(received, S_RU_LANG);
    Tuple *s_auto_tuple = dict_find(received, S_AUTO);
    Tuple *icon_bt_tuple = dict_find(received, ICON_BT);
    Tuple *icon_battery_tuple = dict_find(received, ICON_BATTERY);

    if (key_tuple) {
        settings.w_key = key_tuple->value->int32;
    };
    if (temp_tuple) {
        cond_t = temp_tuple->value->int16;
    };
    if (icon_tuple) {
        cond_icon = icon_tuple->value->int16;
    };
    if (city_tuple) {
        strcpy(cond_city, city_tuple->value->cstring);
    };
    if (vibe_bt_tuple) {
        settings.vibe_bt = (bool)vibe_bt_tuple->value->int16;
    };
    if (vibe_hourly_tuple) {
        settings.vibe_hourly = (bool)vibe_hourly_tuple->value->int16;
    };
    if (s_standby_i_tuple) {
        settings.s_standby_i = (bool)s_standby_i_tuple->value->int16;
    };
    if (s_info_i_tuple) {
        settings.s_info_i = (bool)s_info_i_tuple->value->int16;
    };
    if (s_ru_lang_tuple) {
        settings.s_ru_lang = (bool)s_ru_lang_tuple->value->int16;
    };
    if (s_auto_tuple) {
        settings.s_auto = (bool)s_auto_tuple->value->int16;
    };
    if (icon_bt_tuple) {
        settings.icon_bt = (bool)icon_bt_tuple->value->int16;
    };
    if (icon_battery_tuple) {
        settings.icon_battery = (bool)icon_battery_tuple->value->int16;
    };
    // Update weather after submit settings page
    send_request();
}

static void app_message_init() {
    app_message_register_inbox_received(in_received_handler);

    const uint32_t inbound_size = 128;
    const uint32_t outbound_size = 128;

    app_message_open(inbound_size, outbound_size);
}

static void load_resources() {
    unsigned char RESOURCE[13] = {RESOURCE_ID_DIGITS_CLOCK, RESOURCE_ID_DIGITS_DATE, RESOURCE_ID_SECONDS, RESOURCE_ID_BACKGROUND, RESOURCE_ID_DAYS, RESOURCE_ID_DAYS_EN, RESOURCE_ID_MONTHS, RESOURCE_ID_MONTHS_EN, RESOURCE_ID_BATTERY, RESOURCE_ID_BLUETOOTH, RESOURCE_ID_AMPM, RESOURCE_ID_WEATHER, RESOURCE_ID_WEATHER_BG};
    for (int i=0; i<13; ++i) {
        bitmap[i] = gbitmap_create_with_resource(RESOURCE[i] );
    }  
}

static void destroy_resources() {
    for (int i=0; i<13; ++i) {
        gbitmap_destroy(bitmap[i]);
    }  
}

static void draw_picture(GContext* ctx, GBitmap **sources, GRect bounces,
                         int number) {
    GPoint origin = bounces.origin;
    bounces.origin = GPoint(bounces.size.w*number, 0);
    GBitmap* temp = gbitmap_create_as_sub_bitmap(*sources, bounces);
    bounces.origin = origin;
    graphics_draw_bitmap_in_rect(ctx, temp, bounces);
    gbitmap_destroy(temp);
}

static void timer_callback() {
    current_screen = 0;
}

static void update_standby(Layer *layer, GContext* ctx) {
    GRect bounds = layer_get_bounds(layer);

    if (settings.s_standby_i) {
        // Цвет фона - белый
        graphics_context_set_fill_color(ctx, GColorWhite);
        // Режим композитинга - инвернтный
        graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
    } else {
        // Цвет фона - черный
        graphics_context_set_fill_color(ctx, GColorBlack);
        // Режим композитинга - нормальный
        graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    };
    // Заливаем слой
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

  // Время
    // AM/PM
    int ampm = 0;
    if (!clock_is_24h_style()) {
        if ( (tick_time->tm_hour - 12) >= 0 ) {
            ampm = 1;
        } else {
            ampm = 0;
        };
        draw_picture(ctx, &bitmap[10], GRect(62, 108, 21, 11), ampm);

        if (tick_time->tm_hour == 0) { tick_time->tm_hour = 12; };
        if (tick_time->tm_hour > 12) { tick_time->tm_hour -= 12; };
    };
  
    // Minutes
    int min_dicker = tick_time->tm_min/10;
    int min_unit = tick_time->tm_min%10;
    draw_picture(ctx, &bitmap[0], GRect(78, 61, 29, 43), min_dicker);
    draw_picture(ctx, &bitmap[0], GRect(110, 61, 29, 43), min_unit);
  
    // Hours
    int hour_dicker = tick_time->tm_hour/10;
    int hour_unit = tick_time->tm_hour%10;
    if (hour_dicker) {
        draw_picture(ctx, &bitmap[0], GRect(6, 61, 29, 43), hour_dicker);
    } else {
        draw_picture(ctx, &bitmap[0], GRect(6, 61, 29, 43), 0);
    };
    draw_picture(ctx, &bitmap[0], GRect(38, 61, 29, 43), hour_unit);
  
  // Рисуем разделитель
    if (settings.s_standby_i) {
          graphics_context_set_fill_color(ctx, GColorBlack);
    } else {
          graphics_context_set_fill_color(ctx, GColorWhite);
    };
    GRect frame = (GRect) {
        .origin = GPoint(70, 74),
        .size = GSize(5, 5)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(70, 86),
        .size = GSize(5, 5)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);

    // AM/PM
    if (!clock_is_24h_style()) {
      frame = (GRect) {
          .origin = GPoint(20, 107),
          .size = GSize(104, 1)
      };
      graphics_fill_rect(ctx, frame, 0, GCornerNone);
    }
}

static void update_info(Layer *layer, GContext* ctx) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Info redraw");
    GRect bounds = layer_get_bounds(layer);

    if (settings.s_info_i) {
        // Цвет фона - белый
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_context_set_text_color(ctx, GColorWhite);
        // Режим композитинга - инвернтный
        graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
    } else {
        // Цвет фона - черный
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_text_color(ctx, GColorBlack);
        // Режим композитинга - нормальный
        graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    };

    // Заливаем слой
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    draw_picture(ctx, &bitmap[3], GRect(51, 60, 43, 13), 0);
    graphics_draw_circle(ctx, GPoint(72, 88), 30);
  
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);


  // Время
    // AM/PM
    int ampm = 0;
    if (!clock_is_24h_style()) {
        if ( (tick_time->tm_hour - 12) >= 0 ) {
            ampm = 1;
        } else {
            ampm = 0;
        };
        draw_picture(ctx, &bitmap[10], GRect(62, 0, 21, 11), ampm);
        
        if (tick_time->tm_hour == 0) { tick_time->tm_hour = 12; };
        if (tick_time->tm_hour > 12) { tick_time->tm_hour -= 12; };
    };

    // Seconds
    draw_picture(ctx, &bitmap[2], GRect(54, 61, 37, 10), tick_time->tm_sec);
  
    // Minutes
    int min_dicker = tick_time->tm_min/10;
    int min_unit = tick_time->tm_min%10;
    draw_picture(ctx, &bitmap[0], GRect(78, 13, 29, 43), min_dicker);
    draw_picture(ctx, &bitmap[0], GRect(110, 13, 29, 43), min_unit);
  
    // Hours
    int hour_dicker = tick_time->tm_hour/10;
    int hour_unit = tick_time->tm_hour%10;
    if (hour_dicker) {
        draw_picture(ctx, &bitmap[0], GRect(6, 13, 29, 43), hour_dicker);
    } else {
        draw_picture(ctx, &bitmap[0], GRect(6, 13, 29, 43), 0);
    };
    draw_picture(ctx, &bitmap[0], GRect(38, 13, 29, 43), hour_unit);
  
  // Рисуем разделитель
    if (settings.s_info_i) {
          graphics_context_set_fill_color(ctx, GColorBlack);
    } else {
          graphics_context_set_fill_color(ctx, GColorWhite);
    };
    GRect frame = (GRect) {
        .origin = GPoint(70, 26),
        .size = GSize(5, 5)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(70, 38),
        .size = GSize(5, 5)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);

    // Weather background lines
    frame = (GRect) {
        .origin = GPoint(17, 149),
        .size = GSize(112, 1)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(16, 152),
        .size = GSize(114, 1)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(15, 155),
        .size = GSize(116, 1)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(14, 158),
        .size = GSize(118, 1)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(13, 161),
        .size = GSize(120, 1)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(12, 164),
        .size = GSize(122, 1)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(11, 167),
        .size = GSize(124, 1)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
  
  
  
  // Дата
    // Day number
    int day_dicker = tick_time->tm_mday/10;
    int day_unit = tick_time->tm_mday%10;
    draw_picture(ctx, &bitmap[1], GRect(54, 75, 17, 31), day_dicker);
    draw_picture(ctx, &bitmap[1], GRect(74, 75, 17, 31), day_unit);

    // Day name
    int w_day = tick_time->tm_wday;
    if (settings.s_ru_lang) {
        draw_picture(ctx, &bitmap[4], GRect(1, 75, 39, 37), w_day);
    } else {
        draw_picture(ctx, &bitmap[5], GRect(1, 75, 39, 37), w_day);
    }
  
  
    // Month
    if (settings.s_ru_lang) {
        draw_picture(ctx, &bitmap[6], GRect(105, 75, 39, 37), tick_time->tm_mon);
    } else {
        draw_picture(ctx, &bitmap[7], GRect(105, 75, 39, 37), tick_time->tm_mon);
    }
  
 
  
    // Батарейка
    if (settings.icon_battery) {
        BatteryChargeState charge_state = battery_state_service_peek();
        int bat_percent = charge_state.charge_percent/10;
        if (charge_state.is_charging) {
            bat_percent = 110/10;
        };
        draw_picture(ctx, &bitmap[8], GRect(119, 3, 17, 7), bat_percent);
        draw_picture(ctx, &bitmap[9], GRect(136, 2, 18, 9), 0);
    };
      
    // bluetooth
    if (settings.icon_bt) {
        if (bluetooth_connection_service_peek()) {
            draw_picture(ctx, &bitmap[9], GRect(0, 2, 18, 9), 0);
        };
    };

    // Погода
    draw_picture(ctx, &bitmap[12], GRect(29, 122, 87, 46), 0);

    if (cond_t < 99) {
        draw_picture(ctx, &bitmap[11], GRect(32, 126, 81, 31), cond_icon);
        // Город
        char message[80] = " ";
        if (strlen(cond_city) <= 8) {
          snprintf(message, sizeof(message), "%s %d", cond_city, cond_t);
        } else {
          snprintf(message, sizeof(message), "%d", cond_t);
        }
        graphics_draw_text(ctx,
                           message,
                           fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                           GRect(23, 152, 99, 16),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentCenter,
                           NULL
        );
    };
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    if (units_changed & MINUTE_UNIT) {
        layer_mark_dirty(standby_layer);
    };
    if (units_changed & HOUR_UNIT) {
        // Update weather every hour
        send_request();
        if (settings.vibe_hourly) {
            vibes_long_pulse();
        };
    };
  if (settings.s_auto) {
    switch (current_screen) {
        case 0:
            if (layer_get_hidden(standby_layer)) {
                layer_set_hidden(info_layer, true);
                layer_set_hidden(standby_layer, false);
            };
            break;
        case 1:
            layer_mark_dirty(info_layer);
            if (layer_get_hidden(info_layer)) {
                layer_set_hidden(standby_layer, true);
                layer_set_hidden(info_layer, false);

                // Update weather on info screen activation
                send_request();

                if (settings.s_auto) {
                    standby_timer = app_timer_register(30000, timer_callback, NULL);
                };
            };
            break;
    };
  } else {
    layer_mark_dirty(info_layer);
    layer_set_hidden(standby_layer, true);
    layer_set_hidden(info_layer, false);
    //send_request();
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (settings.s_auto) {
    current_screen = !current_screen;
  }
}

void bt_handler(bool connected) {
    if (settings.vibe_bt) {
        vibes_long_pulse();
    };
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    load_resources();

  if (settings.s_auto) {
    standby_layer = layer_create(bounds);
    layer_add_child(window_layer, standby_layer);
    layer_set_update_proc(standby_layer, update_standby);
    info_layer = layer_create(bounds);
    layer_set_hidden(info_layer, true);
    layer_add_child(window_layer, info_layer);
    layer_set_update_proc(info_layer, update_info);
  } else {
    // Update weather on watchface load
    send_request();
    info_layer = layer_create(bounds);
    layer_add_child(window_layer, info_layer);
    layer_set_update_proc(info_layer, update_info);
    standby_layer = layer_create(bounds);
    layer_set_hidden(standby_layer, true);
    layer_add_child(window_layer, standby_layer);
    layer_set_update_proc(standby_layer, update_standby);
  }
}

static void window_unload(Window *window) {
    layer_destroy(standby_layer);
    layer_destroy(info_layer);
    destroy_resources();
}

static void init(void) {
  if (persist_exists(STORAGE_KEY)) {
      persist_read_data(STORAGE_KEY, &settings, sizeof(settings));
  } else {
      persist_write_data(STORAGE_KEY, &settings, sizeof(settings));
  };
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;

  app_message_init();

  window_stack_push(window, animated);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  accel_tap_service_subscribe(tap_handler);
  bluetooth_connection_service_subscribe(bt_handler);
}

static void deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  persist_write_data(STORAGE_KEY, &settings, sizeof(settings));
}

int main(void) {
  init();

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}