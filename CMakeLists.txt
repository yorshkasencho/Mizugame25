# SPDX-License-Identifier: MIT

target_sources(app PRIVATE src/behaviors/behavior_battery_led.c)

# HoshinoUtsuro用のBLE profile -> layer restoreは
# Mizugame25ではまだ使用しない。
# src/profile_layer_restore.c はコンパイルしない。
