#ifndef _PARSER_H_
#define _PARSER_H_
#include "shared_string.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//Hackish way of stringifying enums separately. Add e(number) for a new state.
#define __STR_READ_ENUMS(e1,e2,e3,e4,e5,e6,e7,e8,e9,e10,e11,e12,e13,e14,ecount) #e1,#e2,#e3,#e4,#e5,#e6,#e7,#e8,#e9,#e10,#e11,#e12,#e13,#e14
//For .h file.
#define __ReadStateWithStringDec(...) typedef enum _ReadState{__VA_ARGS__}ReadState;\
extern const char* ReadStateStrings[RS_Count];
//Defining strings in .c file.
#define __ReadStateWithStringDef(...) const char* ReadStateStrings[RS_Count]={__STR_READ_ENUMS(__VA_ARGS__)};
//One source of enums here.
#define __ReadStateEnums\
    RS_Start,\
    RS_Comments,\
    RS_RepeatStart,\
    RS_RepeatEnd,\
    RS_RepeatEndNumber,\
    RS_KeyOrMouse,\
    RS_KeyState,\
    RS_Delay,\
    RS_DelayNum,\
    RS_MouseClickType,\
    RS_MouseClickState,\
    RS_MouseMove,\
    RS_JumpTo,\
    RS_JumpFrom,\
    RS_Count
__ReadStateWithStringDec(__ReadStateEnums)
typedef enum{
    IS_Down,IS_Up,IS_Click
}InputState;
typedef struct repeat_id_manager_s repeat_id_manager_t;
typedef struct jump_id_manager_s jump_id_manager_t;
typedef struct command_array_s command_array_t;
typedef struct macro_buffer_s{
    int token_i;
    int line_num;
    int char_num;
    int size;
    char* contents;
    command_array_t* cmd_arr;
    repeat_id_manager_t* rim;
    jump_id_manager_t* jim;
    bool parse_error;
}macro_buffer_t;
typedef struct keystroke_s{
    InputState key_state;
    char* key;
}keystroke_t;
typedef struct repeat_id_manager_s{//Get ids from created names.
    int size;
    char** names;
    int* index;
    shared_string_manager_t* ssm;
}repeat_id_manager_t;
typedef struct jump_id_manager_s{
    int size;
    char** names;
    int* index;
    bool* jump_from_added;
    shared_string_manager_t* ssm;
}jump_id_manager_t;
typedef __uint64_t delay_ns_t;
typedef int repeat_start_t;
typedef struct repeat_end_s{
    int index;//Index refers to repeat_start_t's index.
    int counter_max;//0 means infinite loop.
}repeat_end_t;
typedef struct mouse_move_s{
    int x;
    int y;
}mouse_move_t;
typedef struct mouse_click_s{
    InputState mouse_state;
    int mouse_type;
}mouse_click_t;
typedef struct jump_to_s{
    int cmd_index;
    int str_index;
}jump_to_t;
extern const int JumpToNotFound;
extern const int JumpFromNotConnected;
typedef struct jump_from_s{
    int str_index;
}jump_from_t;
typedef union command_union{
    keystroke_t ks;
    delay_ns_t delay;
    repeat_start_t repeat_start;
    repeat_end_t repeat_end;
    mouse_click_t mouse_click;
    mouse_move_t mouse_move;
    jump_to_t jump_to;
    jump_from_t jump_from;
}command_union_t;
typedef enum _CommandType{
    VT_KeyStroke,VT_Delay,VT_RepeatStart,VT_RepeatEnd,VT_MouseClick,VT_MouseMove,VT_Exit,VT_JumpTo,VT_JumpFrom
}CommandType;
typedef struct command_s{//Aggregating like for SDL events (enums and unions).
    CommandType type;
    command_union_t cmd;
}command_t;
typedef struct command_array_s{
    int size;
    command_t* cmds;
    shared_string_manager_t* SSM;
}command_array_t;
int trim_whitespace(char**  strptr_owner);
void replace_str(char** strptr, const char* replace, const char* with);
macro_buffer_t* macro_buffer_new(char* str_owned, command_array_t* cmd_arr);
bool macro_buffer_process_next(macro_buffer_t* mb,bool print_debug);
void macro_buffer_free(macro_buffer_t* this);
repeat_id_manager_t* repeat_id_manager_new();
void repeat_id_manager_add_name(repeat_id_manager_t* this, char* str_owned, int index);
int repeat_id_manager_search_command_index(const repeat_id_manager_t* this,const char* search_str);
void repeat_id_manager_free(repeat_id_manager_t* this);
jump_id_manager_t* jump_id_manager_new();
void jump_id_manager_add_name(jump_id_manager_t* this, char* str_owned, int index, bool is_jump_from);
int jump_id_manager_search_command_index(const jump_id_manager_t* this,const char* search_str);
int jump_id_manager_search_string_index(const jump_id_manager_t* this,const char* search_str);
bool jump_id_manager_set_command_index_once(jump_id_manager_t* this, int index, int cmd_index);
void jump_id_manager_free(jump_id_manager_t* this);
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
