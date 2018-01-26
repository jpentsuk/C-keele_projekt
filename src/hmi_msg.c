
#include <avr/pgmspace.h>
#include "hmi_msg.h"

const char name[] PROGMEM = "Jan Pentshuk";
const char not_num[] PROGMEM = "Its not a number";
const char ent_num_p[] PROGMEM = "Enter number, please!";
const char console_error_msg[] PROGMEM = "Please enter number between 0 and 9";
const char lcd_error_msg[] PROGMEM = "Error";
const char ent_d[] PROGMEM = "You entered number  %S\r\n";
const char ent_num [] = "Enter number!";

const char string_0[] PROGMEM = "Zero";
const char string_1[] PROGMEM = "One";
const char string_2[] PROGMEM = "Two";
const char string_3[] PROGMEM = "Three";
const char string_4[] PROGMEM = "Four";
const char string_5[] PROGMEM = "Five";
const char string_6[] PROGMEM = "Six";
const char string_7[] PROGMEM = "Seven";
const char string_8[] PROGMEM = "Eight";
const char string_9[] PROGMEM = "Nine";

PGM_P const numbernames[] PROGMEM = {
    string_0,
    string_1,
    string_2,
    string_3,
    string_4,
    string_5,
    string_6,
    string_7,
    string_8,
    string_9
};
