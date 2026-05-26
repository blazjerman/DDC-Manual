# ESP32 WiFi Clock Setter

This ESP32 gets time from WiFi/NTP, turns WiFi off, then uses the internal clock to start and set the external clock.

## Arduino IDE settings

Recommended:

```text
Tools -> CPU Frequency -> 80MHz
Tools -> Flash Frequency -> 40MHz
```

If upload or boot is unstable, return flash frequency to the board default.

## ESP32 pins

| ESP32 pin | Use |
|---|---|
| GPIO 32 | Clock reset sense + clock start output |
| GPIO 27 | Set minutes output |
| GPIO 26 | Set hours output |
| GPIO 33 | WiFi reset button |
| GND | Common ground |

## Clock header J5

| J5 pin | Function | Connection |
|---|---|---|
| Pin 1 | Disable LEDs | Not used |
| Pin 2 | Reset check / start | ESP32 GPIO 32 |
| Pin 3 | 5V power | 5V supply only if safe |
| Pin 4 | Carry bit for day | Not used |
| Pin 5 | Set minutes | NPN transistor collector |
| Pin 6 | Set hours | NPN transistor collector |
| Pin 7 | Ground | ESP32 GND |

## Important power note

Do **not** power the ESP32 from USB and also connect another 5V supply to the ESP32 5V pin at the same time.

Use one of these:

```text
Option A: Clock has its own 5V supply, ESP32 uses USB, grounds connected.
Option B: Clock and ESP32 share one good 5V supply, no USB power connected.
```

## J5 Pin 2 reset/start wire

Connect:

```text
J5 Pin 2 -> ESP32 GPIO 32
```

Behavior:

```text
Below 0.7V = clock reset/off
Above 0.7V = clock running
```

The code treats below this as reset/off:

```cpp
const int CLOCK_RESET_THRESHOLD_MV = 700;
```

GPIO 32 is normally an analog input. At exact second `00`, ESP32 changes GPIO 32 to output HIGH to start the clock, then returns it to input mode.

Warning: ESP32 pins are **not 5V tolerant**. Check J5 Pin 2 with a multimeter. If it can go above 3.3V, use a voltage divider/protection.

## Minute and hour setting transistors

J5 Pin 5 and J5 Pin 6 are 5V clock inputs. Do **not** connect them directly to ESP32.

Use one NPN transistor per pin.

### Minutes

```text
ESP32 GPIO 27 -> 100k resistor -> NPN base
NPN emitter  -> GND
NPN collector -> J5 Pin 5
```

### Hours

```text
ESP32 GPIO 26 -> 100k resistor -> NPN base
NPN emitter  -> GND
NPN collector -> J5 Pin 6
```

Suitable NPN transistors:

```text
2N3904, BC547, 2N2222
```

## WiFi reset button

Connect:

```text
ESP32 GPIO 33 -> button -> GND
```

Hold the button while powering/resetting the ESP32. Keep holding for 3 seconds to clear saved WiFi and timezone.

## First WiFi setup

When no WiFi is saved, ESP32 creates:

```text
ESP32-WiFi-Setup
```

Connect to it and open:

```text
http://192.168.4.1
```

Enter WiFi SSID, password, and timezone. ESP32 saves them and restarts.

## Startup sequence

1. ESP32 connects to saved WiFi.
2. It retries until WiFi works.
3. It retries NTP until time is known.
4. It turns WiFi off.
5. It checks GPIO 32 voltage.
6. If clock is reset/off, it waits for second `00`.
7. At exact `00`, GPIO 32 goes HIGH.
8. It pulses hours and minutes.
9. GPIO 32 returns to input mode.

## Troubleshooting

If WiFi setup opens every boot, check Serial Monitor and GPIO 33 reset button wiring.

If the clock keeps resetting, check J5 Pin 2 voltage and `CLOCK_RESET_THRESHOLD_MV`.

If hours/minutes do not increment, check NPN transistor wiring and common ground.

