#ifndef _VARIABLE_LOADER_H_
#define _VARIABLE_LOADER_H_
//Because test and normal binary conflicts with intellisense. 2 is the test binary.
#ifdef __KEYSTROKE_PLAYER__
    #include "hash_map_impl.h"
#elif __KEYSTROKE_PLAYER_TESTS__
    #include "hash_map_impl2.h"
#endif
typedef struct VariableLoader_s VariableLoader_t;
//To store double as a long for VariableLoader_t. First, store number as (LD_u){.l=num}.d and double converts it to (LD_u){.d=double_num}.l
typedef union _LD_u{
    long l;
    double d;
}LD_u;
typedef enum _VLClosureType{
    VLClosure_Double,VLClosure_Long,VLClosure_Int,VLClosure_Char,VLClosure_VDouble,VLClosure_VLong,VLClosure_VInt,VLClosure_VChar
}VLClosureType;
typedef bool(*closure_double_f_t)(double*,long);
typedef bool(*closure_long_f_t)(long*,long);
typedef bool(*closure_int_f_t)(int*,long);
typedef bool(*closure_char_f_t)(char*,long);
typedef bool(*closure_vdouble_f_t)(const VariableLoader_t*,double*,const char*);
typedef bool(*closure_vlong_f_t)(const VariableLoader_t*,long*,const char*);
typedef bool(*closure_vint_f_t)(const VariableLoader_t*,int*,const char*);
typedef bool(*closure_vchar_f_t)(const VariableLoader_t*,char*,const char*);
typedef union vlfunction_union{
    closure_double_f_t as_double;
    closure_long_f_t as_long;
    closure_int_f_t as_int;
    closure_char_f_t as_char;
    closure_vdouble_f_t as_vdouble;
    closure_vlong_f_t as_vlong;
    closure_vint_f_t as_vint;
    closure_vchar_f_t as_vchar;
}vlfunction_union_t;
typedef struct vlargs_variable_s{
    const char* str;
    VariableLoader_t* vl;
}vlargs_variable_t;
typedef union vlargs_union{
    long number;
    vlargs_variable_t variable;
}vlargs_union_t;
typedef struct vloader_closure_s{
    VLClosureType closure_type;
    vlfunction_union_t func;
    vlargs_union_t args;
}vloader_closure_t;
bool ProcessVLClosure(const vloader_closure_t* closure,void* at_address);
typedef struct VariableLoader_s{
    StringMap_long_t* sml;
    vloader_closure_t* closures;
    int closure_size;
}VariableLoader_t;
VariableLoader_t* VariableLoader_new(size_t size);
bool VariableLoader_add_as_d(VariableLoader_t* this,char* variable,double value);
bool VariableLoader_add_as_l(VariableLoader_t* this,char* variable,long value);
bool VariableLoader_rewrite_as_d(VariableLoader_t* this,const char* variable,double new_value);
bool VariableLoader_rewrite_as_l(VariableLoader_t* this,const char* variable,long new_value);
//To hold closure pointer offset information.
typedef int vlclosure_i;
vlclosure_i VariableLoader_new_closure_double(VariableLoader_t* this,long value);
vlclosure_i VariableLoader_new_closure_long(VariableLoader_t* this,long value);
vlclosure_i VariableLoader_new_closure_int(VariableLoader_t* this,long value);
vlclosure_i VariableLoader_new_closure_char(VariableLoader_t* this,long value);
vlclosure_i VariableLoader_new_closure_vdouble(VariableLoader_t* this,const char* variable);
vlclosure_i VariableLoader_new_closure_vlong(VariableLoader_t* this,const char* variable);
vlclosure_i VariableLoader_new_closure_vint(VariableLoader_t* this,const char* variable);
vlclosure_i VariableLoader_new_closure_vchar(VariableLoader_t* this,const char* variable);
const vloader_closure_t* VariableLoader_get_closure(const VariableLoader_t* this,vlclosure_i index);
void VariableLoader_free(VariableLoader_t* this);
#endif