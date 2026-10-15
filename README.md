# PIC16F877A Industrial Temperature Monitoring & Safety Controller

> **PIC16F877A Industrial Project Handbook** — Level 1

A complete embedded C firmware project for the **PIC16F877A** microcontroller that reads ambient temperature via an **LM35** analog sensor, displays it on a **16x2 LCD**, drives a **PWM-controlled cooling fan** proportionally, and triggers a **buzzer + LED alarm** when a critical temperature threshold is breached.

---

## Features

| Feature | Detail |
|---|---|
| 🌡️ Temperature Sensing | LM35 on ADC channel AN0 (10 mV/°C) |
| 📺 LCD Display | 16x2 HD44780, 4-bit mode via PORTD |
| 🌀 PWM Fan Control | CCP1/Timer2, proportional T_MIN→T_MAX |
| 🔔 Alarm System | Buzzer (RB0) + LED (RB1) at critical threshold |
| ⚠️ Sensor Fault Detection | 3 consecutive bad ADC readings → LCD fault message |
| 📡 UART Telemetry | Sends ADC + PWM data at 9600 baud |

---

## LCD Display Behaviour

| State | Row 1 | Row 2 |
|---|---|---|
| Startup | `  SYSTEM START  ` | `Initializing... ` |
| Normal | `T: 35C  Fan: 75%` | `ADC: 716 PWM:187` |
| Alarm (≥ 40°C) | ` ** ALARM! **   ` | `T: 42C  Fan:100%` |
| Sensor Fault | `SENSOR FAULT!   ` | `Check LM35 conn.` |

---

## Hardware Requirements

- PIC16F877A microcontroller
- LM35 analog temperature sensor → **AN0 (RA0)**
- 16x2 LCD (HD44780) → **PORTD** (4-bit mode)
- DC cooling fan via NPN transistor → **RC2 / CCP1**
- Buzzer → **RB0**
- Alarm LED → **RB1**
- 16 MHz HS crystal oscillator
- 5V regulated supply

## Pin Map

| Pin | Function |
|---|---|
| RA0 (AN0) | LM35 temperature sensor input |
| RC2 (CCP1) | PWM output to fan transistor |
| RC6 (TX) | UART transmit |
| RC7 (RX) | UART receive |
| RD0 | LCD RS |
| RD1 | LCD Enable |
| RD2 | LCD RW |
| RD4–RD7 | LCD Data (4-bit) |
| RB0 | Buzzer |
| RB1 | Alarm LED |

---

## Software Configuration

| Parameter | Value |
|---|---|
| Oscillator | HS, 16 MHz |
| `_XTAL_FREQ` | 16,000,000 UL |
| UART Baud | 9600 (SPBRG=103, BRGH=1) |
| PWM Period (PR2) | 250 |
| Timer2 Prescaler | 4 |
| T_MIN | 20.0 °C (fan starts) |
| T_MAX | 40.0 °C (alarm + full fan) |

---

## Toolchain

- **IDE:** MPLAB X
- **Compiler:** XC8 v3.10
- **Simulation:** Proteus Design Suite
- **Language Standard:** C99

---

## Project Structure

```
PIC16F877A-Industrial-Project/
├── newmain.c                          # Main firmware source
├── Makefile                           # MPLAB X build makefile
├── nbproject/                         # MPLAB X project config
├── Proteus_simulation.DSN             # Proteus schematic file
└── PIC16F877A_Industrial_Project.pdf  # Project handbook
```

---

## How to Build

1. Open project in **MPLAB X IDE**
2. Select **XC8** toolchain
3. Click **Build** (or `F11`)
4. Flash the `.hex` from `dist/` to your PIC16F877A

---

*Based on the PIC16F877A Industrial Project Handbook — Educational simulation. Not a certified industrial safety device.*
