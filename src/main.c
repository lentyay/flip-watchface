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

#define STORAGE_KEY 99

typedef struct persist {
    int w_key;
    bool vibe_bt;
    bool vibe_hourly;
    bool s_standby_i;
    bool s_info_i;
    bool s_ru_lang;
    bool s_auto;
} __attribute__((__packed__)) persist;

persist settings = {
    .w_key = 601339,
    .vibe_bt = true,
    .vibe_hourly = false,
    .s_standby_i = false,
    .s_info_i = false,
    .s_ru_lang = false,
    .s_auto = true
};

static Window *window;

static GBitmap *bmp_digits_clock;
static GBitmap *bmp_digits_date;
static GBitmap *bmp_seconds;
static GBitmap *bmp_background;
static GBitmap *bmp_days;
static GBitmap *bmp_days_en;
static GBitmap *bmp_months;
static GBitmap *bmp_months_en;
static GBitmap *bmp_battery;
static GBitmap *bmp_bluetooth;
static GBitmap *bmp_ampm;




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
    };

    Tuplet value0 = TupletInteger(W_KEY, settings.w_key);
    dict_write_tuplet(iter, &value0);

    dict_write_end(iter);
    app_message_outbox_send();

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
}

static void app_message_init() {
    app_message_register_inbox_received(in_received_handler);

    const uint32_t inbound_size = 128;
    const uint32_t outbound_size = 128;

    app_message_open(inbound_size, outbound_size);
}

static void load_resources() {
    bmp_digits_clock = gbitmap_create_with_resource(RESOURCE_ID_DIGITS_CLOCK);
    bmp_digits_date = gbitmap_create_with_resource(RESOURCE_ID_DIGITS_DATE);
    bmp_seconds = gbitmap_create_with_resource(RESOURCE_ID_SECONDS);
    bmp_background = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
    bmp_days = gbitmap_create_with_resource(RESOURCE_ID_DAYS);
    bmp_days_en = gbitmap_create_with_resource(RESOURCE_ID_DAYS_EN);
    bmp_months = gbitmap_create_with_resource(RESOURCE_ID_MONTHS);
    bmp_months_en = gbitmap_create_with_resource(RESOURCE_ID_MONTHS_EN);
    bmp_battery = gbitmap_create_with_resource(RESOURCE_ID_BATTERY);
    bmp_bluetooth = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH);
    bmp_ampm = gbitmap_create_with_resource(RESOURCE_ID_AMPM);
}

static void destroy_resources() {
    gbitmap_destroy(bmp_digits_clock);
    gbitmap_destroy(bmp_digits_date);
    gbitmap_destroy(bmp_seconds);
    gbitmap_destroy(bmp_background);
    gbitmap_destroy(bmp_days);
    gbitmap_destroy(bmp_days_en);
    gbitmap_destroy(bmp_months);
    gbitmap_destroy(bmp_months_en);
    gbitmap_destroy(bmp_battery);
    gbitmap_destroy(bmp_bluetooth);
    gbitmap_destroy(bmp_ampm);
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
//    int ampm = 0;

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
        draw_picture(ctx, &bmp_ampm, GRect(62, 108, 20, 12), ampm);

        if (tick_time->tm_hour == 0) { tick_time->tm_hour = 12; };
        if (tick_time->tm_hour > 12) { tick_time->tm_hour -= 12; };
    };
  
    // Minutes
    int min_dicker = tick_time->tm_min/10;
    int min_unit = tick_time->tm_min%10;
    draw_picture(ctx, &bmp_digits_clock, GRect(77, 61, 29, 43), min_dicker);
    draw_picture(ctx, &bmp_digits_clock, GRect(109, 61, 29, 43), min_unit);

    // Hours
    int hour_dicker = tick_time->tm_hour/10;
    int hour_unit = tick_time->tm_hour%10;
    if (hour_dicker) {
        draw_picture(ctx, &bmp_digits_clock, GRect(5, 61, 29, 43), hour_dicker);
    } else {
        draw_picture(ctx, &bmp_digits_clock, GRect(5, 61, 29, 43), 0);
    };
    draw_picture(ctx, &bmp_digits_clock, GRect(38, 61, 29, 43), hour_unit);
  
  // Рисуем разделитель
    if (settings.s_standby_i) {
          graphics_context_set_fill_color(ctx, GColorBlack);
    } else {
          graphics_context_set_fill_color(ctx, GColorWhite);
    };
    GRect frame = (GRect) {
        .origin = GPoint(bounds.size.w/2-2, 74),
        .size = GSize(4, 4)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(bounds.size.w/2-2, 86),
        .size = GSize(4, 4)
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
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Info redraw");
    GRect bounds = layer_get_bounds(layer);

    if (settings.s_info_i) {
        // Цвет фона - белый
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        // Режим композитинга - инвернтный
        graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
    } else {
        // Цвет фона - черный
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        // Режим композитинга - нормальный
        graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    };

    // Заливаем слой
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    draw_picture(ctx, &bmp_background, GRect(41, 78, 62, 47), 0);
  
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
        draw_picture(ctx, &bmp_ampm, GRect(62, 0, 20, 12), ampm);
        
        if (tick_time->tm_hour == 0) { tick_time->tm_hour = 12; };
        if (tick_time->tm_hour > 12) { tick_time->tm_hour -= 12; };
    };

    // Seconds
    draw_picture(ctx, &bmp_seconds, GRect(45, 62, 54, 16), tick_time->tm_sec);
  
    // Minutes
    int min_dicker = tick_time->tm_min/10;
    int min_unit = tick_time->tm_min%10;
    draw_picture(ctx, &bmp_digits_clock, GRect(77, 15, 29, 43), min_dicker);
    draw_picture(ctx, &bmp_digits_clock, GRect(109, 15, 29, 43), min_unit);
  
    // Hours
    int hour_dicker = tick_time->tm_hour/10;
    int hour_unit = tick_time->tm_hour%10;
    if (hour_dicker) {
        draw_picture(ctx, &bmp_digits_clock, GRect(5, 15, 29, 43), hour_dicker);
    } else {
        draw_picture(ctx, &bmp_digits_clock, GRect(5, 15, 29, 43), 0);
    };
    draw_picture(ctx, &bmp_digits_clock, GRect(38, 15, 29, 43), hour_unit);
  
  // Рисуем разделитель
    if (settings.s_standby_i) {
          graphics_context_set_fill_color(ctx, GColorBlack);
    } else {
          graphics_context_set_fill_color(ctx, GColorWhite);
    };
    GRect frame = (GRect) {
        .origin = GPoint(bounds.size.w/2-2, 28),
        .size = GSize(4, 4)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(bounds.size.w/2-2, 40),
        .size = GSize(4, 4)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
  
  // Дата
    // Day number
    int day_dicker = tick_time->tm_mday/10;
    int day_unit = tick_time->tm_mday%10;
    draw_picture(ctx, &bmp_digits_date, GRect(53, 80, 18, 33), day_dicker);
    draw_picture(ctx, &bmp_digits_date, GRect(73, 80, 18, 33), day_unit);

    // Day name
    int w_day = tick_time->tm_wday;
    if (settings.s_ru_lang) {
        draw_picture(ctx, &bmp_days, GRect(1, 75, 39, 37), w_day);
    } else {
        draw_picture(ctx, &bmp_days_en, GRect(1, 75, 39, 37), w_day);
    }

    // Month
    if (settings.s_ru_lang) {
        draw_picture(ctx, &bmp_months, GRect(104, 75, 39, 37), tick_time->tm_mon);
    } else {
        draw_picture(ctx, &bmp_months_en, GRect(104, 75, 39, 37), tick_time->tm_mon);
    }
  
  
 
  
    // Батарейка
    BatteryChargeState charge_state = battery_state_service_peek();
    int bat_percent = charge_state.charge_percent/10;
    if (charge_state.is_charging) {
        bat_percent = 110/10;
    };
    draw_picture(ctx, &bmp_battery, GRect(119, 3, 25, 7), bat_percent);

    // bluetooth
    if (bluetooth_connection_service_peek()) {
        draw_picture(ctx, &bmp_bluetooth, GRect(0, 3, 18, 9), 0);
    };

/*  

    int mon_dicker = (tick_time->tm_mon+1)/10;
    int mon_unit = (tick_time->tm_mon+1)%10;
    int year_dicker = (tick_time->tm_year-100)/10;
    int year_unit = (tick_time->tm_year-100)%10;

    int y_day = tick_time->tm_yday + 1;
    int y_day_cent = y_day/100;
    int y_day_dicker = (y_day%100)/10;
    int y_day_unit = (y_day%100)%10;


    draw_picture(ctx, &bmp_digits_midi, GRect(34, 90, 8, 16), mon_dicker);
    draw_picture(ctx, &bmp_digits_midi, GRect(44, 90, 8, 16), mon_unit);

    draw_picture(ctx, &bmp_digits_midi, GRect(62, 90, 8, 16), year_dicker);
    draw_picture(ctx, &bmp_digits_midi, GRect(72, 90, 8, 16), year_unit);

    GRect frame = (GRect) {
        .origin = GPoint(28, 98),
        .size = GSize(4, 2)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
    frame = (GRect) {
        .origin = GPoint(56, 98),
        .size = GSize(4, 2)
    };
    graphics_fill_rect(ctx, frame, 0, GCornerNone);

    frame = (GRect) {
        .origin = GPoint(86, 90),
        .size = GSize(18, 16)
    };
    graphics_fill_rect(ctx, frame, 4, GCornersAll);

    draw_picture(ctx, &bmp_digits_midi, GRect(108, 90, 8, 16), y_day_cent);
    draw_picture(ctx, &bmp_digits_midi, GRect(118, 90, 8, 16), y_day_dicker);
    draw_picture(ctx, &bmp_digits_midi, GRect(128, 90, 8, 16), y_day_unit);

    // Разделитель
    graphics_draw_line(ctx, GPoint(24, 84), GPoint(120, 84));

    // Погода
    int x;

    if (cond_t < 99) {
        // Город
        graphics_draw_text(ctx,
                           cond_city,
                           fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                           GRect(5, 5, 144, 18 + 2),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentCenter,
                           NULL
        );

        // Десятки температуры
        if (abs(cond_t)/10 > 0) {
            draw_picture(ctx, &bmp_digits, GRect(78, 35, 20, 38),
                         abs(cond_t)/10);
        };
        // Единицы температуры
        draw_picture(ctx, &bmp_digits, GRect(100, 35, 20, 38),
                     abs(cond_t)%10);

        // Значок градус
        graphics_draw_circle(ctx, GPoint(124, 40), 2);

        if (cond_t < 0) {
            if (abs(cond_t)/10 > 0) {
                x = 66;
            } else {
                x = 86;
            };
            GRect frame = (GRect) {
                .origin = GPoint(x, 52),
                .size = GSize(8, 2)
            };
            graphics_fill_rect(ctx, frame, 0, GCornerNone);
        };
        draw_picture(ctx, &bmp_weather, GRect(21, 42, 32, 32), cond_icon);
    };
*/    
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
    //if (units_changed & SECOND_UNIT) {
        //layer_mark_dirty(info_layer);
    //};
    switch (current_screen) {
        case 0:
            //layer_set_hidden(info_layer, true);
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
                send_request();
                if (settings.s_auto) {
                    standby_timer = app_timer_register(30000, timer_callback, NULL);
                };
            };
            break;
    };
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
    current_screen = !current_screen;
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

    standby_layer = layer_create(bounds);
    layer_add_child(window_layer, standby_layer);
    layer_set_update_proc(standby_layer, update_standby);
    info_layer = layer_create(bounds);
    layer_set_hidden(info_layer, true);
    layer_add_child(window_layer, info_layer);
    layer_set_update_proc(info_layer, update_info);
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

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}