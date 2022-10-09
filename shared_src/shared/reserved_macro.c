#include "reserved_macro.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>
StringMap_ImplDef(r_ts_macro_t,r_ts_macro)
StringMap_r_ts_macro_t* R_TS_MacroFunctions=0;
int EnumValue=0;
static inline char* str_dup(const char* str){
    char* this=malloc(sizeof(char)*(strlen(str)+1));
    strcpy(this,str);
    return this;
}
//[!\@REP:str:n!] (3 args) Repeats str number of n times.
bool rtsmf_repeat_string(char** output,char** str_arr,int num_args){
    long rep_num=strtol(str_arr[1],NULL,10);
    if(rep_num<=0){
        *output=0;
        fprintf(stderr,ERR("@REP: 2nd argument needs to be a valid integer greater than 0.\n"));
        return false;
    }
    size_t strlen_str=strlen(str_arr[0]);
    *output=malloc(sizeof(char)*(strlen_str*rep_num+1));
    EXIT_IF_NULL(*output,char*);
    (*output)[0]='\0';//For strcat in the for loop.
    for(long i=0;i<rep_num;i++) strcat(*output+strlen_str*i,str_arr[0]);
    return (void)num_args,true;
}
//[!\@FOREACH:rep_str:whole_str:str1:str2:...:strn!] (Args greater than 2), whole_str contains rep_str, where rep_str is replaced by str1 to strn.
//Similar to \@REP but each iteration is differentiated by str1 to strn. Ex: [!\@FOREACH:a:abcde:1:2:3!] outputs "1bcde2bcde3bcde"
bool rtsmf_for_each(char** output,char** str_arr,int num_args){
    if(num_args<3){
        *output=0;
        fprintf(stderr,ERR("@FOREACH: Requires at least 3 or more arguments.\n"));
        return false;
    }
    *output=calloc(1,sizeof(char));
    EXIT_IF_NULL(*output,char*);
    for(int i=2;i<num_args;i++){
        char* str_edit=str_dup(str_arr[1]);
        replace_str(&str_edit,str_arr[0],str_arr[i]);
        const size_t output_old_strlen=strlen(*output);
        *output=realloc(*output,sizeof(char)*(output_old_strlen+strlen(str_edit)+1));
        strcat(*output+output_old_strlen,str_edit);
        free(str_edit);
    }
    return true;
}
//[!\@C!] outputs ":"
bool rtsmf_colon(char** output,char** str_arr,int num_args){
    *output=malloc(sizeof(char)*2);
    EXIT_IF_NULL(*output,char*);
    memcpy(*output,":",2);
    return (void)str_arr,(void)num_args,true;
}
//[!\@ENUM:lhs_str:rhs_str!] adds a number between lhs_str and rhs_str, use [!\@ENUM_S:num!] to reset.
bool rtsmf_enumerate(char** output,char** str_arr,int num_args){
    //TODO
    return (void)output,(void)str_arr,(void)num_args,false;
}
//[!\@ENUM_S:num!] to set the number for @ENUM to string num. Must be a number. TODO
bool rtsmf_enumerate_set(char** output,char** str_arr,int num_args){
    //TODO
    return (void)output,(void)str_arr,(void)num_args,false;
}
//Temporary.
bool rtsmf_null(char** output,char** str_arr,int num_args){
    return (void)output,(void)str_arr,(void)num_args,false;
}
void R_TS_Macro_Init(){
    if(!R_TS_MacroFunctions){
        R_TS_MacroFunctions=StringMap_r_ts_macro_new(9);
        ValueAssignE status;
        bool is_ph=true;
#define SMVA_DEBUG 1
#if SMVA_DEBUG==1
    #define SMVA(Str,Function,Args)\
    status=StringMap_r_ts_macro_assign_ph(R_TS_MacroFunctions,Str,(r_ts_macro_t){.parse_f=Function,.num_args=Args});\
    printf("Placing token: "  Str ": ");\
    puts((status==VA_Written)?"OK":ERR("Collision has Occured! Not a Perfect Hash."));\
    if(is_ph) is_ph=(status==VA_Written);\
    (void)0
#else
    #define SMVA(Str,Function,Args)\
    status=StringMap_r_ts_macro_assign_ph(R_TS_MacroFunctions,Str,(r_ts_macro_t){.parse_f=Function,.num_args=Args});\
    if(is_ph) is_ph=(status==VA_Written);\
    (void)0
#endif
        SMVA("@REP",rtsmf_repeat_string,2);
        SMVA("@FOREACH",rtsmf_for_each,-1);
        SMVA("@C",rtsmf_colon,0);
        SMVA("@ENUM",rtsmf_null,2);
        SMVA("@ENUM_S",rtsmf_null,1);
#if SMVA_DEBUG==1
        StringMap_r_ts_macro_print_debug(R_TS_MacroFunctions);
#endif
#undef SMVA
#undef SMVA_DEBUG
        if(!is_ph){
            fprintf(stderr,ERR("R_TS_MacroFunctions should be a perfect hash. Exiting program.\n"));
            exit(EXIT_FAILURE);
        }
        R_TS_Macro_ResetState();
    }else puts(ERR("StringMap for Reserved Macro Functions not initialized."));
}
#ifndef NDEBUG
#define R_TS_Macro_InitCalledFirst() \
if(!R_TS_MacroFunctions){\
    fprintf(stderr,ERR("R_TS_Macro_Init should be called first.\n"));\
    exit(EXIT_FAILURE);\
} (void)0
#else
#define R_TS_Macro_InitCalledFirst() (void)0
#endif
bool R_TS_Macro_IsReserved(const char* str_name){
    R_TS_Macro_InitCalledFirst();
    return StringMap_r_ts_macro_read_ph(R_TS_MacroFunctions,str_name).exists;
}
void R_TS_Macro_ResetState(){
    R_TS_Macro_InitCalledFirst();
    EnumValue=0;
}
bool R_TS_Macro_GetString(char** arg_arr,const char* str_name,char** output,int num_args){
    R_TS_Macro_InitCalledFirst();
    StringMapOpt_r_ts_macro_t smo_rtsm=StringMap_r_ts_macro_read_ph(R_TS_MacroFunctions,str_name);
    if(smo_rtsm.exists){
        if(smo_rtsm.value.num_args!=-1&&num_args!=smo_rtsm.value.num_args){
            fprintf(stderr,ERR("Improper number of arguments for reserved macro '%s' (Should be %d)\n"),str_name,smo_rtsm.value.num_args);
            return false;
        }
        bool is_success=smo_rtsm.value.parse_f(output,arg_arr,num_args);
        if(!is_success) fprintf(stderr,ERR("One of the reserved macros have not successfully processed.\n"));
        return is_success;
    }
    fprintf(stderr,ERR("Macro name, '%s', is not a defined reserved macro.\n"),str_name);
    return false;
}
void R_TS_Macro_Free(){
    StringMap_r_ts_macro_free(R_TS_MacroFunctions);
    R_TS_MacroFunctions=0;
}
