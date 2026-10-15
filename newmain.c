/*
 * File:   newmain.c
 * Author: LENOVO
 *
 * Created on 8 October, 2026, 2:54 PM
 *
 * Updated: Buzzer (RB0), Alarm LED (RB1), sensor fault detection,
 *          ALARM LCD text, fixed >= threshold, all LCD strings = 16 chars.
 */

#pragma config FOSC = HS        // HS oscillator (16 MHz)
#pragma config WDTE = OFF       // Watchdog Timer disabled
#pragma config PWRTE = OFF      // Power-up Timer disabled
#pragma config BOREN = OFF      // Brown-out Reset disabled
#pragma config LVP = OFF        // Low-Voltage Programming disabled
#pragma config CPD = OFF        // Data EEPROM Code Protection off
#pragma config WRT = OFF        // Flash Write Enable off
#pragma config CP = OFF         // Flash Code Protection off

#include <xc.h>
#include <stdio.h>

#define _XTAL_FREQ 16000000UL   // 16 MHz HS oscillator

/* ── LCD pins (PORTD) ───────────────────────────────────────────────────── */
#define RS  RD0
#define RW  RD2
#define E   RD1

/* ── Alarm pins (PORTB) ─────────────────────────────────────────────────── */
#define BUZZER    RB0            // Buzzer output on RB0
#define ALARM_LED RB1            // Alarm LED output on RB1

/* ── Temperature thresholds ─────────────────────────────────────────────── */
#define T_MIN           20.0f   // Below this  -> fan OFF (PWM = 0)
#define T_MAX           40.0f   // At/above this -> ALARM, fan 100%

/* ── Sensor fault: consecutive bad ADC readings before fault declared ────── */
#define FAULT_THRESHOLD 3

/* ═══════════════════════════════════════════════════════════════════════════
   LCD FUNCTIONS
   All LCD string literals are exactly 16 characters to fill the display
   and prevent stale / garbage characters on the 16x2 HD44780 LCD.
   ═══════════════════════════════════════════════════════════════════════════ */

void lcd_nibble(unsigned char n)
{
    PORTD = (PORTD & 0x0F) | (n & 0xF0);   // D4-D7 on upper nibble of PORTD
    E = 1; __delay_us(50);
    E = 0; __delay_us(50);
}

void command(unsigned char cmd)
{
    RS = 0; RW = 0;
    lcd_nibble(cmd);            // High nibble first
    lcd_nibble(cmd << 4);       // Low nibble
    __delay_ms(5);
}

void data(unsigned char ch)
{
    RS = 1; RW = 0;
    lcd_nibble(ch);
    lcd_nibble(ch << 4);
    __delay_ms(2);
}

void str_tx(char str[])
{
    for (int i = 0; str[i] != '\0'; i++)
        data(str[i]);
}

void lcd_init(void)
{
    __delay_ms(50);             // HD44780 power-on delay

    RS = 0; RW = 0;
    lcd_nibble(0x30); __delay_ms(5);  // Wake pulse 1
    lcd_nibble(0x30); __delay_ms(1);  // Wake pulse 2
    lcd_nibble(0x30); __delay_ms(1);  // Wake pulse 3
    lcd_nibble(0x20); __delay_ms(2);  // Switch to 4-bit mode

    command(0x28);              // 4-bit, 2 lines, 5x8 font
    command(0x0C);              // Display ON, cursor OFF
    command(0x06);              // Entry: auto-increment, no shift
    command(0x01);              // Clear display
    __delay_ms(5);
}

/* ═══════════════════════════════════════════════════════════════════════════
   UART FUNCTIONS
   ═══════════════════════════════════════════════════════════════════════════ */

void uart_char_tx(char ch)
{
    while (TRMT == 0);          // Wait until transmit shift register empty
    TXREG = ch;
}

void uart_str_tx(char str[])
{
    for (int i = 0; str[i] != '\0'; i++)
        uart_char_tx(str[i]);
}

/* ═══════════════════════════════════════════════════════════════════════════
   ADC FUNCTION
   ═══════════════════════════════════════════════════════════════════════════ */

int read_adc(void)
{
    GO = 1;
    while (GO == 1);            // Wait for conversion complete
    return (ADRESH << 8) | ADRESL;
}

/* ═══════════════════════════════════════════════════════════════════════════
   TIMER2 DELAY  (100 x TMR2 overflows)
   ═══════════════════════════════════════════════════════════════════════════ */

void delay(void)
{
    for (int i = 0; i < 100; i++)
    {
        while (TMR2IF == 0);
        TMR2   = 0;
        TMR2IF = 0;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   PERIPHERAL INITIALISATION
   ═══════════════════════════════════════════════════════════════════════════ */

void adc_init(void)
{
    TRISA  = 0xFF;              // All PORTA pins as inputs (analog)
    PORTA  = 0x00;
    ADCON0 = 0x41;              // ADC ON, channel AN0, Fosc/8
    ADCON1 = 0x80;              // Right-justified result, all analog
}

void uart_init(void)
{
    TXSTA = 0x24;               // TXEN=1, BRGH=1, async
    RCSTA = 0x90;               // SPEN=1, CREN=1
    SPBRG = 103;                // 9600 baud @ 16 MHz, BRGH=1
}

void pwm_init(void)
{
    T2CON   = 0x05;             // Timer2 ON, prescaler = 4
    PR2     = 250;              // PWM period register
    CCP1CON = 0x0C;             // CCP1 in PWM mode
    CCPR1L  = 0;                // Initial duty = 0%
}

/* ═══════════════════════════════════════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════════════════════════════════════ */

void main(void)
{
    /* Port directions */
    TRISC = 0x80;               // RC7=RX (input), RC6=TX (output)
    PORTC = 0x00;
    TRISB = 0x00;               // PORTB all output  (RB0=Buzzer, RB1=LED)
    PORTB = 0x00;               // All off
    TRISD = 0x00;               // PORTD all output  (LCD)
    PORTD = 0x00;

    __delay_ms(100);

    lcd_init();
    uart_init();
    adc_init();
    pwm_init();

    /* ── Startup splash (each string = exactly 16 chars) ─────────────────── */
    command(0x80);  str_tx("  SYSTEM START  ");   // 16 chars ✓
    command(0xC0);  str_tx("Initializing... ");   // 16 chars ✓
    __delay_ms(2000);
    command(0x01);                                 // Clear display
    __delay_ms(5);

    char str[24];               // Scratch buffer (24 bytes, safe for all formats)
    int  fault_count = 0;       // Consecutive out-of-range ADC readings counter

    while (1)
    {
        int res = read_adc();   // 10-bit ADC result (0–1023)

        /* ── Sensor Fault Detection ──────────────────────────────────────── *
         * LM35 open-circuit  → ADC ≈ 0                                       *
         * LM35 short to VDD  → ADC ≈ 1023                                    *
         * Three or more consecutive bad readings = sensor fault.              *
         * ─────────────────────────────────────────────────────────────────── */
        if (res <= 0 || res >= 1023)
            fault_count++;
        else
            fault_count = 0;    // Valid reading — clear counter

        if (fault_count >= FAULT_THRESHOLD)
        {
            CCPR1L    = 250;    // Force fan to full speed (safety)
            BUZZER    = 1;
            ALARM_LED = 1;

            command(0x80);  str_tx("SENSOR FAULT!   ");  // 16 chars ✓
            command(0xC0);  str_tx("Check LM35 conn.");  // 16 chars ✓

            uart_str_tx("\rSENSOR FAULT DETECTED\r");
            delay();
            continue;           // Skip rest of loop for this cycle
        }

        /* ── Temperature conversion  (LM35: 10 mV/°C, 5 V ref, 10-bit) ─── */
        float temp = (res * 5.0f / 1023.0f) * 100.0f;

        /* ── PWM duty calculation (linear: T_MIN→0%  …  T_MAX→100%) ─────── */
        int pwm_val;
        if      (temp <= T_MIN) pwm_val = 0;
        else if (temp >= T_MAX) pwm_val = 250;
        else                    pwm_val = (int)((temp - T_MIN) * 250.0f / (T_MAX - T_MIN));

        /* ── Critical Threshold Alarm (>= T_MAX) ────────────────────────── */
        if (temp >= T_MAX)
        {
            BUZZER    = 1;      // Buzzer ON  (RB0)
            ALARM_LED = 1;      // LED ON     (RB1)
            CCPR1L    = 250;    // Fan forced to full duty

            /* LCD Row 1 — alarm banner, exactly 16 chars:
               " ** ALARM! **   "
                1234567890123456                                               */
            command(0x80);
            str_tx(" ** ALARM! **   ");

            /* LCD Row 2 — temperature at alarm, exactly 16 chars:
               sprintf with %3d gives a 3-char field, e.g. " 42" or "100"
               "T: 42C  Fan:100%"  →  T:(2) + _42(3) + C__Fan:(7) + 100%(4) = 16 ✓
               "T:100C  Fan:100%"  →  T:(2) + 100(3) + C__Fan:(7) + 100%(4) = 16 ✓  */
            command(0xC0);
            sprintf(str, "T:%3dC  Fan:100%%", (int)temp);
            str_tx(str);
        }
        else
        {
            /* ── Normal proportional operation ──────────────────────────── */
            BUZZER    = 0;      // Buzzer OFF
            ALARM_LED = 0;      // LED OFF
            CCPR1L    = pwm_val;

            int fan_pct = (pwm_val * 100) / 250;   // 0–100 %

            /* LCD Row 1 — temp and fan %, exactly 16 chars:
               sprintf %3d gives 3-char field for both temp and fan_pct.
               "T: 35C  Fan: 75%"  →  T:(2) + _35(3) + C__Fan:(7) + _75%(4) = 16 ✓
               "T:  0C  Fan:  0%"  →  T:(2) + __0(3) + C__Fan:(7) + __0%(4) = 16 ✓  */
            command(0x80);
            sprintf(str, "T:%3dC  Fan:%3d%%", (int)temp, fan_pct);
            str_tx(str);

            /* LCD Row 2 — raw ADC and PWM for diagnostics, exactly 16 chars:
               "ADC: 512 PWM:187"  →  ADC:(4) + _512(4) + _PWM:(5) + 187(3) = 16 ✓
               "ADC:   0 PWM:  0"  →  all fields right-aligned, always 16    ✓  */
            command(0xC0);
            sprintf(str, "ADC:%4d PWM:%3d", res, pwm_val);
            str_tx(str);
        }

        /* ── UART telemetry ─────────────────────────────────────────────── */
        sprintf(str, "\radc=%d : pwm=%d \r", res, pwm_val);
        uart_str_tx(str);

        delay();
    }

    return;
}