#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "hmi_msg.h"
#include "print_helper.h"
#include "../lib/hd44780_111/hd44780.h"
#include "../lib/helius_microrl/microrl.h"
#include "../lib/andygock_avr-uart/uart.h"
#include "../lib/matejx_avr_lib/mfrc522.h"
#include <time.h> // time.h can be used from avr-libc version 2.0.0
#include <stdlib.h> // stdlib is needed to use ltoa() - Long to ASCII
#include <avr/interrupt.h>
#include <util/delay.h>
#include "cli_microrl.h"

#define BLINK_DELAY_MS 100

#define UART_BAUD           9600
#define UART_STATUS_MASK    0x00FF

#define LED_RED         PORTA0 // Arduino Mega digital pin 22
#define LED_GREEN       PORTA2 // Arduino Mega digital pin 24

//since main.d does not have h-file, lets declare it  here
void door(void);


// Create microrl object and pointer on it
microrl_t rl;
microrl_t *prl = &rl;

static inline void init_leds(void)
{
    //LED board set up and off
    DDRA |= _BV(LED_RED) | _BV(LED_GREEN);
    PORTA &= ~(_BV(LED_RED) | _BV(LED_GREEN));
}


static inline void init_microrl(void)
{
    // Call init with ptr to microrl instance and print callback
    microrl_init (prl, uart0_puts);
    // Set callback for execute
    microrl_set_execute_callback (prl, cli_execute);
}


static inline void printversion(void)
{
    uart1_puts_p(PSTR(VER_FW));
    uart1_puts_p(PSTR(VER_LIBC));
}


static inline void init_sys_timer(void)
{
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1B |= _BV(WGM12); // Turn on CTC (Clear Timer on Compare)
    TCCR1B |= _BV(CS12); // fCPU/256
    OCR1A = 62549; // Note that it is actually two registers OCR5AH and OCR5AL
    TIMSK1 |= _BV(OCIE1A); // Output Compare A Match Interrupt Enable
}


static inline void simu_big_prog(void)
{
    /* Simulate big program with delay and toggle LEDs */
    PORTA ^= _BV(LED_RED);
    _delay_ms(BLINK_DELAY_MS);
}


static inline void heartbeat(void)
{
    static time_t prev_time;
    char ascii_buf[11] = {0x00};
    time_t now = time(NULL);

    if (prev_time != now) {
        //Print uptime to uart1
        ltoa(now, ascii_buf, 10);
        uart1_puts_p(PSTR("Uptime: "));
        uart1_puts(ascii_buf);
        uart1_puts_p(PSTR(" s.\r\n"));
        //Toggle LED
        PORTA ^= _BV(PORTA2);
        prev_time = now;
    }
}


static inline void init_con_uart1(void)
{
    uart1_init(UART_BAUD_SELECT(UART_BAUD, F_CPU));
    uart1_puts_p(PSTR("Console started\r\n"));
}


static inline void init_con_uart0(void)
{
    uart0_init(UART_BAUD_SELECT(UART_BAUD, F_CPU));
    uart0_puts_p(PSTR("Main console started\r\n"));
    uart0_puts_p(name);
    uart0_puts_p(PSTR("\r\n"));
    uart0_puts_p(PSTR("Use backspace to delete entry and enter to confirm\r\n"));
}


static inline void init_rfid_reader(void)
{
    //Init RFID-RC522
    MFRC522_init();
    PCD_Init();
}


void main(void)
{
    sei(); // Enable all interrupts. Set up all interrupts before sei()!!!
    init_leds();
    lcd_init();
    lcd_clrscr();
    lcd_puts_P(name);
    init_con_uart1();
    init_con_uart0();
    init_rfid_reader();
    init_microrl();
    init_sys_timer();
    printversion();

    while (1) {
        heartbeat();
        door();
        //for testing reasons
        //simu_big_prog();
        microrl_insert_char(prl, (uart0_getc() & UART_STATUS_MASK));
    }
}
ISR(TIMER1_COMPA_vect)
{
    system_tick();
}
