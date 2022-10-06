#ifndef _RESERVED_MACRO_H_
#define _RESERVED_MACRO_H_
#include "shared_string.h"
#include <stdbool.h>
#include <generics/hash_map.h>
typedef struct r_ts_macro_s{
    bool(*parse_f)(char**,char**);
    int num_args;
}r_ts_macro_t;
StringMap_ImplDecl(r_ts_macro_t,r_ts_macro,(const char* str){
    hash_t hash=5381;//djb hashing
    int c;
    while((c=*str++)) hash=((hash<<5)+hash)+c;
    return hash;
})
extern void R_TS_Macro_Init();
bool R_TS_Macro_IsReserved(const char* str_name);
bool R_TS_Macro_GetString(char** arg_arr,const char* str_name,char** output,int num_args);
extern void R_TS_Macro_Free();
#endif