#ifndef MENU_H
#define MENU_H

#define MENU_MAIN   0
#define MENU_CLOCK  1
#define MENU_TEMP   2
#define MENU_UART   3

void menu_init(void);
void menu_show(void);
void menu_process_key(char key);
unsigned char menu_get_state(void);

#endif