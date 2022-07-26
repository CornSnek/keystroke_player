#ifndef _PARSER_H_
#define _PARSER_H_
#include "shared_string.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
typedef enum{
    RS_Start,
    RS_Comments,
    RS_RepeatStart,
    RS_RepeatEnd,
    RS_RepeatEndNumber,
    RS_KeyOrMouse,
    RS_KeyState,
    RS_Delay,
    RS_DelayNum,
    RS_MouseClickType,
    RS_MouseClickState,
    RS_MouseMove,
}ReadState;
typedef enum{
    IS_Down,IS_Up,IS_Click
}InputState;
typedef struct repeat_id_manager repeat_id_manager_t;
typedef struct command_array command_array_t;
typedef struct{
    int token_i;
    int line_num;
    int char_num;
    int size;
    char* contents;
    command_array_t* cmd_arr;
    repeat_id_manager_t* rim;
    bool parse_error;
}macro_buffer_t;
typedef struct{
    InputState key_state;
    char* key;
}keystroke_t;
typedef struct repeat_id_manager{//Get ids from created names.
    int size;
    char** names;
    int* index;
    shared_string_manager_t* ssm;
}repeat_id_manager_t;
typedef __uint64_t delay_ns_t;
typedef int repeat_start_t;
typedef struct{
    int index;//Index refers to repeat_start_t's index.
    int counter_max;//0 means infinite loop.
}repeat_end_t;
typedef struct{
    int x;
    int y;
}mouse_move_t;
typedef struct{
    InputState mouse_state;
    int mouse_type;
}mouse_click_t;
typedef union{
    keystroke_t ks;
    delay_ns_t delay;
    repeat_start_t repeat_start;
    repeat_end_t repeat_end;
    mouse_click_t mouse_click;
    mouse_move_t mouse_move;
}command_union_t;
typedef enum{
    VT_KeyStroke,VT_Delay,VT_RepeatStart,VT_RepeatEnd,VT_MouseClick,VT_MouseMove,VT_Exit
}CommandType;
typedef struct{//Aggregating like for SDL events (enums and unions).
    CommandType type;
    command_union_t cmd;
}command_t;
typedef struct command_array{
    int size;
    command_t* cmds;
    shared_string_manager_t* SSM;
}command_array_t;
int trim_whitespace(char**  strptr_owner);
void replace_str(char** strptr, const char* replace, const char* with);
macro_buffer_t* macro_buffer_new(char* str_owned, command_array_t* cmd_arr, repeat_id_manager_t* rim);//macro_buffer_new has ownership to char*.
bool macro_buffer_process_next(macro_buffer_t* mb,bool print_debug);
void macro_buffer_free(macro_buffer_t* this);
repeat_id_manager_t* repeat_id_manager_new();
void repeat_id_manager_add_name(repeat_id_manager_t* this, char*  str_owned, int index);
int repeat_id_manager_search_index(const repeat_id_manager_t* this,const char* search_str);
void repeat_id_manager_free(repeat_id_manager_t* this);
command_array_t* command_array_new();
void command_array_add(command_array_t* this, command_t cmd);
int command_array_count(const command_array_t* this);
void command_array_print(const command_array_t* this);
void command_array_free(command_array_t* this);
bool char_is_key(char c);
bool char_is_keystate(char c);
bool char_is_whitespace(char c);
bool char_is_delay(char c);
#endif
