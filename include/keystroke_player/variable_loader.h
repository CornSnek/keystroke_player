#ifndef _VARIABLE_LOADER_H_
#define _VARIABLE_LOADER_H_
#include "hash_map_impl.h"
typedef struct StringVariableLoader_s StringVariableLoader_t;

typedef enum _VLClosureType{
    VLClosure_Number,VLClosure_Variable
}VLClosureType;
typedef enum _VLAddressType{
    VLAddress_Int,VLAddress_Long
}VLAddressType;
typedef bool(*closure_number_f_t)(void*,VLAddressType,long);
typedef bool(*closure_variable_f_t)(const StringVariableLoader_t*,void*,VLAddressType,const char*);
typedef union vlfunction_union{
    closure_number_f_t number;
    closure_variable_f_t variable;
}vlfunction_union_t;
typedef struct vlargs_variable_s{
    char* str;
    StringVariableLoader_t* svl;
}vlargs_variable_t;
typedef union vlargs_union{
    long number;
    vlargs_variable_t variable;
}vlargs_union_t;
typedef struct vloader_closure_s{
    VLClosureType closure_type;
    vlfunction_union_t func;
    vlargs_union_t args;
    void* at_address;
    VLAddressType address_type;
}vloader_closure_t;
bool VariableLoader_process(vloader_closure_t closure);
bool VL_number_closure(void* at_address,VLAddressType address_type,long value);
static inline vloader_closure_t VL_number_closure_new(void* at_address,VLAddressType address_type,long value){
    return (vloader_closure_t){
        .closure_type=VLClosure_Number,
        .func.number=VL_number_closure,
        .args.number=value,
        .at_address=at_address,
        .address_type=address_type
    };
}
typedef struct StringVariableLoader_s{
    StringMap_long_t* sml;
}StringVariableLoader_t;
StringVariableLoader_t* StringVariableLoader_new(size_t size);
bool StringVariableLoader_add(StringVariableLoader_t* this,char* variable,long value,void* at_address,VLAddressType address_type,vloader_closure_t* closure);
bool StringVariableLoader_rewrite(StringVariableLoader_t* this,char* variable,long new_value);
bool StringVariableLoader_load(const StringVariableLoader_t* this,void* at_address,VLAddressType address_type,const char* variable);
void StringVariableLoader_free(StringVariableLoader_t* this);
#endif