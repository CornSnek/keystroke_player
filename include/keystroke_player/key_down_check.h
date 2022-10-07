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
#include <parser.h>
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
typedef struct km_grabs_s{//To use XGrabKey/XUngrabKey and check if key was pressed in a different thread when main thread is sleeping.
    xdo_t* xdo_obj;
    keystroke_t* ks_arr;
    bool* ks_pressed;
    int size;
    mouse_button_t b_arr[5];//5 for all mouse buttons.
    bool b_grab_exist[5];
    bool b_pressed[5];
}km_grabs_t;
km_grabs_t* km_grabs_new(xdo_t* xdo_obj);
void km_grabs_kadd(km_grabs_t* this,keystroke_t ks_add);
void km_grabs_badd(km_grabs_t* this,mouse_button_t b_add,bool held_down);
bool km_grabs_kgrab_exist(const km_grabs_t* this,keystroke_t ks);
bool km_grabs_bgrab_exist(const km_grabs_t* this,unsigned int b);
bool km_grabs_get_kpressed(const km_grabs_t* this,keystroke_t ks);
bool km_grabs_get_bpressed(const km_grabs_t* this,unsigned int b);
void km_grabs_set_bpressed(km_grabs_t* this,unsigned int b,bool p);
void km_grabs_kremove(km_grabs_t* this,keystroke_t ks_rm);
void km_grabs_bremove(km_grabs_t* this,mouse_button_t b_rm);
void km_grabs_kremove_all(km_grabs_t* this);
void km_grabs_bremove_all(km_grabs_t* this);
void km_grabs_ktoggle(km_grabs_t* this,bool toggle);
void km_grabs_btoggle(km_grabs_t* this,bool toggle);
void km_grabs_free(km_grabs_t* this);
#endif

