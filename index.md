---
layout: home
title: Discrete Digital Clock Manual
---


---

## 📦 Overview

This clock is an enlarged version of a TTL logic IC, designed entirely from discrete components. It’s a cool way to see what’s inside a typical clock counter IC. The design is optimized for discrete components and extremely simplified to use a relatively small number of parts.

**TTL (Transistor-Transistor Logic)** is a type of digital logic built using bipolar junction transistors. It was the standard technology used in early digital circuits before modern integrated circuits became common.

What makes this clock especially interesting is that it only uses **basic components**—transistors, resistors, diodes, and LEDs. No microcontrollers or ICs are involved. In fact, this clock could have been built as early as **1970**, using only the technology available at the time.

All components used are **high-quality and RoHS-compliant**, ensuring safety and durability. The **expected lifetime is extremely long** for nearly all parts—except for the **electrolytic capacitors**, which may eventually require replacement after many years. Fortunately, these are easy to identify and replace if needed.

Another major advantage of this design is **visibility**: thanks to the widespread use of LEDs throughout the circuit, problems are easy to spot and diagnose. If something fails, you can typically see exactly where the issue is and fix it quickly.

The clock is meant for people who are interested in electronics—enthusiasts who don’t take ICs for granted. It’s also great for those learning electronics or anyone who simply likes how it looks.

This project started during the COVID lockdowns, when I had a lot of free time and decided to design my own transistor-based clock. That first version worked poorly and wasn’t very reliable. A couple of years later, I revisited the project and improved it significantly.

The clock’s precision depends on your local electric grid frequency, and is usually accurate to within ±20 seconds per year. Since it relies on mains frequency, it resets when the power goes out. This approach simplifies the design and reduces the component count. The fewer the components, the fewer things that can fail over time.

The clock is built on an X × X cm PCB that contains all necessary components. Surface-mount (SMD) components are used to reduce soldering time and manufacturing cost. While it would definitely look cooler with through-hole (THT) parts, that would double the size and take much longer to assemble.

It supports 220/110V, 50/60Hz input and can run directly from mains. If you’re not comfortable working with mains voltage, there’s also a 6V version available.

There are other similar digital clock kits online, often sold in THT form, but they aren’t optimized for discrete components. This design uses around 800 components, while others typically use between 1200 and 1800. Links to similar projects are provided at the end.

You can mount this clock on a wall using the four included 3M screws, which can be attached to a wooden plate or other surface and then hung.

---

## 🖼️ Preview

The clock board is divided into different chambers, each labeled by name and representing a specific functional section. There are five labeled chambers:

- Power Supply  
- Seconds  
- Hours / Minutes  
- Decoder  
- Multiplexer  

Below is a detailed description of each chamber.

### Power Supply

*Description coming soon...*

### Seconds

*Description coming soon...*

### Hours / Minutes

*Description coming soon...*

### Decoder

*Description coming soon...*

### Multiplexer

*Description coming soon...*

---

## 🛠️ Installation

Installation is extremely simple, but **please take caution**—especially with the mains-powered version.

### ⚠️ Mains-Powered Version

If you're using the mains version, you **must have a basic understanding of mains voltage**, as it can be **lethal**. You also need to **isolate the mains section** of the clock (or the entire unit) to ensure safety.

You’ll need your own power cable for connection. The cable should be connected to the **top right side** of the clock (the power supply section), where the connector is located.

A **30mA fuse** is included in the mains version for protection.

---

### 🔌 6V AC Version

For the low-voltage version, you need a **6V AC transformer** capable of supplying at least **1.9A**.

A **0.5A fuse** is pre-installed for this version as well.

---

## 🛠️ How to Use

When you power on the clock for the first time, it will be set to **00:00** without the colon (`:`) illuminated between the digits. All the counters will show zero and will not begin counting.

To activate the clock, you need to **press the Reset button**. This feature lets you know when a power outage has occurred, as the clock will remain at 00:00 until manually reset.

You can set the time using the following buttons:
- **M** (bottom left) to set **minutes**
- **H** (top left) to set **hours**

To set the seconds accurately, press the **Reset** button and release it **exactly at 0 seconds** (e.g., using another clock or stopwatch as reference).

---

## 📩 Contact

---
