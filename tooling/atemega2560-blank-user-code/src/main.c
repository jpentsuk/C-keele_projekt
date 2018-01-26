#include <avr/io.h>

void main (void)
{
    /* Set port B pin 7 for output for Arduino Mega yellow LED and turn it off */
    DDRB |= _BV(DDB7);
    PORTB &= ~_BV(PORTB7);
    /* Exit program */
    return;
}
