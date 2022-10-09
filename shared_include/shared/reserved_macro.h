#ifndef _RESERVED_MACRO_H_
#define _RESERVED_MACRO_H_
#include "shared_string.h"
#include <stdbool.h>
#include <generics/hash_map.h>
typedef struct r_ts_macro_s{
    bool(*parse_f)(char**,char**,int);
    int num_args;
}r_ts_macro_t;
StringMap_ImplDecl(r_ts_macro_t,r_ts_macro,(const char* str){
    hash_t hash=5381;//djb hashing
    int c;
    while((c=*str++)) hash=((hash<<5)+hash)+c;
    return hash;
})
void R_TS_Macro_Init();
bool R_TS_Macro_IsReserved(const char* str_name);
void R_TS_Macro_ResetState();
bool R_TS_Macro_GetString(char** arg_arr,const char* str_name,char** output,int num_args);
void R_TS_Macro_Free();
bool rtsfm_repeat_string_recursive(char** output,char** str_arr,int num_args);
#endif


