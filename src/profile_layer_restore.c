/*
 * HoshinoUtsuro BLE profile -> layer restore
 */

#include <zephyr/settings/settings.h>

#include <zmk/ble.h>
#include <zmk/keymap.h>

#define HOSHINO_LAYER_DEFAULT 0
#define HOSHINO_LAYER_APPLE   1
#define HOSHINO_LAYER_DRAW    5

static int hoshino_restore_profile_layers(void) {
    const int profile = zmk_ble_active_profile_index();

    switch (profile) {
    case 1:
        /* iPad: Apple base + Draw */
        zmk_keymap_layer_to(HOSHINO_LAYER_APPLE);
        zmk_keymap_layer_activate(HOSHINO_LAYER_DRAW);
        break;

    case 2:
    case 3:
        /* iPhone / MacBook */
        zmk_keymap_layer_to(HOSHINO_LAYER_APPLE);
        break;

    case 0:
    case 4:
    default:
        /* Windows / default */
        zmk_keymap_layer_to(HOSHINO_LAYER_DEFAULT);
        break;
    }

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(
    hoshino_profile_layer_restore,
    "hoshino_profile_layer_restore",
    NULL,
    NULL,
    hoshino_restore_profile_layers,
    NULL
);
