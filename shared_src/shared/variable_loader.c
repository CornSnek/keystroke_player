#include "variable_loader.h"
StringMap_ImplDef(as_number_t,as_number)
bool _VL_callback_rewrite_variable(VariableLoader_t* this,as_number_t* new_value,const char* variable);
bool _VL_callback_rewrite_variable_rpn(VariableLoader_t* this,const char* rpn_str,const char* variable,bool see_stack);
bool _VL_callback_double_func(as_number_t* at_address,double value);
bool _VL_callback_long_func(as_number_t* at_address,long value);
bool _VL_callback_int_func(as_number_t* at_address,int value);
bool _VL_callback_char_func(as_number_t* at_address,char value);
bool _VL_callback_number_rpn(const VariableLoader_t* vl,as_number_t* at_address,const char* rpn_str,bool see_stack);
bool _VL_callback_load_variable_func(const VariableLoader_t* this,as_number_t* at_address,const char* variable);
as_number_opt_t String_to_as_number_t(const char* token){
    if(strlen(token)==1&&*token=='-') return (as_number_opt_t){0};//No name collision with minus sign.
    VLNumberType vlnt=VLNT_Long;//Default
    for(size_t i=0;i<strlen(token);i++){
        const char current_char=token[i];
        if(isdigit(current_char)) continue;
        if(i==0&&current_char=='-') continue;
        if(current_char=='.'){
            if(vlnt==VLNT_Double) return (as_number_opt_t){0};//Only one dot.
            if(vlnt!=VLNT_Double) vlnt=VLNT_Double;
            continue;
        }
        if(vlnt!=VLNT_Double&&i==strlen(token)-1){
            switch(current_char){
                case 'c': vlnt=VLNT_Char; goto valid_num;
                case 'i': vlnt=VLNT_Int; goto valid_num;
                case 'l': vlnt=VLNT_Long; goto valid_num;
                case 'd': vlnt=VLNT_Double; goto valid_num;
                default: return (as_number_opt_t){0};
            }
        }
        return (as_number_opt_t){0};
    }
    valid_num:
    switch(vlnt){ 
        case VLNT_Char: return (as_number_opt_t){.exists=true,.v=(as_number_t){.type=vlnt,.c=strtol(token,0,10)}};
        case VLNT_Int: return (as_number_opt_t){.exists=true,.v=(as_number_t){.type=vlnt,.i=strtol(token,0,10)}};
        case VLNT_Long: return (as_number_opt_t){.exists=true,.v=(as_number_t){.type=vlnt,.l=strtol(token,0,10)}};
        case VLNT_Double: return (as_number_opt_t){.exists=true,.v=(as_number_t){.type=vlnt,.d=strtod(token,0)}};
        default: exit(EXIT_FAILURE); return (as_number_opt_t){0};//Should be unreachable.
    }
}
void VLNumberPrintNumber(as_number_t num,unsigned decimals){
    switch(num.type){
        case VLNT_Invalid: printf("NaN"); break;
        case VLNT_Char: printf("%dc",num.c); break;
        case VLNT_Int: printf("%di",num.i); break;
        case VLNT_Long: printf("%ldl",num.l); break;
        case VLNT_Double: printf("%.*lfd",decimals,num.d); break;
    }
}
char* VLNumberGetNumberString(as_number_t num,unsigned decimals){
#define Buffer 512
    static char temp[Buffer]={0};
    int num_bytes;
    switch(num.type){
        case VLNT_Invalid: num_bytes=snprintf(temp,Buffer,"NaN"); break;
        case VLNT_Char: num_bytes=snprintf(temp,Buffer,"%dc",num.c); break;
        case VLNT_Int: num_bytes=snprintf(temp,Buffer,"%di",num.i); break;
        case VLNT_Long: num_bytes=snprintf(temp,Buffer,"%ldl",num.l); break;
        case VLNT_Double: num_bytes=snprintf(temp,Buffer,"%.*lfd",decimals,num.d); break;
    }
#undef Buffer
    char* ret=malloc(sizeof(char[num_bytes+1]));
    EXIT_IF_NULL(ret,char*);
    return strcpy(ret,temp);
}
as_number_t VLNumberCast(as_number_t num,VLNumberType type){
    #define GET_NUM ((num.type==VLNT_Double)?num.d:\
    (num.type==VLNT_Long)?num.l:\
    (num.type==VLNT_Int)?num.i:\
    (num.type==VLNT_Char)?num.c:0)
    switch(type){
        case VLNT_Double: return (as_number_t){.d=(double)GET_NUM,.type=type};
        case VLNT_Long: return (as_number_t){.l=(long)GET_NUM,.type=type};
        case VLNT_Int: return (as_number_t){.i=(int)GET_NUM,.type=type};
        case VLNT_Char: return (as_number_t){.c=(char)GET_NUM,.type=type};
        default: SHOULD_BE_UNREACHABLE(); return (as_number_t){0};
    }
    #undef GET_NUM
}
//Can return false for variable types if the variable string did not exist yet.
bool ProcessVLCallback(VariableLoader_t* vl,vlcallback_info vlc_info,as_number_t* number_io){
    const vlcallback_t* callback=VL_get_callback(vl,vlc_info);
    switch(callback->callback_type){
        case VLCallback_Double: return callback->func.as_double(number_io,callback->args.dnumber);
        case VLCallback_Long: return callback->func.as_long(number_io,callback->args.number);
        case VLCallback_Int: return callback->func.as_int(number_io,callback->args.number);
        case VLCallback_Char: return callback->func.as_char(number_io,callback->args.number);
        case VLCallback_LoadVariable: return callback->func.as_load_variable(vl,number_io,callback->args.variable);
        case VLCallback_RewriteVariable: return callback->func.as_rewrite_variable(vl,number_io,callback->args.variable);
        case VLCallback_RewriteVariableRPN: return callback->func.as_rewrite_variable_rpn(vl,callback->args.rpn.rpn_str,callback->args.rpn.variable,callback->args.rpn.see_stack);
        case VLCallback_NumberRPN: return callback->func.as_number_rpn(vl,number_io,callback->args.an_rpn.rpn_str,callback->args.an_rpn.see_stack);
    }
    SHOULD_BE_UNREACHABLE(); return false;
}
VariableLoader_t* VL_new(size_t size){
    VariableLoader_t* this=malloc(sizeof(VariableLoader_t));
    EXIT_IF_NULL(this,VariableLoader_t*);
    *this=(VariableLoader_t){.sman=StringMap_as_number_new(size),.callbacks=0,.callback_size=0,.ssm=SSManager_new()};
    return this;
}
vlcallback_info _VariableLoader_add_callback(VariableLoader_t* this,vlcallback_t* vlc){
    if(this->callback_size) this->callbacks=realloc(this->callbacks,sizeof(vlcallback_t)*(this->callback_size+1));
    else this->callbacks=malloc(sizeof(vlcallback_t));
    EXIT_IF_NULL(this->callbacks,vlcallback_t*);
    const int this_index=this->callback_size;
    this->callbacks[this_index]=*vlc;
    this->callback_size++;
    return (vlcallback_info){this_index};
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_rewrite_variable(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_RewriteVariable,
        .number_type=VLNT_Invalid,
        .func.as_rewrite_variable=_VL_callback_rewrite_variable,
        .args.variable=variable
    });
}
//Takes malloc ownership of both strings.
vlcallback_info VL_new_callback_rewrite_variable_rpn(VariableLoader_t* this,char* rpn_str,char* variable,bool see_stack){
    SSManager_add_string(this->ssm,&rpn_str);
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_RewriteVariableRPN,
        .number_type=VLNT_Invalid,
        .func.as_rewrite_variable_rpn=_VL_callback_rewrite_variable_rpn,
        .args.rpn=(vlargs_rpn_t){.rpn_str=rpn_str,.variable=variable,.see_stack=see_stack}
    });
}
//Takes malloc ownership of rpn.
vlcallback_info VL_new_callback_number_rpn(VariableLoader_t* this,char* rpn_str,bool see_stack){
    SSManager_add_string(this->ssm,&rpn_str);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_NumberRPN,
        .number_type=VLNT_Invalid,
        .func.as_number_rpn=_VL_callback_number_rpn,
        .args.an_rpn=(vlargs_an_rpn_t){
            .rpn_str=rpn_str,
            .see_stack=see_stack
        }
    });
}

vlcallback_info VL_new_callback_double(VariableLoader_t* this,double value){
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_Double,
        .number_type=VLNT_Double,
        .func.as_double=_VL_callback_double_func,
        .args.dnumber=value
    });
}
vlcallback_info VL_new_callback_long(VariableLoader_t* this,long value){
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_Long,
        .number_type=VLNT_Long,
        .func.as_long=_VL_callback_long_func,
        .args.number=value
    });
}
vlcallback_info VL_new_callback_int(VariableLoader_t* this,int value){
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_Int,
        .number_type=VLNT_Int,
        .func.as_int=_VL_callback_int_func,
        .args.number=value
    });
}
vlcallback_info VL_new_callback_char(VariableLoader_t* this,char value){
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_Char,
        .number_type=VLNT_Char,
        .func.as_char=_VL_callback_char_func,
        .args.number=value
    });
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_load_variable(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_LoadVariable,
        .number_type=VLNT_Invalid,
        .func.as_load_variable=_VL_callback_load_variable_func,
        .args.variable=variable
    });
}
vlcallback_t* VL_get_callback(const VariableLoader_t* this,vlcallback_info vlc_info){
    return this->callbacks+vlc_info.i;
}
StringMapOpt_as_number_t VL_get_as_number(const VariableLoader_t* this,const char* variable){
    return StringMap_as_number_read(this->sman,variable);
}
void VL_free(VariableLoader_t* this){
    free(this->callbacks);
    StringMap_as_number_free(this->sman);
    SSManager_free(this->ssm);
    free(this);
}
//Reedits and owns malloc variable string to get pointer of the same string.
ValueAssignE VL_add_as_double(VariableLoader_t* this,char** variable,double value){
    SSManager_add_string(this->ssm,variable);
    return StringMap_as_number_assign(this->sman,*variable,(as_number_t){.d=value,.type=VLNT_Double});
}
//Reedits and owns malloc variable string to get pointer of the same string.
ValueAssignE VL_add_as_long(VariableLoader_t* this,char** variable,long value){
    SSManager_add_string(this->ssm,variable);
    return StringMap_as_number_assign(this->sman,*variable,(as_number_t){.l=value,.type=VLNT_Long});
}
//Reedits and owns malloc variable string to get pointer of the same string.
ValueAssignE VL_add_as_int(VariableLoader_t* this,char** variable,int value){
    SSManager_add_string(this->ssm,variable);
    return StringMap_as_number_assign(this->sman,*variable,(as_number_t){.i=value,.type=VLNT_Int});
}
//Reedits and owns malloc variable string to get pointer of the same string.
ValueAssignE VL_add_as_char(VariableLoader_t* this,char** variable,char value){
    SSManager_add_string(this->ssm,variable);
    return StringMap_as_number_assign(this->sman,*variable,(as_number_t){.c=value,.type=VLNT_Char});
}
//Rewrite by casting the number to the original variable's type.
bool _VL_callback_rewrite_variable(VariableLoader_t* this,as_number_t* new_value,const char* variable){
    StringMapOpt_as_number_t old_v={0};
    #define NEW_VALUE_CAST(mem,op,ToType)\
    (ToType)((mem op type)==VLNT_Double?(mem op d):\
    (mem op type)==VLNT_Long?(mem op l):\
    (mem op type)==VLNT_Int?(mem op i):\
    (mem op type)==VLNT_Char?(mem op c):0)
    if((old_v=StringMap_as_number_read(this->sman,variable)).exists){
        switch(old_v.value.type){
            case VLNT_Char: StringMap_as_number_assign(this->sman,variable,(as_number_t){.c=NEW_VALUE_CAST(new_value,->,char),.type=old_v.value.type}); break;
            case VLNT_Int: StringMap_as_number_assign(this->sman,variable,(as_number_t){.i=NEW_VALUE_CAST(new_value,->,int),.type=old_v.value.type}); break;
            case VLNT_Long: StringMap_as_number_assign(this->sman,variable,(as_number_t){.l=NEW_VALUE_CAST(new_value,->,long),.type=old_v.value.type}); break;
            case VLNT_Double: StringMap_as_number_assign(this->sman,variable,(as_number_t){.d=NEW_VALUE_CAST(new_value,->,double),.type=old_v.value.type}); break;
            default: exit(EXIT_FAILURE); //Invalid variables.
        };
        return true;
    }
    return false;
}
#include "rpn_evaluator.h"
bool _VL_callback_rewrite_variable_rpn(VariableLoader_t* this,const char* rpn_str,const char* variable,bool see_stack){
    StringMapOpt_as_number_t old_v=StringMap_as_number_read(this->sman,variable);
    as_number_t an={0};
    if(!old_v.exists) return false;
    RPNValidStringE status=RPNEvaluatorEvaluate(rpn_str,this,&an,see_stack,true,RPN_EVAL_START_B,RPN_EVAL_END_B,RPN_EVAL_SEP);
    if(status!=RPNVS_Ok) return false;
    switch(old_v.value.type){
        case VLNT_Char: StringMap_as_number_assign(this->sman,variable,(as_number_t){.c=NEW_VALUE_CAST(an,.,char),.type=old_v.value.type}); break;
        case VLNT_Int: StringMap_as_number_assign(this->sman,variable,(as_number_t){.i=NEW_VALUE_CAST(an,.,int),.type=old_v.value.type}); break;
        case VLNT_Long: StringMap_as_number_assign(this->sman,variable,(as_number_t){.l=NEW_VALUE_CAST(an,.,long),.type=old_v.value.type}); break;
        case VLNT_Double: StringMap_as_number_assign(this->sman,variable,(as_number_t){.d=NEW_VALUE_CAST(an,.,double),.type=old_v.value.type}); break;
        default: exit(EXIT_FAILURE); //Invalid variables.
    }
    return true;
    #undef NEW_VALUE_CAST
}
//Callbacks below always return true.
bool _VL_callback_double_func(as_number_t* at_address,double value){
    *at_address=(as_number_t){.d=value,.type=VLNT_Double};
    return true;
}
bool _VL_callback_long_func(as_number_t* at_address,long value){
    *at_address=(as_number_t){.l=value,.type=VLNT_Long};
    return true;
}
bool _VL_callback_int_func(as_number_t* at_address,int value){
    *at_address=(as_number_t){.i=value,.type=VLNT_Int};
    return true;
}
bool _VL_callback_char_func(as_number_t* at_address,char value){
    *at_address=(as_number_t){.c=value,.type=VLNT_Char};
    return true;
}
bool _VL_callback_number_rpn(const VariableLoader_t* vl,as_number_t* at_address,const char* rpn_str,bool see_stack){
    return RPNEvaluatorEvaluate(rpn_str,vl,at_address,see_stack,true,RPN_EVAL_START_B,RPN_EVAL_END_B,RPN_EVAL_SEP)==RPNVS_Ok;
}
bool _VL_callback_load_variable_func(const VariableLoader_t* this,as_number_t* at_address,const char* variable){
    const StringMapOpt_as_number_t v=StringMap_as_number_read(this->sman,variable);
    if(v.exists){
        switch(v.value.type){
            case VLNT_Char: *at_address=(as_number_t){.c=v.value.c,.type=VLNT_Char}; break;
            case VLNT_Int: *at_address=(as_number_t){.i=v.value.i,.type=VLNT_Int}; break;
            case VLNT_Long: *at_address=(as_number_t){.l=v.value.l,.type=VLNT_Long}; break;
            case VLNT_Double: *at_address=(as_number_t){.d=v.value.d,.type=VLNT_Double}; break;
            default: exit(EXIT_FAILURE); //Invalid variables.
        }
        return true;
    }
    return false;
}
