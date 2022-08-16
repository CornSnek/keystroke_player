#include "variable_loader.h"
//Can return false in VLClosure_Variable if the variable string did not exist yet.
bool VariableLoader_process(vloader_closure_t closure){
    switch(closure.closure_type){
        case VLClosure_Number: return closure.func.number(closure.at_address,closure.address_type,closure.args.number);
        case VLClosure_Variable: return closure.func.variable(closure.args.variable.svl,closure.at_address,closure.address_type,closure.args.variable.str);
        default: fprintf(stderr,"Bad closure type. Code shouldn't reach here.\n"); exit(EXIT_FAILURE); return false;
    }
}
//Closure of closure_number_f_t
bool VL_number_closure(void* at_address,VLAddressType address_type,long value){
    switch(address_type){
        case VLAddress_Int: *(int*)at_address=value; return true;
        case VLAddress_Long: *(long*)at_address=value; return true;
        default: fprintf(stderr,"Bad address type. Code shouldn't reach here.\n"); exit(EXIT_FAILURE); return false;
    }
}
StringVariableLoader_t* StringVariableLoader_new(size_t size){
    StringVariableLoader_t* this=malloc(sizeof(StringVariableLoader_t));
    EXIT_IF_NULL(this,StringVariableLoader_t*);
    *this=(StringVariableLoader_t){.sml=StringMap_long_new(size)};
    return this;
}
//Malloc strings only. bool is false if full or string has already been added.
bool StringVariableLoader_add(StringVariableLoader_t* this,char* variable,long value,void* at_address,VLAddressType address_type,vloader_closure_t* closure){
    ValueAssignE status=StringMap_long_assign_own(this->sml,variable,value);
    if(status==VA_Written){//Shouldn't rewrite any values or be full.
        *closure=(vloader_closure_t){
            .closure_type=VLClosure_Variable,
            .func.variable=StringVariableLoader_load,
            .args.variable=(vlargs_variable_t){.str=variable,.svl=this},
            .at_address=at_address,
            .address_type=address_type
        };
        return true;
    }else{
        *closure=(vloader_closure_t){0};
        return false;
    }
}
//Variable should be added by StringVariableLoader_Add.
bool StringVariableLoader_rewrite(StringVariableLoader_t* this,char* variable,long new_value){
    return StringMap_long_assign(this->sml,variable,new_value)==VA_Rewritten;
}
//Closure of closure_variable_f_t
bool StringVariableLoader_load(const StringVariableLoader_t* this,void* at_address,VLAddressType address_type,const char* variable){
    StringMapValue_long_t v=StringMap_long_read(this->sml,variable);
    if(v.exists){
        switch(address_type){
            case VLAddress_Int:
                *(int*)at_address=v.value;
                break;
            case VLAddress_Long:
                *(long*)at_address=v.value;
                break;
        }
        return true;
    }else return false;
}
void StringVariableLoader_free(StringVariableLoader_t* this){
    StringMap_long_free(this->sml);
    free(this);
}