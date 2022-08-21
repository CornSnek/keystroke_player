#ifndef _VARIABLE_LOADER_H_
#define _VARIABLE_LOADER_H_
//Because test and normal binary conflicts with intellisense. 2 is the test binary.
#ifdef __KEYSTROKE_PLAYER__
    #include "hash_map_impl.h"
#elif __KEYSTROKE_PLAYER_TESTS__
    #include "hash_map_impl2.h"
#endif
#include "shared_string.h"
typedef struct VariableLoader_s VariableLoader_t;
//To store double as a long for VariableLoader_t. First, store number as (LD_u){.l=num}.d and double converts it to (LD_u){.d=double_num}.l
typedef enum _LD_e{
    LD_L,LD_D
}LD_e;
typedef union _LD_u{
    long l;
    double d;
}LD_u;
typedef enum _VLCallbackType{
    VLCallback_Double,
    VLCallback_Long,
    VLCallback_Int,
    VLCallback_Char,
    VLCallback_VDouble,
    VLCallback_VLong,
    VLCallback_VInt,
    VLCallback_VChar,
    VLCallback_AddAsLong,
    VLCallback_AddAsDouble,
    VLCallback_RewriteAsLong,
    VLCallback_RewriteAsDouble
}VLCallbackType;
typedef enum _VLCallbackSubtype{
    VLCallbackST_Char,
    VLCallbackST_Int,
    VLCallbackST_Long,
    VLCallbackST_Double
}VLCallbackSubtype;
static inline const char* VLCallbackSubtypeStr(VLCallbackSubtype vlcst){
    return  vlcst==VLCallbackST_Char?"char":
            vlcst==VLCallbackST_Int?"int":
            vlcst==VLCallbackST_Long?"long":"double";
}
typedef bool(*vlcallback_double_f_t)(double*,double);
typedef bool(*vlcallback_long_f_t)(long*,long);
typedef bool(*vlcallback_int_f_t)(int*,int);
typedef bool(*vlcallback_char_f_t)(char*,char);
typedef bool(*vlcallback_vdouble_f_t)(const VariableLoader_t*,double*,const char*);
typedef bool(*vlcallback_vlong_f_t)(const VariableLoader_t*,long*,const char*);
typedef bool(*vlcallback_vint_f_t)(const VariableLoader_t*,int*,const char*);
typedef bool(*vlcallback_vchar_f_t)(const VariableLoader_t*,char*,const char*);
typedef bool(*vlcallback_add_as_l)(VariableLoader_t*,const char*,long);
typedef bool(*vlcallback_add_as_d)(VariableLoader_t*,const char*,double);
typedef union vlfunction_union{
    vlcallback_double_f_t as_double;
    vlcallback_long_f_t as_long;
    vlcallback_int_f_t as_int;
    vlcallback_char_f_t as_char;
    vlcallback_vdouble_f_t as_vdouble;
    vlcallback_vlong_f_t as_vlong;
    vlcallback_vint_f_t as_vint;
    vlcallback_vchar_f_t as_vchar;
    vlcallback_add_as_l as_add_as_l;
    vlcallback_add_as_d as_add_as_d;
}vlfunction_union_t;
typedef union vlargs_union{
    long number;
    double dnumber;
    const char* variable;
}vlargs_union_t;
typedef struct vlcallback_s{
    VLCallbackType callback_type;
    VLCallbackSubtype subtype;
    vlfunction_union_t func;
    vlargs_union_t args;
}vlcallback_t;
//To hold callback pointer offset information and its type.
typedef struct vlcallback_info_s{
    int i;
}vlcallback_info;
bool ProcessVLCallback(VariableLoader_t* vl,vlcallback_info vlc_info,void* at_address);
vlcallback_t* VL_get_callback(const VariableLoader_t* this,vlcallback_info vlc_info);
typedef struct VariableLoader_s{//Class that contains callbacks to load/save variables to a string using ProcessVLCallback.
    StringMap_long_t* sml;
    vlcallback_t* callbacks;
    int callback_size;
    shared_string_manager_t* ssm;
}VariableLoader_t;
VariableLoader_t* VL_new(size_t size);
//Add variables to hash table to (re)modify values.
vlcallback_info VL_new_callback_add_as_l(VariableLoader_t* this,char* variable);
vlcallback_info VL_new_callback_add_as_d(VariableLoader_t* this,char* variable);
vlcallback_info VL_new_callback_rewrite_as_l(VariableLoader_t* this,char* variable);
vlcallback_info VL_new_callback_rewrite_as_d(VariableLoader_t* this,char* variable);
//Constant values (Doesn't use VariableLoader)
vlcallback_info VL_new_callback_double(VariableLoader_t* this,double value);
vlcallback_info VL_new_callback_long(VariableLoader_t* this,long value);
vlcallback_info VL_new_callback_int(VariableLoader_t* this,int value);
vlcallback_info VL_new_callback_char(VariableLoader_t* this,char value);
//Variables (Uses VaribleLoader)
vlcallback_info VL_new_callback_vdouble(VariableLoader_t* this,char* variable);
vlcallback_info VL_new_callback_vlong(VariableLoader_t* this,char* variable);
vlcallback_info VL_new_callback_vint(VariableLoader_t* this,char* variable);
vlcallback_info VL_new_callback_vchar(VariableLoader_t* this,char* variable);
void VL_free(VariableLoader_t* this);
#endif