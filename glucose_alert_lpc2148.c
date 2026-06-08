#include <LPC214x.h>
#include <stdio.h>

#define LED (1 << 10)
#define BUZZER (1 << 11)

// LCD Pins
#define RS (1 << 16)
#define EN (1 << 17)
#define LCD_DATA_MASK 0xFF

void delay_ms(unsigned int ms)
{
    unsigned int i, j;

    for (i = 0; i < ms; i++)
        for (j = 0; j < 6000; j++);
}

// UART0 Init
void uart_init() {

    PINSEL0 |= 0x05;

    U0LCR = 0x83;
    U0DLL = 97;
    U0DLM = 0;
    U0LCR = 0x03;
}

void uart_send_string(char *str) {

    while (*str) {

        while (!(U0LSR & 0x20));

        U0THR = *str++;
    }
}

void uart_send_float(const char *label, float value) {

    char buf[50];

    sprintf(buf, "%s: %.1f mg/dL\n", label, value);

    uart_send_string(buf);
}

// LCD Functions
void lcd_cmd(unsigned char cmd) {

    IOCLR1 = RS;
    IOCLR1 = EN;

    IOCLR0 = LCD_DATA_MASK;

    IOSET0 = cmd;

    IOSET1 = EN;

    delay_ms(2);

    IOCLR1 = EN;
}

void lcd_data(unsigned char data) {

    IOSET1 = RS;
    IOCLR1 = EN;

    IOCLR0 = LCD_DATA_MASK;

    IOSET0 = data;

    IOSET1 = EN;

    delay_ms(2);

    IOCLR1 = EN;
}

void lcd_init() {

    IO1DIR |= RS | EN;
    IO0DIR |= LCD_DATA_MASK;

    lcd_cmd(0x38);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
}

void lcd_print(char *msg) {

    while (*msg)
        lcd_data(*msg++);
}

// ADC Setup
void adc_init() {

    PINSEL1 |= (1 << 24);

    AD1CR = 0x00200402;
}

unsigned int read_adc() {

    AD1CR |= (1 << 24);

    while (!(AD1GDR & (1 << 31)));

    return (AD1GDR >> 6) & 0x3FF;
}

int main() {

    unsigned int adc_val;
    float voltage, glucose;

    char lcd_buf[32];

    IO0DIR |= LED | BUZZER | LCD_DATA_MASK;
    IO1DIR |= RS | EN;

    uart_init();
    lcd_init();
    adc_init();

    lcd_print("Glucose Monitor");

    delay_ms(2000);

    lcd_cmd(0x01);

    uart_send_string("==== Glucose Monitor Started ====\n");

    while (1) {

        adc_val = read_adc();

        voltage = adc_val * 3.3 / 1023.0;

        glucose = voltage * 100.0;

        lcd_cmd(0x80);

        sprintf(lcd_buf, "Glucose: %.1fmg", glucose);

        lcd_print(lcd_buf);

        uart_send_float("Glucose", glucose);

        if (glucose < 70.0) {

            IOSET0 = LED | BUZZER;

            lcd_cmd(0xC0);
            lcd_print("LOW ALERT");

            uart_send_string("!!! LOW Glucose ALERT !!!\n");
        }

        else if (glucose > 140.0) {

            IOSET0 = LED | BUZZER;

            lcd_cmd(0xC0);
            lcd_print("HIGH ALERT");

            uart_send_string("!!! HIGH Glucose ALERT !!!\n");
        }

        else {

            IOCLR0 = LED | BUZZER;

            lcd_cmd(0xC0);
            lcd_print("Normal Level");

            uart_send_string("Status: Glucose Normal.\n");
        }

        delay_ms(3000);

        lcd_cmd(0x01);
    }
}
