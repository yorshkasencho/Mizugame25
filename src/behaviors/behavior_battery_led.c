#define DT_DRV_COMPAT zmk_behavior_battery_led

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <errno.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/battery.h>

/*
 * HoshinoUtsuro Battery / Status LED behavior
 *
 * XIAO nRF52840 onboard RGB USER LED:
 *   led0 / Red   = P0.26 GPIO_ACTIVE_LOW
 *   led1 / Green = P0.30 GPIO_ACTIVE_LOW
 *   led2 / Blue  = P0.06 GPIO_ACTIVE_LOW
 *
 * gpio_pin_set_dt() respects GPIO_ACTIVE_LOW:
 *   logical 1 = ON
 *   logical 0 = OFF
 */

#define LED_MODE_BATTERY     0
#define LED_MODE_CHECK       1
#define LED_MODE_DRAW        2
#define LED_MODE_DEFAULT     3
#define LED_MODE_KANA        4
#define LED_MODE_EISU        5

enum led_color {
    LED_COLOR_OFF,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_YELLOW,
    LED_COLOR_CYAN,
    LED_COLOR_PURPLE,
    LED_COLOR_WHITE,
};

enum led_pattern {
    LED_PATTERN_NONE,
    LED_PATTERN_BATTERY_BLINK,
    LED_PATTERN_CHECK,
    LED_PATTERN_TRIPLE,
    LED_PATTERN_SINGLE,
};

static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);
static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(DT_NODELABEL(led2), gpios);

static struct k_work_delayable led_work;

static enum led_pattern current_pattern = LED_PATTERN_NONE;
static enum led_color current_color = LED_COLOR_OFF;
static int step = 0;

static void show_color(enum led_color color) {
    gpio_pin_set_dt(&led_red, 0);
    gpio_pin_set_dt(&led_green, 0);
    gpio_pin_set_dt(&led_blue, 0);

    switch (color) {
    case LED_COLOR_RED:
        gpio_pin_set_dt(&led_red, 1);
        break;

    case LED_COLOR_GREEN:
        gpio_pin_set_dt(&led_green, 1);
        break;

    case LED_COLOR_BLUE:
        gpio_pin_set_dt(&led_blue, 1);
        break;

    case LED_COLOR_YELLOW:
        gpio_pin_set_dt(&led_red, 1);
        gpio_pin_set_dt(&led_green, 1);
        break;

    case LED_COLOR_CYAN:
        gpio_pin_set_dt(&led_green, 1);
        gpio_pin_set_dt(&led_blue, 1);
        break;

    case LED_COLOR_PURPLE:
        gpio_pin_set_dt(&led_red, 1);
        gpio_pin_set_dt(&led_blue, 1);
        break;

    case LED_COLOR_WHITE:
        gpio_pin_set_dt(&led_red, 1);
        gpio_pin_set_dt(&led_green, 1);
        gpio_pin_set_dt(&led_blue, 1);
        break;

    case LED_COLOR_OFF:
    default:
        break;
    }
}

static enum led_color battery_color_from_percent(uint8_t percent) {
    if (percent >= 90) {
        return LED_COLOR_BLUE;
    } else if (percent >= 50) {
        return LED_COLOR_GREEN;
    } else if (percent >= 20) {
        return LED_COLOR_YELLOW;
    } else {
        return LED_COLOR_RED;
    }
}

static void led_work_handler(struct k_work *work) {
    switch (current_pattern) {
    case LED_PATTERN_BATTERY_BLINK:
        /*
         * 3 seconds total:
         * 250ms x 12 steps = about 3 seconds
         */
        if (step >= 12) {
            show_color(LED_COLOR_OFF);
            current_pattern = LED_PATTERN_NONE;
            return;
        }

        if ((step % 2) == 0) {
            show_color(current_color);
        } else {
            show_color(LED_COLOR_OFF);
        }

        step++;
        k_work_schedule(&led_work, K_MSEC(250));
        break;

    case LED_PATTERN_CHECK:
        /*
         * LED check:
         * white/all colors -> red -> green -> blue -> yellow -> cyan -> purple -> off
         * about 0.5s each
         */
        switch (step) {
        case 0:
            show_color(LED_COLOR_WHITE);
            break;
        case 1:
            show_color(LED_COLOR_RED);
            break;
        case 2:
            show_color(LED_COLOR_GREEN);
            break;
        case 3:
            show_color(LED_COLOR_BLUE);
            break;
        case 4:
            show_color(LED_COLOR_YELLOW);
            break;
        case 5:
            show_color(LED_COLOR_CYAN);
            break;
        case 6:
            show_color(LED_COLOR_PURPLE);
            break;
        default:
            show_color(LED_COLOR_OFF);
            current_pattern = LED_PATTERN_NONE;
            return;
        }

        step++;
        k_work_schedule(&led_work, K_MSEC(500));
        break;

    case LED_PATTERN_TRIPLE:
        /*
         * short triple blink:
         * ON/OFF x 3
         */
        if (step >= 6) {
            show_color(LED_COLOR_OFF);
            current_pattern = LED_PATTERN_NONE;
            return;
        }

        if ((step % 2) == 0) {
            show_color(current_color);
        } else {
            show_color(LED_COLOR_OFF);
        }

        step++;
        k_work_schedule(&led_work, K_MSEC(120));
        break;

    case LED_PATTERN_SINGLE:
        /*
         * single blink:
         * ON -> OFF
         */
        if (step == 0) {
            show_color(current_color);
            step++;
            k_work_schedule(&led_work, K_MSEC(350));
        } else {
            show_color(LED_COLOR_OFF);
            current_pattern = LED_PATTERN_NONE;
        }
        break;

    case LED_PATTERN_NONE:
    default:
        show_color(LED_COLOR_OFF);
        current_pattern = LED_PATTERN_NONE;
        return;
    }
}

static void start_pattern(enum led_pattern pattern, enum led_color color) {
    current_pattern = pattern;
    current_color = color;
    step = 0;

    k_work_cancel_delayable(&led_work);
    k_work_schedule(&led_work, K_NO_WAIT);
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case LED_MODE_BATTERY: {
        uint8_t percent = zmk_battery_state_of_charge();
        start_pattern(LED_PATTERN_BATTERY_BLINK, battery_color_from_percent(percent));
        break;
    }

    case LED_MODE_CHECK:
        start_pattern(LED_PATTERN_CHECK, LED_COLOR_WHITE);
        break;

    case LED_MODE_DRAW:
        start_pattern(LED_PATTERN_TRIPLE, LED_COLOR_WHITE);
        break;

    case LED_MODE_DEFAULT:
        start_pattern(LED_PATTERN_TRIPLE, LED_COLOR_GREEN);
        break;

    case LED_MODE_KANA:
        start_pattern(LED_PATTERN_SINGLE, LED_COLOR_GREEN);
        break;

    case LED_MODE_EISU:
        start_pattern(LED_PATTERN_SINGLE, LED_COLOR_BLUE);
        break;

    default:
        start_pattern(LED_PATTERN_SINGLE, LED_COLOR_RED);
        break;
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_battery_led_init(const struct device *dev) {
    if (!device_is_ready(led_red.port) ||
        !device_is_ready(led_green.port) ||
        !device_is_ready(led_blue.port)) {
        return -ENODEV;
    }

    gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

    show_color(LED_COLOR_OFF);

    k_work_init_delayable(&led_work, led_work_handler);

    return 0;
}

static const struct behavior_driver_api behavior_battery_led_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define BATTERY_LED_INST(n)                                                                    \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_battery_led_init, NULL, NULL, NULL, POST_KERNEL,        \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_battery_led_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BATTERY_LED_INST)
