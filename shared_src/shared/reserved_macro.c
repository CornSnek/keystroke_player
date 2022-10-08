#include "reserved_macro.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>
StringMap_ImplDef(r_ts_macro_t,r_ts_macro)
StringMap_r_ts_macro_t* R_TS_MacroFunctions=0;
bool repeat_string(char** output,char** str_arr){
    long rep_num=strtol(str_arr[1],NULL,10);
    if(rep_num<=0){
        *output=0;
        fprintf(stderr,ERR("2nd argument needs to be a valid integer greater than 0.\n"));
        return false;
    }
    size_t strlen_str=strlen(str_arr[0]);
    *output=malloc(sizeof(char)*(strlen_str*rep_num+1));
    EXIT_IF_NULL(*output,char*);
    (*output)[0]='\0';//For strcat in the for loop.
    for(long i=0;i<rep_num;i++) strcat(*output+strlen_str*i,str_arr[0]);
    return true;
}
void R_TS_Macro_Init(){
    if(!R_TS_MacroFunctions){
        R_TS_MacroFunctions=StringMap_r_ts_macro_new(10);
        ValueAssignE status;
        bool is_ph=true;
#define SMVA_DEBUG 0
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
        SMVA("@REP",repeat_string,2);
#if SMVA_DEBUG==1
        StringMap_r_ts_macro_print_debug(R_TS_MacroFunctions);
#endif
#undef SMVA
#undef SMVA_DEBUG
        if(!is_ph){
            fprintf(stderr,ERR("R_TS_MacroFunctions should be a perfect hash. Exiting program.\n"));
            exit(EXIT_FAILURE);
        }
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
bool R_TS_Macro_GetString(char** arg_arr,const char* str_name,char** output,int num_args){
    R_TS_Macro_InitCalledFirst();
    StringMapOpt_r_ts_macro_t smo_rtsm=StringMap_r_ts_macro_read_ph(R_TS_MacroFunctions,str_name);
    if(smo_rtsm.exists){
        if(num_args!=smo_rtsm.value.num_args){
            fprintf(stderr,ERR("Improper number of arguments for reserved macro '%s' (Should be %d)\n"),str_name,smo_rtsm.value.num_args);
            return false;
        }
        bool is_success=smo_rtsm.value.parse_f(output,arg_arr);
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
