#include "variable_loader.h"
bool _VL_closure_double_func(double* at_address,long value);
bool _VL_closure_long_func(long* at_address,long value);
bool _VL_closure_int_func(int* at_address,long value);
bool _VL_closure_char_func(char* at_address,long value);
bool _VL_closure_vdouble_func(const VariableLoader_t* this,double* at_address,const char* variable);
bool _VL_closure_vlong_func(const VariableLoader_t* this,long* at_address,const char* variable);
bool _VL_closure_vint_func(const VariableLoader_t* this,int* at_address,const char* variable);
bool _VL_closure_vchar_func(const VariableLoader_t* this,char* at_address,const char* variable);
//Can return false for VLClosure_V types if the variable string did not exist yet.
bool ProcessVLClosure(const vloader_closure_t* closure,void* at_address){
    switch(closure->closure_type){
        case VLClosure_Double: return closure->func.as_double(at_address,closure->args.number);
        case VLClosure_Long: return closure->func.as_long(at_address,closure->args.number);
        case VLClosure_Int: return closure->func.as_int(at_address,closure->args.number);
        case VLClosure_Char: return closure->func.as_char(at_address,closure->args.number);
        case VLClosure_VDouble: return closure->func.as_vdouble(closure->args.variable.vl,at_address,closure->args.variable.str);
        case VLClosure_VLong: return closure->func.as_vlong(closure->args.variable.vl,at_address,closure->args.variable.str);
        case VLClosure_VInt: return closure->func.as_vint(closure->args.variable.vl,at_address,closure->args.variable.str);
        case VLClosure_VChar: return closure->func.as_vchar(closure->args.variable.vl,at_address,closure->args.variable.str);
        default: fprintf(stderr,"Bad closure type. Code shouldn't reach here.\n"); exit(EXIT_FAILURE); return false;
    }
}
VariableLoader_t* VariableLoader_new(size_t size){
    VariableLoader_t* this=malloc(sizeof(VariableLoader_t));
    EXIT_IF_NULL(this,VariableLoader_t*);
    *this=(VariableLoader_t){.sml=StringMap_long_new(size),.closures=0,.closure_size=0};
    return this;
}
//Code takes malloc ownership. Bool is false if full hash table, or string has already been added.
bool VariableLoader_add_as_d(VariableLoader_t* this,char* variable,double value){
    return VariableLoader_add_as_l(this,variable,(LD_u){.d=value}.l);
}
//Code takes malloc ownership. Bool is false if full hash table, or string has already been added.
bool VariableLoader_add_as_l(VariableLoader_t* this,char* variable,long value){
    return StringMap_long_assign_own(this->sml,variable,value)==VA_Written;
}
//Variable should already be added by VariableLoader_Add.
bool VariableLoader_rewrite_as_d(VariableLoader_t* this,const char* variable,double new_value){
    return VariableLoader_rewrite_as_l(this,variable,(LD_u){.d=new_value}.l);
}
bool VariableLoader_rewrite_as_l(VariableLoader_t* this,const char* variable,long new_value){
    if(StringMap_long_read(this->sml,variable).exists){
        StringMap_long_assign(this->sml,variable,new_value);
        return true;
    }
    return false;
}
vlclosure_i _VariableLoader_add_closure(VariableLoader_t* this,vloader_closure_t* vlc){
    if(this->closure_size) this->closures=realloc(this->closures,sizeof(vloader_closure_t)*(this->closure_size+1));
    else this->closures=malloc(sizeof(vloader_closure_t));
    EXIT_IF_NULL(this->closures,vloader_closure_t*);
    const int this_index=this->closure_size;
    this->closures[this_index]=*vlc;
    this->closure_size++;
    return this_index;
}
vlclosure_i VariableLoader_new_closure_double(VariableLoader_t* this,long value){
    return _VariableLoader_add_closure(this,&(vloader_closure_t){
        .closure_type=VLClosure_Double,
        .func.as_double=_VL_closure_double_func,
        .args.number=value
    });
}
vlclosure_i VariableLoader_new_closure_long(VariableLoader_t* this,long value){
    return _VariableLoader_add_closure(this,&(vloader_closure_t){
        .closure_type=VLClosure_Long,
        .func.as_long=_VL_closure_long_func,
        .args.number=value
    });
}
vlclosure_i VariableLoader_new_closure_int(VariableLoader_t* this,long value){
    return _VariableLoader_add_closure(this,&(vloader_closure_t){
        .closure_type=VLClosure_Int,
        .func.as_int=_VL_closure_int_func,
        .args.number=value
    });
}
vlclosure_i VariableLoader_new_closure_char(VariableLoader_t* this,long value){
    return _VariableLoader_add_closure(this,&(vloader_closure_t){
        .closure_type=VLClosure_Char,
        .func.as_char=_VL_closure_char_func,
        .args.number=value
    });
}
vlclosure_i VariableLoader_new_closure_vdouble(VariableLoader_t* this,const char* variable){
    return _VariableLoader_add_closure(this,&(vloader_closure_t){
        .closure_type=VLClosure_VDouble,
        .func.as_vdouble=_VL_closure_vdouble_func,
        .args.variable=(vlargs_variable_t){.str=variable,.vl=this}
    });
}
vlclosure_i VariableLoader_new_closure_vlong(VariableLoader_t* this,const char* variable){
    return _VariableLoader_add_closure(this,&(vloader_closure_t){
        .closure_type=VLClosure_VLong,
        .func.as_vlong=_VL_closure_vlong_func,
        .args.variable=(vlargs_variable_t){.str=variable,.vl=this}
    });
}
vlclosure_i VariableLoader_new_closure_vint(VariableLoader_t* this,const char* variable){
    return _VariableLoader_add_closure(this,&(vloader_closure_t){
        .closure_type=VLClosure_VInt,
        .func.as_vint=_VL_closure_vint_func,
        .args.variable=(vlargs_variable_t){.str=variable,.vl=this}
    });
}
vlclosure_i VariableLoader_new_closure_vchar(VariableLoader_t* this,const char* variable){
    return _VariableLoader_add_closure(this,&(vloader_closure_t){
        .closure_type=VLClosure_VChar,
        .func.as_vchar=_VL_closure_vchar_func,
        .args.variable=(vlargs_variable_t){.str=variable,.vl=this}
    });
}
const vloader_closure_t* VariableLoader_get_closure(const VariableLoader_t* this,vlclosure_i index){
    return this->closures+index;
}
void VariableLoader_free(VariableLoader_t* this){
    free(this->closures);
    StringMap_long_free(this->sml);
    free(this);
}
bool _VL_closure_double_func(double* at_address,long value){
    *at_address=(double)value;
    return true;
}
bool _VL_closure_long_func(long* at_address,long value){
    *at_address=value;
    return true;
}
bool _VL_closure_int_func(int* at_address,long value){
    *at_address=(int)value;
    return true;
}
bool _VL_closure_char_func(char* at_address,long value){
    *at_address=(char)value;
    return true;
}
bool _VL_closure_vdouble_func(const VariableLoader_t* this,double* at_address,const char* variable){
    const StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        *at_address=(LD_u){.l=v.value}.d;
        return true;
    }
    return false;
}
bool _VL_closure_vlong_func(const VariableLoader_t* this,long* at_address,const char* variable){
    const StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        *at_address=v.value;
        return true;
    }
    return false;
}
bool _VL_closure_vint_func(const VariableLoader_t* this,int* at_address,const char* variable){
    const StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        *at_address=(int)v.value;
        return true;
    }
    return false;
}
bool _VL_closure_vchar_func(const VariableLoader_t* this,char* at_address,const char* variable){
    const StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        *at_address=(char)v.value;
        return true;
    }
    return false;
}