#ifndef _HMI_MSG_H_
#define _HMI_MSG_H_

#define VER_FW "Version: " FW_VERSION " built on: " __DATE__ " " __TIME__ "\r\n"
#define VER_LIBC "avr-libc version: " __AVR_LIBC_VERSION_STRING__ " avr-gcc version: " __VERSION__ "\r\n"

extern const char name[];
extern const char ent_num_p[];
extern const char ent_num[];
extern const char console_error_msg[];
extern const char lcd_error_msg[];
extern const char ent_d[];
extern const char not_num [];
extern PGM_P const numbernames[];

#endif
