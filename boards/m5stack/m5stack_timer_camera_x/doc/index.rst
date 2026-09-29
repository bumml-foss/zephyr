.. zephyr:board:: m5stack_timer_camera_x

Overview
********

M5Stack Timer Camera X is a battery powered camera module based on the ESP32. Its real-time
clock can switch the board on at a set time, so that it takes a picture and turns itself off
again. See the `M5Stack Timer Camera X Documentation`_.

Hardware
********

- ESP32-D0WDQ6-V3 chip (dual-core Xtensa LX6 processor @240MHz, Wi-Fi, Bluetooth)
- PSRAM 8MB (Quad)
- Flash 4MB
- Camera 3 MP OV3660, DFOV 66.5°
- RTC BM8563
- Battery 140mAh 3.7 V with charger, voltage divider and power switch
- Blue status LED
- Power button
- Grove (HY2.0-4P) port
- USB-C, USB to serial bridge

.. note::

   The camera is not supported. Zephyr has no capture driver for the ESP32 and no driver for
   the OV3660.

.. list-table::
   :header-rows: 1

   * - Function
     - Pins
     - Zephyr node
   * - Console
     - GPIO1 (TX), GPIO3 (RX)
     - ``uart0``
   * - Status LED
     - GPIO2
     - ``blue_led`` (``led0``)
   * - RTC BM8563 (0x51)
     - GPIO12 (SDA), GPIO14 (SCL)
     - ``i2c0``, ``bm8563`` (``rtc``)
   * - Grove port
     - GPIO4 (SDA), GPIO13 (SCL)
     - ``i2c1`` (``grove_i2c`` / ``grove_header``)
   * - Battery power hold
     - GPIO33
     - ``bat_hold``
   * - Battery voltage
     - GPIO38
     - ``vbat``, ``adc0`` channel 2
   * - Camera data D0 to D7
     - GPIO32, GPIO35, GPIO34, GPIO5, GPIO39, GPIO18, GPIO36, GPIO19
     - not supported
   * - Camera XCLK, PCLK, VSYNC, HREF
     - GPIO27, GPIO21, GPIO22, GPIO26
     - not supported
   * - Camera SCCB (0x3C), reset
     - GPIO25 (SDA), GPIO23 (SCL), GPIO15
     - not supported

The RTC bus and the Grove port have no pull-up resistors on the board. They use the pull-ups of
the SoC and are set to standard speed.

Power
=====

On USB power the board is always on. On battery power it is switched on by the power button or
by the interrupt output of the RTC, and stays on only as long as GPIO33 is high. The board
definition drives GPIO33 high at boot through the ``bat_hold`` regulator. Disabling that
regulator while running from the battery turns the board off, which is how an application
ends a wake-up.

The interrupt output of the RTC is connected to the power switch only, not to a GPIO. The power
button reaches GPIO37 through a diode and without a pull-up, and is not defined as an input.

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

The USB to serial bridge of the board passes 115200 and 1500000 baud only. The board flashes
at 1500000 baud by default.

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

The JTAG pins of the SoC (GPIO12 to GPIO15) are used by the RTC, the Grove port and the camera.
Only GPIO13 is accessible, on the Grove port, so the board cannot be debugged over JTAG.

References
**********

.. target-notes::

.. _`M5Stack Timer Camera X Documentation`: https://docs.m5stack.com/en/unit/timercam_x
