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
#define TEMP_F 13

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
    int temp_f;
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
    .icon_battery = true,
    .temp_f = 0
};

static Window *window;

int bitmap_count = 14;
static GBitmap *bitmap[14];

static Layer *standby_layer;
static Layer *info_layer;

int current_screen = 0;
AppTimer *standby_timer = NULL;
AppTimer *weather_timer = NULL;

int cond_t = 99;
//int cond_icon = 0;
char cond_icon[1];

char cond_city[32];
char message[80];
char temp_scale;
bool show_temp = true;
GFont weather_icon;
GFont font_days;
GFont font_time;

static bool send_request() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    if (iter == NULL) {
        return false;
    };

    Tuplet value0 = TupletInteger(W_KEY, settings.w_key);
    dict_write_tuplet(iter, &value0);

    Tuplet value1 = TupletInteger(TEMP_F, settings.temp_f);
    dict_write_tuplet(iter, &value1);
  
  
    dict_write_end(iter);
    app_message_outbox_send();

    //vibes_short_pulse();

    return true;
}

static void weather_callback() {
    send_request();
    weather_timer = app_timer_register(900000, weather_callback, NULL); // 15 min
}

static void in_received_handler(DictionaryIterator *received, void *context) {
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
    Tuple *temp_f_tuple = dict_find(received, TEMP_F);

    if (temp_f_tuple) {
        settings.temp_f = temp_f_tuple->value->int16;
    };
  
    if (key_tuple) {
        settings.w_key = key_tuple->value->int32;

        // Update weather after submit settings page
        send_request();
    };
    if (temp_tuple) {
        cond_t = temp_tuple->value->int16;
    };
    if (icon_tuple) {
        //cond_icon = icon_tuple->value->int16;
        strcpy(cond_icon, icon_tuple->value->cstring);
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
}

static void app_message_init() {
    app_message_register_inbox_received(in_received_handler);

    const uint32_t inbound_size = 128;
    const uint32_t outbound_size = 128;

    app_message_open(inbound_size, outbound_size);
}

static void load_resources() {
    unsigned char RESOURCE[14] = {RESOURCE_ID_DIGITS_TIME, RESOURCE_ID_SECONDS, RESOURCE_ID_BACKGROUND, RESOURCE_ID_DAYS, RESOURCE_ID_MONTHS, RESOURCE_ID_BATTERY, RESOURCE_ID_BLUETOOTH, RESOURCE_ID_AMPM, RESOURCE_ID_WEATHER_BG, RESOURCE_ID_WEATHER_BG_TOP, RESOURCE_ID_DIGITS_DAY, RESOURCE_ID_DAYS_EN, RESOURCE_ID_MONTHS_EN, RESOURCE_ID_DAYS_BG};
    for (int i=0; i<bitmap_count; ++i) {
        bitmap[i] = gbitmap_create_with_resource(RESOURCE[i]);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "%d %d", i, resource_size(resource_get_handle(RESOURCE[i])));  

    }
  
    weather_icon = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_26));
/*  
    font_days = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BERNARD_38));
    font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BERNARD_42));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", resource_size(resource_get_handle(RESOURCE_ID_FONT_WEATHER_26)));  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", resource_size(resource_get_handle(RESOURCE_ID_FONT_BERNARD_38)));  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", resource_size(resource_get_handle(RESOURCE_ID_FONT_BERNARD_42)));  
*/
}

static void destroy_resources() {
    for (int i=0; i<bitmap_count; ++i) {
        gbitmap_destroy(bitmap[i]);
    }
  
    fonts_unload_custom_font(weather_icon);
    fonts_unload_custom_font(font_days);
    fonts_unload_custom_font(font_time);
}

static void draw_picture(GContext* ctx, GBitmap **sources, GRect bounces, int number) {
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
  #ifdef PBL_PLATFORM_APLITE
    if (settings.s_standby_i) {
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
  #elif PBL_PLATFORM_BASALT
      // Цвет фона - черный
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_context_set_stroke_color(ctx, GColorChromeYellow);
      graphics_context_set_text_color(ctx, GColorBlack);
      // Режим композитинга - нормальный
      graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  #endif  

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
        draw_picture(ctx, &bitmap[7], GRect(62, 108, 21, 11), ampm);

        if (tick_time->tm_hour == 0) { tick_time->tm_hour = 12; };
        if (tick_time->tm_hour > 12) { tick_time->tm_hour -= 12; };
    };

    // Minutes
    draw_picture(ctx, &bitmap[0], GRect(78, 61, 29, 43), tick_time->tm_min/10);
    draw_picture(ctx, &bitmap[0], GRect(110, 61, 29, 43), tick_time->tm_min%10);

    // Hours
    draw_picture(ctx, &bitmap[0], GRect(6, 61, 29, 43), tick_time->tm_hour/10);
    draw_picture(ctx, &bitmap[0], GRect(38, 61, 29, 43), tick_time->tm_hour%10);

/*
    // Minutes
    draw_picture(ctx, &bitmap[0], GRect(78, 61, 29, 43), 0);
    snprintf(message, sizeof(message), "%d", tick_time->tm_min/10);
    graphics_draw_text(ctx,
                       message,
                       font_time,
                       GRect(78, 57, 29, 43),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    );

    draw_picture(ctx, &bitmap[0], GRect(110, 61, 29, 43), 0);
    snprintf(message, sizeof(message), "%d", tick_time->tm_min%10);
    graphics_draw_text(ctx,
                       message,
                       font_time,
                       GRect(110, 57, 29, 43),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    ); 
  
    // Hours
    draw_picture(ctx, &bitmap[0], GRect(6, 61, 29, 43), 0);
    snprintf(message, sizeof(message), "%d", tick_time->tm_hour/10);
    graphics_draw_text(ctx,
                       message,
                       font_time,
                       GRect(6, 57, 29, 43),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    );
  
    draw_picture(ctx, &bitmap[0], GRect(38, 61, 29, 43), 0);
    snprintf(message, sizeof(message), "%d", tick_time->tm_hour%10);
    graphics_draw_text(ctx,
                       message,
                       font_time,
                       GRect(38, 57, 29, 43),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    );
*/  
  
  // Рисуем разделитель
  #ifdef PBL_PLATFORM_APLITE
    if (settings.s_standby_i) {
          graphics_context_set_fill_color(ctx, GColorBlack);
    } else {
          graphics_context_set_fill_color(ctx, GColorWhite);
    };
  #elif PBL_PLATFORM_BASALT
    graphics_context_set_fill_color(ctx, GColorWhite);
  #endif
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
      #ifdef PBL_PLATFORM_BASALT
        graphics_context_set_fill_color(ctx, GColorChromeYellow);
      #endif
      
      frame = (GRect) {
          .origin = GPoint(20, 107),
          .size = GSize(104, 1)
      };
      graphics_fill_rect(ctx, frame, 0, GCornerNone);
    }
}

static void update_info(Layer *layer, GContext* ctx) {
    GRect bounds = layer_get_bounds(layer);

  #ifdef PBL_PLATFORM_APLITE
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
  #elif PBL_PLATFORM_BASALT
      // Цвет фона - черный
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_context_set_stroke_color(ctx, GColorChromeYellow);
      graphics_context_set_text_color(ctx, GColorBlack);
      // Режим композитинга - нормальный
      graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  #endif  

    // Заливаем слой
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    draw_picture(ctx, &bitmap[2], GRect(51, 60, 43, 13), 0);
  
    graphics_draw_circle(ctx, GPoint(72, 88), 30);

    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

  // Дата
    // Day number
    draw_picture(ctx, &bitmap[10], GRect(54, 75, 17, 31), tick_time->tm_mday/10);
    draw_picture(ctx, &bitmap[10], GRect(74, 75, 17, 31), tick_time->tm_mday%10);
/*  
    snprintf(message, sizeof(message), "%d%d", tick_time->tm_mday/10, tick_time->tm_mday%10);
    graphics_draw_text(ctx,
                       message,
                       font_days,
                       GRect(50, 69, 44, 40),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    );
*/
    // Day name
    draw_picture(ctx, &bitmap[13], GRect(1, 70, 39, 37), 0);
    if (settings.s_ru_lang) {
      draw_picture(ctx, &bitmap[3], GRect(1, 84, 39, 9), tick_time->tm_wday);
    } else {
      draw_picture(ctx, &bitmap[11], GRect(1, 84, 39, 9), tick_time->tm_wday);
    }
  
    // Month
    draw_picture(ctx, &bitmap[13], GRect(105, 70, 39, 37), 1);
    if (settings.s_ru_lang) {
      draw_picture(ctx, &bitmap[4], GRect(105, 84, 39, 9), tick_time->tm_mon);
    } else {
      draw_picture(ctx, &bitmap[12], GRect(105, 84, 39, 9), tick_time->tm_mon);
    }
  
  // Время
    // AM/PM
    int ampm = 0;
    if (!clock_is_24h_style()) {
        if ( (tick_time->tm_hour - 12) >= 0 ) {
            ampm = 1;
        } else {
            ampm = 0;
        };
        draw_picture(ctx, &bitmap[7], GRect(62, 0, 21, 11), ampm);

        if (tick_time->tm_hour == 0) { tick_time->tm_hour = 12; };
        if (tick_time->tm_hour > 12) { tick_time->tm_hour -= 12; };
    };

    // Seconds
    draw_picture(ctx, &bitmap[1], GRect(54, 61, 37, 10), tick_time->tm_sec);

    // Minutes
    draw_picture(ctx, &bitmap[0], GRect(78, 13, 29, 43), tick_time->tm_min/10);
    draw_picture(ctx, &bitmap[0], GRect(110, 13, 29, 43), tick_time->tm_min%10);

    // Hours
    draw_picture(ctx, &bitmap[0], GRect(6, 13, 29, 43), tick_time->tm_hour/10);
    draw_picture(ctx, &bitmap[0], GRect(38, 13, 29, 43), tick_time->tm_hour%10);
  
/*  
    // Minutes
    draw_picture(ctx, &bitmap[0], GRect(78, 13, 29, 43), 0);
    snprintf(message, sizeof(message), "%d", tick_time->tm_min/10);
    graphics_draw_text(ctx,
                       message,
                       font_time,
                       GRect(78, 9, 29, 43),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    );

    draw_picture(ctx, &bitmap[0], GRect(110, 13, 29, 43), 0);
    snprintf(message, sizeof(message), "%d", tick_time->tm_min%10);
    graphics_draw_text(ctx,
                       message,
                       font_time,
                       GRect(110, 9, 29, 43),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    );
  
    // Hours
    draw_picture(ctx, &bitmap[0], GRect(6, 13, 29, 43), 0);
    snprintf(message, sizeof(message), "%d", tick_time->tm_hour/10);
    graphics_draw_text(ctx,
                       message,
                       font_time,
                       GRect(6, 9, 29, 43),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    );
  
    draw_picture(ctx, &bitmap[0], GRect(38, 13, 29, 43), 0);
    snprintf(message, sizeof(message), "%d", tick_time->tm_hour%10);
    graphics_draw_text(ctx,
                       message,
                       font_time,
                       GRect(38, 9, 29, 43),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
    );
*/

  // Weather background lines
    #ifdef PBL_PLATFORM_BASALT
      graphics_context_set_fill_color(ctx, GColorBulgarianRose);
      GRect basalt_frame = (GRect) {
        .origin = GPoint(22, 147),
        .size = GSize(102, 21)
      };
      graphics_fill_rect(ctx, basalt_frame, 0, GCornerNone);
    #endif  

    
  // Рисуем разделитель
  #ifdef PBL_PLATFORM_APLITE
    if (settings.s_info_i) {
          graphics_context_set_fill_color(ctx, GColorBlack);
    } else {
          graphics_context_set_fill_color(ctx, GColorWhite);
    };
  #elif PBL_PLATFORM_BASALT
    graphics_context_set_fill_color(ctx, GColorWhite);
  #endif  

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
    #ifdef PBL_PLATFORM_BASALT
      graphics_context_set_fill_color(ctx, GColorChromeYellow);
    #endif  

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




    // Батарейка
    if (settings.icon_battery) {
        BatteryChargeState charge_state = battery_state_service_peek();
        int bat_percent = charge_state.charge_percent/10;
        if (charge_state.is_charging) {
            bat_percent = 110/10;
        };
        draw_picture(ctx, &bitmap[5], GRect(119, 3, 17, 7), bat_percent);
        draw_picture(ctx, &bitmap[6], GRect(136, 2, 18, 9), 0);
    };

    // bluetooth
    if (settings.icon_bt) {
        if (bluetooth_connection_service_peek()) {
            draw_picture(ctx, &bitmap[6], GRect(0, 2, 18, 9), 0);
        };
    };


    // Погода
    draw_picture(ctx, &bitmap[8], GRect(29, 139, 87, 29), 0);
    draw_picture(ctx, &bitmap[9], GRect(29, 122, 87, 17), 0);

    if (cond_t < 99) {
        graphics_draw_text(ctx,
                           cond_icon,
                           weather_icon,
                           GRect(23, 123, 99, 16),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentCenter,
                           NULL
        );
      
      
      
        if (settings.temp_f == 1) {
            temp_scale = 'F';
        } else {
            temp_scale = 'C';
        }

        if (tick_time->tm_sec == 0 || tick_time->tm_sec == 20 || tick_time->tm_sec == 40) {
            show_temp = !show_temp;
        }
      
        // Город
        if (strlen(cond_city) <= 10) {
            if (show_temp) {
                snprintf(message, sizeof(message), "%s", cond_city);
            } else {
                snprintf(message, sizeof(message), "%d °%c", cond_t, temp_scale);
            }
        } else {
            snprintf(message, sizeof(message), "%d °%c", cond_t, temp_scale);
        }
          
        graphics_draw_text(ctx,
                           message,
                           fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                           GRect(23, 153, 99, 16),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentCenter,
                           NULL
        );
    };
  
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Width = %d", heap_bytes_used());  

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    if (units_changed & MINUTE_UNIT) {
        layer_mark_dirty(standby_layer);
    };
    if (units_changed & HOUR_UNIT) {
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
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  show_temp = !show_temp;
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

  // weather update on watchface load
  weather_timer = app_timer_register(3000, weather_callback, NULL);

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
  app_event_loop();
  deinit();
}