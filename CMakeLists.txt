# SPDX-License-Identifier: MIT

target_sources(app PRIVATE src/behaviors/behavior_battery_led.c)

target_sources_ifdef(
    CONFIG_ZMK_BLE
    app
    PRIVATE
    src/profile_layer_restore.c
)
