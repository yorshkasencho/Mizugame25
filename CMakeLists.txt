# SPDX-License-Identifier: MIT

# Keep the custom battery/LED behavior from HoshinoUtsuro available for
# later Mizugame25 combos and LED indications.
target_sources(app PRIVATE src/behaviors/behavior_battery_led.c)

# profile_layer_restore.c is intentionally not compiled yet.
# Mizugame25's additional layers/BLE profile behavior will be defined later.
