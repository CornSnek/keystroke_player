#ifndef _VARIABLE_LOADER_H_
#define _VARIABLE_LOADER_H_
#include "shared_string.h"
typedef struct VariableLoader_s VariableLoader_t;
//To store double as a long for VariableLoader_t. First, store number as (LD_u){.l=num}.d and double converts it to (LD_u){.d=double_num}.l
typedef union _LD_u{
    long l;
    double d;
}LD_u;
typedef enum _VLNumberType{
    VLNT_Invalid,
    VLNT_Char,//Used for indexing and to get number type. Do not touch the ordering.
    VLNT_Int,
    VLNT_Long,
    VLNT_Double
}VLNumberType;
typedef struct as_number_s{
    union{
        char c;
        int i;
        long l;
        double d;
    };
    VLNumberType type;
}as_number_t;
typedef struct as_number_opt_s{
    bool exists;
    as_number_t v;
}as_number_opt_t;
typedef enum _VLCallbackType{
    VLCallback_Double,
    VLCallback_Long,
    VLCallback_Int,
    VLCallback_Char,
    VLCallback_NumberRPN,
    VLCallback_LoadVariable,
    VLCallback_RewriteVariable,
    VLCallback_RewriteVariableRPN
}VLCallbackType;
static inline const char* VLNumberTypeStr(VLNumberType vlnt){
    return  vlnt==VLNT_Char?"char":
            vlnt==VLNT_Int?"int":
            vlnt==VLNT_Long?"long":
            vlnt==VLNT_Double?"double":"NaN";
}
as_number_opt_t String_to_as_number_t(const char* token);
void VLNumberPrintNumber(as_number_t num,unsigned decimals);
as_number_t VLNumberCast(as_number_t num,VLNumberType type);
typedef bool(*vlcallback_double_f_t)(as_number_t*,double);
typedef bool(*vlcallback_long_f_t)(as_number_t*,long);
typedef bool(*vlcallback_int_f_t)(as_number_t*,int);
typedef bool(*vlcallback_char_f_t)(as_number_t*,char);
typedef bool(*vlcallback_number_rpn_f_t)(const VariableLoader_t*,as_number_t*,const char*,bool);
typedef bool(*vlcallback_load_variable_f_t)(const VariableLoader_t*,as_number_t*,const char*);
typedef bool(*vlcallback_rewrite_variable_f_t)(VariableLoader_t*,as_number_t*,const char*);
typedef bool(*vlcallback_rewrite_variable_rpn_f_t)(VariableLoader_t*,const char*,const char*,bool);
typedef union vlfunction_union{
    vlcallback_double_f_t as_double;
    vlcallback_long_f_t as_long;
    vlcallback_int_f_t as_int;
    vlcallback_char_f_t as_char;
    vlcallback_load_variable_f_t as_load_variable;
    vlcallback_rewrite_variable_f_t as_rewrite_variable;
    vlcallback_rewrite_variable_rpn_f_t as_rewrite_variable_rpn;
    vlcallback_number_rpn_f_t as_number_rpn;
}vlfunction_union_t;
typedef struct vlargs_rpn_s{
    const char* variable;
    const char* rpn_str;
    bool see_stack;
}vlargs_rpn_t;
typedef struct vlargs_an_rpn_s{
    const char* rpn_str;
    bool see_stack;
}vlargs_an_rpn_t;
typedef union vlargs_union{
    long number;
    double dnumber;
    const char* variable;
    vlargs_an_rpn_t an_rpn;
    vlargs_rpn_t rpn;
}vlargs_union_t;
typedef struct vlcallback_s{
    VLCallbackType callback_type;
    VLNumberType number_type;
    vlfunction_union_t func;
    vlargs_union_t args;
}vlcallback_t;
//To hold callback pointer offset information and its type.
typedef struct vlcallback_info_s{
    int i;
}vlcallback_info;
bool ProcessVLCallback(VariableLoader_t* vl,vlcallback_info vlc_info,as_number_t* number_io);
vlcallback_t* VL_get_callback(const VariableLoader_t* this,vlcallback_info vlc_info);
#include <generics/hash_map.h>
StringMap_ImplDecl(as_number_t,as_number,
(const char* str){
    hash_t hash=5381;//djb hashing
    int c;
    while((c=*str++)) hash=((hash<<5)+hash)+c;
    return hash;
})
typedef struct VariableLoader_s{//Class that contains callbacks to load/save variables to a string using ProcessVLCallback.
    StringMap_as_number_t* sman;
    vlcallback_t* callbacks;
    int callback_size;
    shared_string_manager_t* ssm;
}VariableLoader_t;
VariableLoader_t* VL_new(size_t size);
ValueAssignE VL_add_as_double(VariableLoader_t* this,char** variable,double value);
ValueAssignE VL_add_as_long(VariableLoader_t* this,char** variable,long value);
ValueAssignE VL_add_as_int(VariableLoader_t* this,char** variable,int value);
ValueAssignE VL_add_as_char(VariableLoader_t* this,char** variable,char value);
//Constant values (Doesn't use VariableLoader for callback.)
vlcallback_info VL_new_callback_double(VariableLoader_t* this,double value);
vlcallback_info VL_new_callback_long(VariableLoader_t* this,long value);
vlcallback_info VL_new_callback_int(VariableLoader_t* this,int value);
vlcallback_info VL_new_callback_char(VariableLoader_t* this,char value);
//Variables (Uses VaribleLoader)
vlcallback_info VL_new_callback_load_variable(VariableLoader_t* this,char* variable);
vlcallback_info VL_new_callback_rewrite_variable(VariableLoader_t* this,char* variable);
vlcallback_info VL_new_callback_rewrite_variable_rpn(VariableLoader_t* this,char* rpn_str,char* variable,bool see_stack);
//Uses VariableLoader and RPN.
vlcallback_info VL_new_callback_number_rpn(VariableLoader_t* this,char* rpn_str,bool see_stack);
//For RPNEvaluator
StringMapOpt_as_number_t VL_get_as_number(const VariableLoader_t* this,const char* variable);
void VL_free(VariableLoader_t* this);
#endif