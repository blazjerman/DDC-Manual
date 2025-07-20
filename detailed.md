---
layout: page
title: Detailed Overview
permalink: /detailed/
---

The clock includes many LED indicators to show how the system works (details about LED functions are provided below). The indicator LEDs operate at low current to increase their lifespan—most run at 2 mA, except for the display LEDs, which are driven at around 15 mA due to multiplexing and their lower brightness.

All other components on the board are high quality. All ceramic capacitors are C0G type from Murata Electronics, and the transistors and diodes are sourced from ON Semiconductor.

The clock board is divided into different chambers, each labeled by name and representing a specific functional section. There are five labeled chambers:

- Power Supply  
- Seconds  
- Hours / Minutes  
- Decoder  
- Multiplexer  

You can find the full schematic of the clock here:  
[📄 View Clock Schematic (PDF)](/files/scheme.pdf)

Below is a detailed description of each chamber.


### Power Supply

![Clock psu](images/power_suply.png)

The power supply takes mains AC and converts it to a lower 6V using a transformer. A fuse is included on the primary side for safety. If a socket transformer is used, you should install a 1A fuse and short transformer pins 1 to 7 and 5 to 9. This allows you to connect the low-voltage wires from the socket to the top-right connector.

The transformer output is then rectified and regulated down to 6V using a low-voltage drop-down regulator. This regulator includes a comparator that compares a reference voltage from a 3V Zener diode to control the output. A BD140 transistor is used for voltage regulation. This transistor is rated to dissipate 1.25W without a heat sink; while a heat sink can be added, it is not required in this case.

On the right side, there is a Schmitt trigger comparator that receives the mains frequency signal and filters out unwanted noise that may come from other devices on the same power network. There is also an LED indicator labeled R296 (marked "CS" on the board), which blinks at the mains frequency.


### Seconds

![Clock psu](images/seconds.png)

This part of the clock converts the mains frequency into seconds, and then into minutes. It is made up of counters and reset counters.

Each counter is a toggle flip-flop with two output states: a normal STATE and its negation. It has a RESET input, which sets it to its initial state when high, and a PULSE input, which toggles its state on the falling edge of the pulse.

The reset counter is a simple pulse extender that outputs high when the input is low. It also includes a reset input that forces the output high, allowing the clock to be reset.

The entire section consists of 12 counters and two reset counters. The first six counters, located at the bottom, convert the mains frequency into seconds. These counters are connected in series: the first receives a noise-free mains frequency signal from the power supply at its pulse input; each of the remaining five counters is connected via the negated output of the previous counter to its own pulse input. Each counter also has an LED that lights up when its STATE output is high, allowing us to visually read the binary value. This design makes the clock digital, with each counter representing one bit of a binary number—in this case, a 6-bit number, with the least significant bit on the right (FS0) and the most significant on the left (FS5). The binary value can be easily converted to decimal by reading the LEDs.

A 6-bit counter can represent 64 values (0 to 63), but we need it to count only up to the mains frequency (e.g., 50Hz) in order to generate a 1-second pulse at the final counter. To achieve this, a reset counter resets the six counters once they reach a specific value. For example, if we want the counters to reset at 50 (binary 110010), we must detect that exact value and then pull all the reset inputs of the six counters high. The simplest way to detect this number is by using an OR logic gate on all the outputs that should be low at the reset point. This gate outputs high whenever the value is not the target, and low only when the exact reset value is reached. The resulting pulse is then inverted and extended by the reset counter to ensure a safe and reliable reset.

This reset method has a minor issue: due to transistor delays, the reset value might briefly appear for a few nanoseconds. This is not visible to the naked eye but can be detected using a high-speed camera. The main advantage of this design is that the same reset signal can be used for all counters that need to reset at a specific value.

The second group of six counters converts seconds into minutes. They function similarly to the first group, but are fixed to reset at 60 (binary 111100). Technically, with a 50Hz mains frequency, all 12 counters could be linked and reset together at a count of 300. However, this would make it difficult to visually convert the binary seconds into decimal using the LEDs from S0 to S5.


### Hours / Minutes
*Description coming soon...*

### Decoder
*Description coming soon...*

### Multiplexer
*Description coming soon...*

