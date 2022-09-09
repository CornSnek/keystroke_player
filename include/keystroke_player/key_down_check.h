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
#define __USE_XOPEN
#include <unistd.h>
int usleep(useconds_t usec);
/**
    This is to send xdo_send_keysequence_window_up events for any xdo_send_keysequence_window_down keys.
*/
typedef struct key_down_check_s{
    const char** keys;
    int len;
}key_down_check_t;
typedef bool(*callback_f)(void*);
typedef struct callback_s{
    KeySym ks;
    callback_f func;
    void* arg;
}callback_t;
key_down_check_t* key_down_check_new(void);
bool key_down_check_add(key_down_check_t* this,const char* add_key);
bool key_down_check_remove(key_down_check_t* this,const char* rem_key);
void key_down_check_print(const key_down_check_t* this);
void key_down_check_key_up(key_down_check_t* this,xdo_t* xdo_obj,Window window);
void key_down_check_free(key_down_check_t* this);
void wait_for_keypress(Display* xdpy,KeySym ks);
void keypress_loop(Display* xdpy,const callback_t* cb_list,size_t cb_list_len);
bool CallbackEndLoop(void* v);
typedef struct _boolean_edit_s{
    bool* p;
    bool v;
}_boolean_edit_t;
bool _boolean_edit_func(void* b_v);
#endif

