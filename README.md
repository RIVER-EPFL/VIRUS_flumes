# Automatic flume pump controller

This Arduino-based controller manages two flumes (A and B), each with two pumps, using a 4-channel relay and an OLED display. The system supports automatic timed cycles and manual control.

## Features

* **Automatic mode** (`btnAuto` / D5):
  Press to start a synchronized on/off cycle for both flume A and flume B. Each cycle runs for a configurable interval (default: 1 hour), followed by a short all-off pause (default: 2 seconds). The OLED shows "Next switch in ..." with hours, minutes, and seconds precision.

* **Manual modes**:

  * **Flume A only** (`btnA` / D6): turn off all pumps (if in automatic mode) and then energize pump 1 only. Leave flume B status unchanged.
  * **Flume B only** (`btnB` / D7): turn off all pumps (if in automatic mode) and then energize pump 3 only. Leave flume A status unchanged.

* **All off**:
  Press buttons **Flume A and Flume B** together to immediately turn off all pumps and stop any cycling.

* **OLED display**:

  1. Header showing countdown or mode
  2. Flume A label and pumps P1/P2 status
  3. Flume B label and pumps P3/P4 status

## Usage

1. Upload the sketch to Arduino (disconnect the system from the main power, only use the usb cable).
3. Power on the system. All pumps remain off.
4. Press the automatic button (D5) to begin the cycle.
5. Use button Flume A (D6), button Flume B (D7) to start the pumps manually, or Flume A+ Flume B to stop all pumps.
6. Watch the OLED display for status and countdown.

## Pin assignments

| function         | Arduino pin | relay channel |
| ---------------- | ----------- | ------------- |
| flume A pump 1   | D9          | IN2           |
| flume A pump 2   | D8          | IN1           |
| flume B pump 3   | D11         | IN4           |
| flume B pump 4   | D10         | IN3           |
| button automatic | D5          |               |
| button A         | D6          |               |
| button B         | D7          |               |
| OLED SCL         | A5          | I²C           |
| OLED SDA         | A4          | I²C           |
| OLED VCC         | 5V          |               |
| common ground    | GND         |               |

> Note: the relay module is active LOW: writing `LOW` energizes (turns on) the pump, and `HIGH` de-energizes (turns off).

## Configuration

* **Cycle interval**: change the `pumpInterval` variable in the sketch (milliseconds). For a 3-hour cycle, use:

  ```cpp
  unsigned long pumpInterval = 3UL * 3600UL * 1000UL; // 3 hours
  ```

* **Pause duration**: change the `pauseDur` variable (milliseconds) for the all-off pause (to avoid overdrawing the 230V to 12V converter with 2 pumps running simultaneously).



