# ESP32 WiFi Clock Setter

This ESP32 add-on sets the clock automatically when the clock boots.

On startup, the ESP32 connects to WiFi, gets the current time from NTP, turns WiFi off, and then uses the clock's J5 header to start and set the external clock.

## Arduino IDE settings

Recommended:

```text
Tools -> CPU Frequency -> 80MHz
Tools -> Flash Frequency -> 40MHz
```

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
| Pin 1 | Reset | Not used |
| Pin 2 | Boot reset check / start | ESP32 GPIO 32 |
| Pin 3 | 5V power | 5V supply only if safe |
| Pin 4 | Carry bit for day | Not used |
| Pin 5 | Set minutes | NPN transistor collector |
| Pin 6 | Set hours | NPN transistor collector |
| Pin 7 | Ground | ESP32 GND |

## Important power note

Do **not** power the ESP32 from USB and also connect another 5V supply to the ESP32 5V pin at the same time.


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

GPIO 32 is normally an analog input. At exact second `00`, ESP32 changes GPIO 32 to output HIGH to start the clock, then returns it to input mode.

## Minute and hour setting transistors

The hour and minute set pins are pulled up to **5V** on the clock board. The clock increments when these pins are pulled **LOW**, so they must be driven through an NPN transistor or another suitable level-shifting circuit. Do **not** connect these pins directly to the ESP32.

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

Enter WiFi SSID, password, and timezone. ESP32 saves them and restarts. You can check serial (bound rate 115200) for debugging.

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

If WiFi setup opens every boot, check Serial Monitor (bound rate 115200) and GPIO 33 reset button wiring.

If the clock keeps resetting, check J5 Pin 2 voltage and `CLOCK_RESET_THRESHOLD_MV`.

If hours/minutes do not increment, check NPN transistor wiring and common ground.

