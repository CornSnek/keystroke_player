#ifndef _PARSER_H_
#define _PARSER_H_
#include "shared_string.h"
#include "macros.h"
#include "variable_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <X11/Xlib.h>
//Sringifying enums separately. Add e(number) and #e(number) for a new enum and string.
#define __STR_READ_ENUMS(e1,e2,e3,e4,e5,e6,e7,e8,e9,e10,e11,e12,e13,e14,e15,e16,e17,e18,e19,e20,e21,e22,e23,e24,e25,e26,e27,e28,e29,e30,ecount)\
#e1,#e2,#e3,#e4,#e5,#e6,#e7,#e8,#e9,#e10,#e11,#e12,#e13,#e14,#e15,#e16,#e17,#e18,#e19,#e20,#e21,#e22,#e23,#e24,#e25,#e26,#e27,#e28,#e29,#e30
//For .h file.
#define __ReadStateWithStringDec(...) typedef enum _ReadState{__VA_ARGS__}ReadState;\
extern const char* ReadStateStrings[RS_Count];
//Defining strings in .c file.
#define __ReadStateWithStringDef(...) const char* ReadStateStrings[RS_Count]={__STR_READ_ENUMS(__VA_ARGS__)};
//Editing enums in .h file only.
#define __ReadStateEnums\
    RS_Start,\
    RS_Comments,\
    RS_RepeatStart,\
    RS_RepeatEnd,\
    RS_RepeatEndValue,\
    RS_Keys,\
    RS_KeyState,\
    RS_Delay,\
    RS_DelayValue,\
    RS_MouseClickType,\
    RS_MouseClickState,\
    RS_MoveMouse,\
    RS_JumpTo,\
    RS_JumpToIndex,\
    RS_JumpFrom,\
    RS_Query,\
    RS_QueryComparePixel,\
    RS_QueryCoordsType,\
    RS_QueryCoordsVarValue,\
    RS_QueryCoordsWithin,\
    RS_QueryRPNEval,\
    RS_QueryKeyPress,\
    RS_InitVarType,\
    RS_InitVarName,\
    RS_InitVarValue,\
    RS_EditVarName,\
    RS_EditVarValue,\
    RS_GrabKey,\
    RS_WaitUntilKey,\
    RS_PrintString,\
    RS_Count
__ReadStateWithStringDec(__ReadStateEnums)
typedef enum _InputState{
    IS_Down,IS_Up,IS_Click
}InputState;
typedef struct repeat_id_manager_s repeat_id_manager_t;
typedef struct jump_id_manager_s jump_id_manager_t;
typedef struct command_array_s command_array_t;
typedef struct vp_array_s vp_array_t;
typedef struct macro_buffer_s{
    int token_i;
    int str_size;
    char* contents;
    command_array_t* cmd_arr;
    repeat_id_manager_t* rim;
    jump_id_manager_t* jim;
    VariableLoader_t* vl;
    vp_array_t* vpa;
    bool parse_error;
}macro_buffer_t;
typedef struct auto_keystroke_s{
    InputState key_state;
    const char* key;
}auto_keystroke_t;
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
typedef size_t delay_ns_t;
typedef struct repeat_start_s{
    int counter;
    int str_index;
}repeat_start_t;
typedef struct repeat_end_s{
    int cmd_index;//Index refers to repeat_start_t's index.
    int str_index;
    vlcallback_info counter;
}repeat_end_t;
typedef struct mouse_move_s{
    vlcallback_info x_cb;
    vlcallback_info y_cb;
    bool is_absolute;
}mouse_move_t;
typedef struct mouse_click_s{
    InputState mouse_state;
    int mouse_type;
}mouse_click_t;
typedef struct jump_to_s{
    int cmd_index;
    int str_index;
    bool store_index;
}jump_to_t;
extern const int IndexNotFound;
extern const int JumpFromNotConnected;
typedef struct jump_from_s{
    int str_index;
}jump_from_t;
typedef struct pixel_compare_s{
    vlcallback_info r_cb,g_cb,b_cb,thr_cb;
}pixel_compare_t;
typedef enum _CompareCoords{
    CMP_NULL=0,
    CMP_X=0,
    CMP_Y=1,
    CMP_LT=0,
    CMP_GT=2,
    CMP_NO_EQ=0,
    CMP_W_EQ=4
}CompareCoords;//Bitflags to compare. 0 or 0b000 is "compare if x is less than variable". 7 or 0b111 is "compare if y is greater than or equal than variable"
typedef struct compare_coords_s{
    CompareCoords cmp_flags;
    vlcallback_info var_callback;
}compare_coords_t;
typedef struct coords_within_s{
    vlcallback_info xl_cb,yl_cb,xh_cb,yh_cb;
}coords_within_t;
typedef struct init_var_s{
    as_number_t as_number;
    const char* variable;
}init_var_t;
typedef struct delay_s{
    vlcallback_info callback;
    long delay_mult;
}delay_t;
typedef struct jump_to_index_s{
    vlcallback_info jump_cb;
    bool is_absolute;
}jump_to_index_t;
typedef struct keystroke_s{
    KeySym keysym;
    const char* key;
}keystroke_t;
typedef struct print_string_s{
    const char* str;
    bool newline;
    const char** rpn_strs;
    int rpn_strs_len;
}print_string_t;
typedef union command_union{
    auto_keystroke_t auto_ks;
    delay_t delay;
    init_var_t init_var;
    vlcallback_info edit_var;
    repeat_start_t repeat_start;
    repeat_end_t repeat_end;
    mouse_click_t mouse_click;
    mouse_move_t mouse_move;
    jump_to_t jump_to;
    jump_to_index_t jump_to_index;
    jump_from_t jump_from;
    pixel_compare_t pixel_compare;
    compare_coords_t compare_coords;
    coords_within_t coords_within;
    vlcallback_info rpn_eval;
    keystroke_t key_pressed;
    keystroke_t grab_key;
    keystroke_t wait_until_key;
    print_string_t print_string;
}command_union_t;
typedef enum _CommandType{
    CMD_KeyStroke,
    CMD_Delay,
    CMD_RepeatStart,
    CMD_RepeatEnd,
    CMD_MouseClick,
    CMD_MoveMouse,
    CMD_Exit,
    CMD_Pass,
    CMD_JumpTo,
    CMD_JumpToIndex,
    CMD_JumpFrom,
    CMD_JumpBack,
    CMD_RepeatResetCounters,
    CMD_SaveMouseCoords,
    CMD_LoadMouseCoords,
    CMD_QueryComparePixel,
    CMD_QueryCompareCoords,
    CMD_QueryCoordsWithin,
    CMD_QueryRPNEval,
    CMD_QueryKeyPress,
    CMD_InitVar,
    CMD_EditVar,
    CMD_WaitUntilKey,
    CMD_GrabKey,
    CMD_UngrabKeyAll,
    CMD_PrintString,
}CommandType;
typedef enum _CommandSubType{
    CMDST_Command,
    CMDST_Jump,
    CMDST_Query,
    CMDST_Var
}CommandSubType;
typedef struct command_s{//Aggregating like for SDL events (enums and unions).
    CommandType type;
    command_union_t cmd_u;
    CommandSubType subtype;//For query commands.
    int query_jump_ne;
    bool invert_query;
    bool print_cmd;
    char* start_cmd_p;
    char* end_cmd_p;
}command_t;
typedef struct command_array_s{
    int size;
    command_t* cmds;
    shared_string_manager_t* SSM;
}command_array_t;
typedef struct vp_array_s{//Container to free any pointer types when vp_array_free is used.
    void** vp_arr;
    int size;
}vp_array_t;
macro_buffer_t* macro_buffer_new(char* str_owned, command_array_t* cmd_arr);
bool macro_buffer_process_next(macro_buffer_t* this,bool print_debug);
void macro_buffer_str_id_check(macro_buffer_t* this,const VariableLoader_t* vl);
void macro_buffer_free(macro_buffer_t* this);
repeat_id_manager_t* repeat_id_manager_new(void);
void repeat_id_manager_add_name(repeat_id_manager_t* this, char* str_owned, int index);
int repeat_id_manager_search_command_index(const repeat_id_manager_t* this,const char* search_str);
int repeat_id_manager_search_string_index(const repeat_id_manager_t* this,const char* search_str);
void repeat_id_manager_free(repeat_id_manager_t* this);
jump_id_manager_t* jump_id_manager_new(void);
void jump_id_manager_add_name(jump_id_manager_t* this, char* str_owned, int index, bool is_jump_from);
int jump_id_manager_search_command_index(const jump_id_manager_t* this,const char* search_str);
int jump_id_manager_search_string_index(const jump_id_manager_t* this,const char* search_str);
bool jump_id_manager_set_command_index_once(jump_id_manager_t* this, int index, int cmd_index);
void jump_id_manager_free(jump_id_manager_t* this);
command_array_t* command_array_new(void);
void command_array_add(command_array_t* this, command_t cmd);
int command_array_count(const command_array_t* this);
void command_array_print(const command_array_t* this,const VariableLoader_t* vl,unsigned char decimals);
void command_array_free(command_array_t* this);
vp_array_t* vp_array_new(void);
void vp_array_add(vp_array_t* this,void* p);
void vp_array_free(vp_array_t* this);
#endif
