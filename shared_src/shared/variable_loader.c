#include "variable_loader.h"
StringMap_ImplDef(as_number_t,as_number)
bool _VL_callback_rewrite_variable(VariableLoader_t* this,as_number_t* new_value,const char* variable);
bool _VL_callback_double_func(as_number_t* at_address,double value);
bool _VL_callback_long_func(as_number_t* at_address,long value);
bool _VL_callback_int_func(as_number_t* at_address,int value);
bool _VL_callback_char_func(as_number_t* at_address,char value);
bool _VL_callback_load_variable_func(const VariableLoader_t* this,as_number_t* at_address,const char* variable);
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
    }
    fprintf(stderr,"Bad callback type. Code shouldn't reach here.\n"); exit(EXIT_FAILURE); return false;
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
//Takes string malloc ownership.
ValueAssignE VL_add_as_double(VariableLoader_t* this,char* variable,double value){
    return StringMap_as_number_assign_own(this->sman,variable,(as_number_t){.d=value,.type=VLNT_Double});
}
//Takes string malloc ownership.
ValueAssignE VL_add_as_long(VariableLoader_t* this,char* variable,long value){
    return StringMap_as_number_assign_own(this->sman,variable,(as_number_t){.l=value,.type=VLNT_Long});
}
//Takes string malloc ownership.
ValueAssignE VL_add_as_int(VariableLoader_t* this,char* variable,int value){
    return StringMap_as_number_assign_own(this->sman,variable,(as_number_t){.i=value,.type=VLNT_Int});
}
//Takes string malloc ownership.
ValueAssignE VL_add_as_char(VariableLoader_t* this,char* variable,char value){
    return StringMap_as_number_assign_own(this->sman,variable,(as_number_t){.c=value,.type=VLNT_Char});
}
//Rewrite by casting the number to the original variable's type.
bool _VL_callback_rewrite_variable(VariableLoader_t* this,as_number_t* new_value,const char* variable){
    StringMapOpt_as_number_t old_v={0};
#define NEW_VALUE_CAST(ToType)\
(ToType)(new_value->type==VLNT_Double?new_value->d:\
new_value->type==VLNT_Long?new_value->l:\
new_value->type==VLNT_Int?new_value->i:\
new_value->type==VLNT_Char?new_value->c:0)
    if((old_v=StringMap_as_number_read(this->sman,variable)).exists){
        switch(old_v.value.type){
            case VLNT_Char: StringMap_as_number_assign(this->sman,variable,(as_number_t){.c=NEW_VALUE_CAST(char),.type=old_v.value.type}); break;
            case VLNT_Int: StringMap_as_number_assign(this->sman,variable,(as_number_t){.i=NEW_VALUE_CAST(int),.type=old_v.value.type}); break;
            case VLNT_Long: StringMap_as_number_assign(this->sman,variable,(as_number_t){.l=NEW_VALUE_CAST(long),.type=old_v.value.type}); break;
            case VLNT_Double: StringMap_as_number_assign(this->sman,variable,(as_number_t){.d=NEW_VALUE_CAST(double),.type=old_v.value.type}); break;
            default: return false;//Invalid variables.
        };
        return true;
    }
    return false;
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
bool _VL_callback_load_variable_func(const VariableLoader_t* this,as_number_t* at_address,const char* variable){
    const StringMapOpt_as_number_t v=StringMap_as_number_read(this->sman,variable);
    if(v.exists){
        switch(v.value.type){
            case VLNT_Char: *at_address=(as_number_t){.c=v.value.c,.type=VLNT_Char}; break;
            case VLNT_Int: *at_address=(as_number_t){.i=v.value.i,.type=VLNT_Int}; break;
            case VLNT_Long: *at_address=(as_number_t){.l=v.value.l,.type=VLNT_Long}; break;
            case VLNT_Double: *at_address=(as_number_t){.d=v.value.d,.type=VLNT_Double}; break;
            default: return false;//Invalid variables.
        }
        return true;
    }
    return false;
}