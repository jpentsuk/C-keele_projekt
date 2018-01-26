//TODO There are most likely unnecessary includes. Clean up during lab6
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include "../lib/hd44780_111/hd44780.h"
#include "../lib/andygock_avr-uart/uart.h"
#include "../lib/helius_microrl/microrl.h"
#include "hmi_msg.h"
#include "cli_microrl.h"
#include <time.h>
#include "print_helper.h"
#include "../lib/andy_brown_memdebug/memdebug.h"
#include "../lib/matejx_avr_lib/mfrc522.h"

#define NUM_ELEMS(x)        (sizeof(x) / sizeof((x)[0]))
#define LED_RED PORTA0
//define times for display_message length and door-open length
#define DISPLAY_MSG_IN_SEC 3
#define DOOR_OPEN_IN_SEC 2
// define message when access is denied
#define DOOR_ACCESS_DENIED "Access denied!"

void cli_print_help(const char *const *argv);
void cli_print_name(const char *const *argv);
void cli_print_ver(const char *const *argv);
void cli_print_ascii_tbls(const char *const *argv);
void cli_handle_number(const char *const *argv);
void cli_print_cmd_error(void);
void cli_print_cmd_arg_error(void);
void cli_example(const char *const *argv);
void cli_mem(const char *const *argv);
void cli_rfid_read(const char *const *argv);
void cli_rfid_add(const char *const *argv);
void cli_rfid_print(const char *const *argv);
void cli_remove_card(const char *const *argv);

typedef enum {
    door_opening,
    door_open,
    door_closing,
    door_closed
} door_state_t;

typedef enum {
    display_name,
    display_access_denied,
    display_clear,
    display_no_update,
} display_state_t;

display_state_t previous_display_state;

typedef struct cli_cmd {
    PGM_P cmd;
    PGM_P help;
    void (*func_p)();
    const uint8_t func_argc;
} cli_cmd_t;

char * current_display_name;
char * previous_display_name;
//at first door has to be closed
door_state_t door_state = door_closed;
//lcd is not updating
display_state_t display_state = display_no_update;
//declaring time_t type variables
time_t time_y2k_cpy;
time_t door_open_time;
time_t msg_display_time;
time_t card_read_time;
time_t now_time;

const char help_cmd[] PROGMEM = "help";
const char help_help[] PROGMEM = "Get help";
const char example_cmd[] PROGMEM = "example";
const char example_help[] PROGMEM =
    "Prints out all provided 3 arguments Usage: example <argument> <argument> <argument>";
const char ver_cmd[] PROGMEM = "version";
const char ver_help[] PROGMEM = "Print FW version";
const char ascii_cmd[] PROGMEM = "ascii";
const char ascii_help[] PROGMEM = "Print ASCII tables";
const char number_cmd[] PROGMEM = "number";
const char number_help[] PROGMEM =
    "Print and display matching number Usage: number <decimal number>";
const char mem_cmd[] PROGMEM = "mem";
const char mem_help[] PROGMEM =
    "Print memory usage and change compered to previous call";
const char read_cmd[] PROGMEM = "read";
const char read_help[] PROGMEM = "Read Mifare Card and print card UID";
const char add_cmd[] PROGMEM = "add";
const char add_help[] PROGMEM =
    "Add Mifare Card to list. Usage: add <card uid in HEX> <card holder name>";
const char print_cmd[] PROGMEM = "print";
const char print_help[] PROGMEM = "Print Cards list";
const char rm_cmd[] PROGMEM = "rm";
const char rm_help[] PROGMEM =
    "Remove Mifare card from list. Usage: rm <card uid in HEX>";

const cli_cmd_t cli_cmds[] = {
    {help_cmd, help_help, cli_print_help, 0},
    {ver_cmd, ver_help, cli_print_ver, 0},
    {example_cmd, example_help, cli_example, 3},
    {ascii_cmd, ascii_help, cli_print_ascii_tbls, 0},
    {number_cmd, number_help, cli_handle_number, 1},
    {mem_cmd, mem_help, cli_mem, 0},
    {read_cmd, read_help, cli_rfid_read, 0},
    {add_cmd, add_help, cli_rfid_add, 2},
    {rm_cmd, rm_help, cli_remove_card, 1},
    {print_cmd, print_help, cli_rfid_print, 0}
};

typedef struct card {
    uint8_t *uid;
    size_t size;
    char *name;
    struct card *next;
} card;

card *card_ptr = NULL; // head pointer
//using 0 and 1 instead of true and false
//https://www.le.ac.uk/users/rjm1/cotter/page_37.htm
int iscorrect = 0;
int issame = 0;


void cli_remove_card(const char *const *argv)
{
    // get inserted uid size
    size_t rm_uid_size = strlen(argv[1]) / 2;
    // get inserted uid value, what we will remove
    uint8_t *rm_uid = (uint8_t *)malloc(rm_uid_size * sizeof(uint8_t));
    tallymarker_hextobin(argv[1], rm_uid, rm_uid_size);
    card *current = card_ptr;//create card type variable
    card *last = NULL;

    while (current != NULL) { //searching from cards list
        if (current->size == rm_uid_size) {
            issame = 1;

            for (size_t i = 0; i < rm_uid_size; i++) {
                if (current->uid[i] != rm_uid[i]) {
                    issame = 0;
                    break;
                }
            }

            if (issame) {//if card is found, remove it
                if (last == NULL) {
                    card_ptr = current->next;//if only 1 card in the list
                    uart0_puts_p(PSTR("Removed card UID: "));

                    for (size_t i = 0; i < rm_uid_size; i++) {
                        print_byte_in_hex(rm_uid[i]);
                    }

                    uart0_puts_p(PSTR("\r\n"));
                    free(rm_uid);
                    free(current->uid);
                    free(current->name);
                    free(current);
                    return;
                } else {
                    last->next = current->next;
                    uart0_puts_p(PSTR("Removed card UID: "));

                    for (size_t i = 0; i < rm_uid_size; i++) {
                        print_byte_in_hex(rm_uid[i]);
                    }

                    uart0_puts_p(PSTR("\r\n"));
                    free(rm_uid);
                    free(current->uid);
                    free(current->name);
                    free(current);
                    return;
                }
            }
        }

        last = current;
        current = current->next;
    }

    uart0_puts_p(PSTR("This card is not in the list. Cannot remove! \r\n"));
}


void cli_rfid_add(const char *const *argv)
{
    (void)argv;
    card *new_card;
    new_card = (card *)malloc(sizeof(card));

    //check if memory was allocated
    if (new_card == NULL) {
        uart0_puts_p(PSTR("Memory operation failed!\r\n"));
        return;
    }

    if (card_ptr->uid == NULL) {
        uart0_puts_p(PSTR("Memory operation failed!\r\n"));
        free(card_ptr);
        return;
    }

    //Program does not add new card randomly to the list, when this
    //condition is used to control if memory was allocated
    /*if (new_card->name == NULL) {
        uart0_puts_p(PSTR("Memory operation failed!\r\n"));
        free(new_card->uid);
        free(new_card);
        return;
    }
    */
    new_card = malloc(sizeof(card));
    new_card->size = strlen(argv[1]) / 2;
    new_card->uid = (uint8_t *)malloc(new_card->size * sizeof(uint8_t));
    new_card->name = malloc(sizeof(char) * strlen(argv[2]));
    strcpy(new_card->name, argv[2]);
    tallymarker_hextobin(argv[1], new_card->uid, new_card->size);

    //check UID length

    if (new_card->size > 10) {
        uart0_puts_p(PSTR("UID is more than 10 bytes, cannot add card! \r\n"));
        return;
    }

    //if card is already in the list
    card *c = card_ptr;

    while (c != NULL) {
        if (c->size == new_card->size) {
            issame = 1;

            for (size_t i = 0; i < c->size; i++) {
                if (new_card->uid[i] != c->uid[i]) {
                    issame = 0;
                }
            }

            if (issame) {
                uart0_puts_p(PSTR("Card cannot added. That UID is already in the list! \r\n"));
                return;
            }
        }

        c = c->next;
    }

    new_card->next = card_ptr;
    card_ptr = new_card;
    uart0_puts_p(PSTR("Added card. UID: "));

    for (size_t i = 0; i < card_ptr->size; i++) {
        print_byte_in_hex(card_ptr->uid[i]);
    }

    uart0_puts_p(PSTR(" Size: "));
    print_byte_in_hex(card_ptr->size);
    uart0_puts_p(PSTR(" Holder name: "));
    uart0_puts(card_ptr->name);
    uart0_puts_p(PSTR("\r\n"));
}


void cli_rfid_print(const char *const *argv)
{
    (void) argv;
    char buffer[20];
    int count = 1;
    card *current = card_ptr;

    if (current == NULL) {
        uart0_puts_p(PSTR("No cards in the list! \r\n"));
    }

    while (current != NULL) {
        sprintf_P(buffer, PSTR("%u. "), count);
        uart0_puts(buffer);
        uart0_puts_p(PSTR(" UID["));
        print_byte_in_hex(current->size);
        uart0_puts_p(PSTR("]: "));

        for (size_t i = 0; i < current->size; i++) {
            print_byte_in_hex(current->uid[i]);
        }

        uart0_puts_p(PSTR(" Holder name: "));
        uart0_puts(current->name);
        uart0_puts_p(PSTR("\r\n"));
        current = current->next;
        count++;
    }
}


void cli_print_help(const char *const *argv)
{
    (void) argv;
    uart0_puts_p(PSTR("Implemented commands:\r\n"));

    for (uint8_t i = 0; i < NUM_ELEMS(cli_cmds); i++) {
        uart0_puts_p(cli_cmds[i].cmd);
        uart0_puts_p(PSTR(" : "));
        uart0_puts_p(cli_cmds[i].help);
        uart0_puts_p(PSTR("\r\n"));
    }
}


void cli_example(const char *const *argv)
{
    uart0_puts_p(PSTR("Command had following arguments:\r\n"));

    for (uint8_t i = 1; i < 4; i++) {
        uart0_puts(argv[i]);
        uart0_puts_p(PSTR("\r\n"));
    }
}


void cli_print_ver(const char *const *argv)
{
    (void) argv;
    uart0_puts_p(PSTR(VER_FW));
    uart0_puts_p(PSTR(VER_LIBC));
}


void cli_print_ascii_tbls(const char *const *argv)
{
    (void) argv;
    print_ascii_tbl();
    unsigned char ascii [128] = {0};

    for (unsigned char i = 0; i < 128; i++) {
        ascii[i] = i;
    }

    print_for_human(ascii, 128);
}


void cli_handle_number(const char *const *argv)
{
    int input = atoi(argv[1]);

    for (size_t i = 0; i < strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) {
            uart0_puts_p(ent_num_p);
            uart0_puts_p(PSTR("\r\n"));
            lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
            lcd_goto(LCD_ROW_2_START);
            lcd_puts_P(not_num);
            return;
        }
    }

    if (input >= 0 && input <= 9) {
        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        lcd_goto(LCD_ROW_2_START);
        uart0_puts_p((PGM_P)pgm_read_word(&(numbernames[input])));
        uart0_puts_p(PSTR("\r\n"));
        lcd_puts_P((PGM_P)pgm_read_word(&(numbernames[input])));
    } else {
        uart0_puts_p(console_error_msg);
        uart0_puts_p(PSTR("\r\n"));
        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        lcd_goto(LCD_ROW_2_START);
        lcd_puts_P(lcd_error_msg);
    }
}


void cli_print_cmd_error(void)
{
    uart0_puts_p(PSTR("Command not implemented.\r\n\tUse <help> to get help.\r\n"));
}


void cli_print_cmd_arg_error(void)
{
    uart0_puts_p(
        PSTR("To few or too many arguments for this command\r\n\tUse <help>\r\n"));
}


void cli_mem(const char *const *argv)
{
    (void) argv;
    char print_buf[256] = {0x00};
    extern int __heap_start, *__brkval;
    int v;
    int space;
    static int prev_space;
    space = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
    uart0_puts_p(PSTR("Heap statistics\r\n"));
    sprintf_P(print_buf, PSTR("Used: %u\r\nFree: %u\r\n"), getMemoryUsed(),
              getFreeMemory());
    uart0_puts(print_buf);
    uart0_puts_p(PSTR("\r\nSpace between stack and heap:\r\n"));
    sprintf_P(print_buf, PSTR("Current  %d\r\nPrevious %d\r\nChange   %d\r\n"),
              space, prev_space, space - prev_space);
    uart0_puts(print_buf);
    uart0_puts_p(PSTR("\r\nFreelist\r\n"));
    sprintf_P(print_buf, PSTR("Freelist size:             %u\r\n"),
              getFreeListSize());
    uart0_puts(print_buf);
    sprintf_P(print_buf, PSTR("Blocks in freelist:        %u\r\n"),
              getNumberOfBlocksInFreeList());
    uart0_puts(print_buf);
    sprintf_P(print_buf, PSTR("Largest block in freelist: %u\r\n"),
              getLargestBlockInFreeList());
    uart0_puts(print_buf);
    sprintf_P(print_buf, PSTR("Largest freelist block:    %u\r\n"),
              getLargestAvailableMemoryBlock());
    uart0_puts(print_buf);
    sprintf_P(print_buf, PSTR("Largest allocable block:   %u\r\n"),
              getLargestNonFreeListBlock());
    uart0_puts(print_buf);
    prev_space = space;
}


void cli_rfid_read(const char *const *argv)
{
    (void) argv;
    Uid uid;
    Uid *uid_ptr = &uid;
    byte bufferATQA[10];
    byte bufferSize[10];
    uart1_puts_p(PSTR("\r\n"));

    if (PICC_IsNewCardPresent()) {
        uart0_puts_p(PSTR("Card selected!\r\n"));
        PICC_ReadCardSerial(uid_ptr);
        uart0_puts_p(PSTR("Card type: "));
        uart0_puts(PICC_GetTypeName(PICC_GetType(uid.sak)));
        uart0_puts_p(PSTR("\r\n"));
        uart0_puts_p(PSTR("Card UID: "));

        for (byte i = 0; i < uid.size; i++) {
            print_byte_in_hex(uid.uidByte[i]);
        }

        uart0_puts_p(PSTR(" (size: "));
        print_byte_in_hex(uid.size);
        uart0_puts_p(PSTR(" bytes)"));
        PICC_WakeupA(bufferATQA, bufferSize);
        uart0_puts_p(PSTR("\r\n"));
    } else {
        uart0_puts_p((PSTR("Unable to select card.\r\n")));
    }
}


int cli_execute(int argc, const char *const *argv)
{
    // Move cursor to new line. Then user can see what was entered.
    //FIXME Why microrl does not do it?
    uart0_puts_p(PSTR("\r\n"));

    for (uint8_t i = 0; i < NUM_ELEMS(cli_cmds); i++) {
        if (!strcmp_P(argv[0], cli_cmds[i].cmd)) {
            // Test do we have correct arguments to run command
            // Function arguments count shall be defined in struct
            if ((argc - 1) != cli_cmds[i].func_argc) {
                cli_print_cmd_arg_error();
                return 0;
            }

            // Hand argv over to function via function pointer,
            // cross fingers and hope that funcion handles it properly
            cli_cmds[i].func_p (argv);
            return 0;
        }
    }

    cli_print_cmd_error();
    return 0;
}


void door(void)
{
    Uid uid;
    Uid *uid_ptr = &uid;
    char lcd_buf[16] = {0x00};

    //  // if card is near the reader
    if (PICC_IsNewCardPresent()) {
        PICC_ReadCardSerial(uid_ptr);
        card *current = card_ptr;

        //if the card list is empty, door remains closed
        if (current == NULL) {
            door_state = door_closed;
            display_state = display_access_denied;
        }

        // as long as there are cards in the list
        while (current != NULL) {
            //if presented card is in the list, door opens
            if (memcmp(uid.uidByte, current->uid, uid.size) == 0) {
                door_state = door_opening;

                //if last card was not right, displaying name again
                if (iscorrect == 1) {
                    display_state = display_name;
                } else {
                    //otherwise do nothing
                    display_state = display_no_update;
                }

                //set iscorrect to true
                iscorrect = 1;
                card_read_time = time(0);

                //show card holder name on the screen
                if (strcmp(current_display_name, current->name) != 0) {
                    display_state = display_name;
                    current_display_name = current->name;
                    break;
                } else {
                    return;
                }
            } else {
                iscorrect = 0;
            }

            current = current->next;
        }

        //if card was not in the list, door is closed
        if (iscorrect == 0) {
            door_state = door_closing;
            display_state = display_access_denied;
        }
    }

    now_time = time(NULL);

    //after 3 seconds, clear display
    if (((now_time - card_read_time) > DISPLAY_MSG_IN_SEC) && (iscorrect == 1)) {
        current_display_name = NULL;
        display_state = display_clear;
    }

    switch (door_state) {
    case door_opening:
        // Document door open time
        door_open_time = time(NULL);
        // Unlock door
        door_state = door_open;
        break;

    case door_open:
        time_y2k_cpy = time(NULL);

        //door is open for 2 seconds, then will be closed
        if ((time_y2k_cpy - door_open_time) > DOOR_OPEN_IN_SEC) {
            door_state = door_closing;
        }

        break;

    case door_closing:
        // Lock door
        door_state = door_closed;
        break;

    case door_closed:
        PORTA &= ~_BV(LED_RED);
        break; // No need to do anything
    }

    switch (display_state) {
    case display_name:

        //red LED on, when last card was not in the list
        if (previous_display_state != display_name) {
            PORTA ^= _BV(LED_RED);
        }

        //if last card was not right, displaying name again
        if ((strcmp(previous_display_name, current_display_name) == 0) &&
                (previous_display_state == display_name)) {
            display_state = display_clear;
            previous_display_state = display_name;
            break;
        }

        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        lcd_goto(LCD_ROW_2_START);

        //we need to truncate name display because name can be long
        if (current_display_name != NULL) {
            strncpy(lcd_buf, current_display_name, LCD_VISIBLE_COLS);
            lcd_puts(lcd_buf);
            previous_display_name = current_display_name;
        } else {
            lcd_puts_P(PSTR("Name read error"));
        }

        msg_display_time = time(NULL);
        display_state = display_clear;
        previous_display_state = display_name;
        break;

    case display_access_denied:

        //since last message was same, we do not update
        if (previous_display_state == display_access_denied) {
            display_state = display_clear;
            break;
        }

        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        lcd_goto(LCD_ROW_2_START);
        lcd_puts_P(PSTR(DOOR_ACCESS_DENIED));
        msg_display_time = time(NULL);
        display_state = display_clear;
        previous_display_state = display_access_denied;
        break;

    case display_clear:
        time_y2k_cpy = time(NULL);

        if ((time_y2k_cpy - msg_display_time) > DISPLAY_MSG_IN_SEC) {
            lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
            display_state = display_no_update;
            previous_display_state = display_clear;
        }

        break;

    case display_no_update:
        break;
    }
}
