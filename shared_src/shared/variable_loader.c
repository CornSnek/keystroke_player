#include "variable_loader.h"
bool _VL_callback_add_as_d(VariableLoader_t* this,const char* variable,double value);
bool _VL_callback_add_as_l(VariableLoader_t* this,const char* variable,long value);
bool _VL_callback_rewrite_as_d(VariableLoader_t* this,const char* variable,double new_value);
bool _VL_callback_rewrite_as_l(VariableLoader_t* this,const char* variable,long new_value);
bool _VL_callback_double_func(double* at_address,double value);
bool _VL_callback_long_func(long* at_address,long value);
bool _VL_callback_int_func(int* at_address,int value);
bool _VL_callback_char_func(char* at_address,char value);
bool _VL_callback_vdouble_func(const VariableLoader_t* this,double* at_address,const char* variable);
bool _VL_callback_vlong_func(const VariableLoader_t* this,long* at_address,const char* variable);
bool _VL_callback_vint_func(const VariableLoader_t* this,int* at_address,const char* variable);
bool _VL_callback_vchar_func(const VariableLoader_t* this,char* at_address,const char* variable);
//Can return false for variable types if the variable string did not exist yet.
bool ProcessVLCallback(VariableLoader_t* vl,vlcallback_info vlc_info,void* at_address){
    const vlcallback_t* callback=VL_get_callback(vl,vlc_info);
    switch(callback->callback_type){
        case VLCallback_Double: return callback->func.as_double(at_address,callback->args.dnumber);
        case VLCallback_Long: return callback->func.as_long(at_address,callback->args.number);
        case VLCallback_Int: return callback->func.as_int(at_address,callback->args.number);
        case VLCallback_Char: return callback->func.as_char(at_address,callback->args.number);
        case VLCallback_VDouble: return callback->func.as_vdouble(vl,at_address,callback->args.variable);
        case VLCallback_VLong: return callback->func.as_vlong(vl,at_address,callback->args.variable);
        case VLCallback_VInt: return callback->func.as_vint(vl,at_address,callback->args.variable);
        case VLCallback_VChar: return callback->func.as_vchar(vl,at_address,callback->args.variable);
        case VLCallback_RewriteAsLong://Fallthrough (Same function type)
        case VLCallback_AddAsLong: return callback->func.as_add_as_l(vl,callback->args.variable,*((long*)at_address));
        case VLCallback_RewriteAsDouble://Fallthrough (Same function type)
        case VLCallback_AddAsDouble: return callback->func.as_add_as_d(vl,callback->args.variable,*((double*)at_address));
    }
    fprintf(stderr,"Bad callback type. Code shouldn't reach here.\n"); exit(EXIT_FAILURE); return false;
}
VariableLoader_t* VL_new(size_t size){
    VariableLoader_t* this=malloc(sizeof(VariableLoader_t));
    EXIT_IF_NULL(this,VariableLoader_t*);
    *this=(VariableLoader_t){.sml=StringMap_long_new(size),.callbacks=0,.callback_size=0,.ssm=SSManager_new()};
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
vlcallback_info VL_new_callback_add_as_l(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_AddAsLong,
        .subtype=VLCallbackST_Long,
        .func.as_add_as_l=_VL_callback_add_as_l,
        .args.variable=variable
    });
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_add_as_d(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_AddAsDouble,
        .subtype=VLCallbackST_Double,
        .func.as_add_as_d=_VL_callback_add_as_d,
        .args.variable=variable
    });
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_rewrite_as_l(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_RewriteAsLong,
        .subtype=VLCallbackST_Long,
        .func.as_add_as_l=_VL_callback_rewrite_as_l,
        .args.variable=variable
    });
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_rewrite_as_d(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_RewriteAsDouble,
        .subtype=VLCallbackST_Double,
        .func.as_add_as_d=_VL_callback_rewrite_as_d,
        .args.variable=variable
    });
}
vlcallback_info VL_new_callback_double(VariableLoader_t* this,double value){
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_Double,
        .subtype=VLCallbackST_Double,
        .func.as_double=_VL_callback_double_func,
        .args.dnumber=value
    });
}
vlcallback_info VL_new_callback_long(VariableLoader_t* this,long value){
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_Long,
        .subtype=VLCallbackST_Long,
        .func.as_long=_VL_callback_long_func,
        .args.number=value
    });
}
vlcallback_info VL_new_callback_int(VariableLoader_t* this,int value){
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_Int,
        .subtype=VLCallbackST_Int,
        .func.as_int=_VL_callback_int_func,
        .args.number=value
    });
}
vlcallback_info VL_new_callback_char(VariableLoader_t* this,char value){
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_Char,
        .subtype=VLCallbackST_Char,
        .func.as_char=_VL_callback_char_func,
        .args.number=value
    });
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_vdouble(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_VDouble,
        .subtype=VLCallbackST_Double,
        .func.as_vdouble=_VL_callback_vdouble_func,
        .args.variable=variable
    });
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_vlong(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_VLong,
        .subtype=VLCallbackST_Long,
        .func.as_vlong=_VL_callback_vlong_func,
        .args.variable=variable
    });
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_vint(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_VInt,
        .subtype=VLCallbackST_Int,
        .func.as_vint=_VL_callback_vint_func,
        .args.variable=variable
    });
}
//Takes malloc ownership of pointer.
vlcallback_info VL_new_callback_vchar(VariableLoader_t* this,char* variable){
    SSManager_add_string(this->ssm,&variable);
    return _VariableLoader_add_callback(this,&(vlcallback_t){
        .callback_type=VLCallback_VChar,
        .subtype=VLCallbackST_Char,
        .func.as_vchar=_VL_callback_vchar_func,
        .args.variable=variable
    });
}
vlcallback_t* VL_get_callback(const VariableLoader_t* this,vlcallback_info vlc_info){
    return this->callbacks+vlc_info.i;
}
void VL_free(VariableLoader_t* this){
    free(this->callbacks);
    StringMap_long_free(this->sml);
    SSManager_free(this->ssm);
    free(this);
}
//Bool is false if full hash table, or string has already been added.
bool _VL_callback_add_as_d(VariableLoader_t* this,const char* variable,double value){
    return _VL_callback_add_as_l(this,variable,(LD_u){.d=value}.l);
}
//Bool is false if full hash table, or string has already been added.
bool _VL_callback_add_as_l(VariableLoader_t* this,const char* variable,long value){
    return StringMap_long_assign(this->sml,variable,value)==VA_Written;
}
//Variable should already be added by VariableLoader_add_*.
bool _VL_callback_rewrite_as_d(VariableLoader_t* this,const char* variable,double new_value){
    return _VL_callback_rewrite_as_l(this,variable,(LD_u){.d=new_value}.l);
}
//Variable should already be added by VariableLoader_add_*.
bool _VL_callback_rewrite_as_l(VariableLoader_t* this,const char* variable,long new_value){
    if(StringMap_long_read(this->sml,variable).exists){
        StringMap_long_assign(this->sml,variable,new_value);
        return true;
    }
    return false;
}
//Functions below always return true.
bool _VL_callback_double_func(double* at_address,double value){
    *at_address=value;
    return true;
}
bool _VL_callback_long_func(long* at_address,long value){
    *at_address=value;
    return true;
}
bool _VL_callback_int_func(int* at_address,int value){
    *at_address=value;
    return true;
}
bool _VL_callback_char_func(char* at_address,char value){
    *at_address=value;
    return true;
}
bool _VL_callback_vdouble_func(const VariableLoader_t* this,double* at_address,const char* variable){
    const StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        *at_address=(LD_u){.l=v.value}.d;
        return true;
    }
    return false;
}
bool _VL_callback_vlong_func(const VariableLoader_t* this,long* at_address,const char* variable){
    const StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        *at_address=v.value;
        return true;
    }
    return false;
}
bool _VL_callback_vint_func(const VariableLoader_t* this,int* at_address,const char* variable){
    const StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        *at_address=(int)v.value;
        return true;
    }
    return false;
}
bool _VL_callback_vchar_func(const VariableLoader_t* this,char* at_address,const char* variable){
    const StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        *at_address=(char)v.value;
        return true;
    }
    return false;
}