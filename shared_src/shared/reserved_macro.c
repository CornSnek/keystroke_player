#include "reserved_macro.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>
StringMap_ImplDef(r_ts_macro_t,r_ts_macro)
StringMap_r_ts_macro_t* R_TS_MacroFunctions=0;
long EnumValue=0;
static inline char* str_dup(const char* str){
    char* this=malloc(sizeof(char)*(strlen(str)+1));
    EXIT_IF_NULL(this,char*);
    strcpy(this,str);
    return this;
}
//[!\@REP:n:str!] (2 args) Repeats str number of n times.
bool rtsmf_repeat_string(char** output,char** str_arr,int num_args){
    long rep_num=strtol(str_arr[0],NULL,10);
    if(rep_num<=0){
        *output=0;
        fprintf(stderr,ERR("@REP: 1st argument needs to be a valid integer greater than 0.\n"));
        return false;
    }
    size_t strlen_str=strlen(str_arr[1]);
    *output=malloc(sizeof(char)*(strlen_str*rep_num+1));
    EXIT_IF_NULL(*output,char*);
    (*output)[0]='\0';//For strcat in the for loop.
    for(long i=0;i<rep_num;i++) strcat(*output+strlen_str*i,str_arr[1]);
    return (void)num_args,true;
}
//[!\@REPMAC:n:lhs_str:rhs_str:MACRO:marg1:marg2:...:margn!] Repeats a MACRO number of n times from left to right.
bool rtsmf_repeat_macro(char** output,char** str_arr,int num_args){
    if(num_args<4){
        *output=0;
        fprintf(stderr,ERR("@REPMAC: Requires at least 4 or more arguments.\n"));
        return false;
    }
    long rep_num=strtol(str_arr[0],NULL,10);
    if(rep_num<0){
        *output=0;
        fprintf(stderr,ERR("@REPMAC: 1st argument needs to be a valid integer 0 or greater.\n"));
        return false;
    }else if(rep_num>0){
        static const char REPRECTemplate[]="%%s[!%s%%s!]%%s[!@REPMAC:%ld:%%s:%%s:%s%%s!]";
        static const size_t PLongIntDigitMax=19,REPRECTemplateNoneLen=sizeof(REPRECTemplate)-7-1;//-7 to exclude %s and %ld and -1 for '\0'
        char* new_output;
        new_output=malloc(sizeof(char)*(REPRECTemplateNoneLen+PLongIntDigitMax+strlen(str_arr[3])*2+1));
        EXIT_IF_NULL(new_output,char*);
        const int realloc_len=sprintf(new_output,REPRECTemplate,str_arr[3],rep_num-1,str_arr[3]);
        new_output=realloc(new_output,sizeof(char)*(realloc_len+1));//+1 because sprintf doesn't include the '\0'
        EXIT_IF_NULL(new_output,char*);
        char* args=calloc(1,sizeof(char));
        for(int i=4;i<num_args;i++){//Replace the %%s with the arguments of the second macro.
            const size_t args_old_strlen=strlen(args);
            args=realloc(args,sizeof(char)*(args_old_strlen+strlen(str_arr[i])+2));//+2 for ':' and '\0' at end.
            EXIT_IF_NULL(args,char*);
            args[args_old_strlen]=':';
            strcpy(args+args_old_strlen+1,str_arr[i]);
        }
        *output=malloc(sizeof(char)*(realloc_len-12+strlen(args)*2+strlen(str_arr[1])*2+strlen(str_arr[2])*2+1));//-12 for 6 %%s converted to %s.
        EXIT_IF_NULL(*output,char*);
        sprintf(*output,new_output,str_arr[1],args,str_arr[2],str_arr[1],str_arr[2],args);
        free(args);
        free(new_output);
    }else{
        *output=calloc(1,sizeof(char));
        EXIT_IF_NULL(*output,char*);
    }
    return true;
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
        EXIT_IF_NULL(*output,char*);
        strcat(*output+output_old_strlen,str_edit);
        free(str_edit);
    }
    return true;
}
static char LongStrBuf[21]={0};
//[!\@ENUM:lhs_str:rhs_str!] (2 arguments that can be empty) adds a number, that incremenents each call, between lhs_str and rhs_str. Default enumeration is 0 initially.
bool rtsmf_enumerate(char** output,char** str_arr,int num_args){
    snprintf(LongStrBuf,21,"%ld",EnumValue++);
    *output=malloc(sizeof(char)*(strlen(str_arr[0])+strlen(LongStrBuf)+strlen(str_arr[1])+1));
    EXIT_IF_NULL(*output,char*);
    *output[0]='\0';
    strcat(*output,str_arr[0]);
    strcat(*output+strlen(str_arr[0]),LongStrBuf);
    strcat(*output+strlen(str_arr[0])+strlen(LongStrBuf),str_arr[1]);
    return (void)num_args,true;
}
//[!\@ENUMREV:lhs_str:rhs_str!] (2 arguments that can be empty) same as [!\@ENUM:lhs_str:rhs_str!], but enumerating backwards.
bool rtsmf_enumerate_reverse(char** output,char** str_arr,int num_args){
    snprintf(LongStrBuf,21,"%ld",EnumValue--);
    *output=malloc(sizeof(char)*(strlen(str_arr[0])+strlen(LongStrBuf)+strlen(str_arr[1])+1));
    EXIT_IF_NULL(*output,char*);
    *output[0]='\0';
    strcat(*output,str_arr[0]);
    strcat(*output+strlen(str_arr[0]),LongStrBuf);
    strcat(*output+strlen(str_arr[0])+strlen(LongStrBuf),str_arr[1]);
    return (void)num_args,true;
}
//[!\@ENUM_K:lhs_str:rhs_str!] (2 arguments that can be empty), same as [!\@ENUM:...!], but keeps the enumeration the same each call.
bool rtsmf_enumerate_keep(char** output,char** str_arr,int num_args){
    snprintf(LongStrBuf,21,"%ld",EnumValue);
    *output=malloc(sizeof(char)*(strlen(str_arr[0])+strlen(LongStrBuf)+strlen(str_arr[1])+1));
    EXIT_IF_NULL(*output,char*);
    *output[0]='\0';
    strcat(*output,str_arr[0]);
    strcat(*output+strlen(str_arr[0]),LongStrBuf);
    strcat(*output+strlen(str_arr[0])+strlen(LongStrBuf),str_arr[1]);
    return (void)num_args,true;
}
//[!\@ENUM_S:num!] (1 argument) to set the number for @ENUM to string num, a number.
bool rtsmf_enumerate_set(char** output,char** str_arr,int num_args){
    EnumValue=strtol(str_arr[0],NULL,10);
    return (void)output,(void)str_arr,(void)num_args,true;
}
//Temporary.
bool rtsmf_null(char** output,char** str_arr,int num_args){
    return (void)output,(void)str_arr,(void)num_args,false;
}
void R_TS_Macro_Init(){
    if(!R_TS_MacroFunctions){
        R_TS_MacroFunctions=StringMap_r_ts_macro_new(18);
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
        SMVA("@REP",rtsmf_repeat_string,2);
        SMVA("@REPMAC",rtsmf_repeat_macro,-1);
        SMVA("@FOREACH",rtsmf_for_each,-1);
        SMVA("@ENUM",rtsmf_enumerate,2);
        SMVA("@ENUMREV",rtsmf_enumerate_reverse,2);
        SMVA("@ENUM_K",rtsmf_enumerate_keep,2);
        SMVA("@ENUM_S",rtsmf_enumerate_set,1);
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
