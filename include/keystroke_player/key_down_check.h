#ifndef _KEY_DOWN_CHECK_H_
#define _KEY_DOWN_CHECK_H_
#define __USE_XOPEN
#include <unistd.h>
#include <xdo.h>
#include "macros.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/**
    This is to send xdo_send_keysequence_window_up events for any xdo_send_keysequence_window_down keys.
*/
typedef struct key_down_check_s{
    const char** keys;
    int len;
}key_down_check_t;

key_down_check_t* key_down_check_new(void);
bool key_down_check_add(key_down_check_t* this,const char* add_key);
bool key_down_check_remove(key_down_check_t* this,const char* rem_key);
void key_down_check_print(key_down_check_t* this);
void key_down_check_key_up(key_down_check_t* this,xdo_t* xdo_obj,Window window);
void key_down_check_free(key_down_check_t* this);
#endif