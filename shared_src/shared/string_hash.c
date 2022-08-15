#include "string_hash.h"
char* _string_copy(const char* key){/*To copy string value.*/
    char* this=malloc(sizeof(char)*strlen(key)+1);
    strcpy(this,key);
    EXIT_IF_NULL(this,char*);
    return this;
}
#define StringMap_ImplDef(ValueType,VName)\
StringMap_##VName##_t* StringMap_##VName##_new(size_t size){\
    StringMap_##VName##_t* this=malloc(sizeof(StringMap_##VName##_t));\
    EXIT_IF_NULL(this,StringMap_##VName##_t);\
    memcpy(this,&(StringMap_##VName##_t){\
        .MaxSize=size,\
        .size=0,\
        .keys=calloc(size,sizeof(char*)),\
        .map_values=calloc(size,sizeof(ValueType))\
        }\
    ,sizeof(StringMap_##VName##_t));/*memcpy+compound literal, because const MaxSize*/\
    EXIT_IF_NULL(this->keys,char**);\
    EXIT_IF_NULL(this->map_values,ValueType);\
    return this;\
}\
/*To do robin hood hashing backwards shift*/\
void _StringMap_##VName##_erase(StringMap_##VName##_t* this,hash_t delete_offset_i){\
    free(this->keys[delete_offset_i]);\
    char** null_this;\
    for(size_t offset_i=1;offset_i<=this->MaxSize;offset_i++){/*<= sign to get the last key if it somehow needs to shift.*/\
        const hash_t this_i=StringMap_##VName##_Mod(this,delete_offset_i+offset_i-1),next_i=StringMap_##VName##_Mod(this,delete_offset_i+offset_i);\
        null_this=this->keys+this_i;\
        char** const next_erase_k=this->keys+next_i;\
        if(!*next_erase_k) goto null_this;/*No need to shift backwards if next string is non-existant.*/\
        if(StringMap_##VName##_Mod(this,StringMap_Hash(*next_erase_k))-next_i){/*Backwards shift if a slot's probe length is not in its intended slot (!=0)*/\
            this->map_values[this_i]=this->map_values[next_i];\
            this->keys[this_i]=this->keys[next_i];\
            continue;\
        }\
        break;/*0 for a slot*/\
    }\
    null_this:\
    *null_this=0;\
}\
/*Copies string.*/\
ValueAssignE StringMap_##VName##_assign(StringMap_##VName##_t* this,const char* key,ValueType map_value){\
    return StringMap_##VName##_assign_own(this,_string_copy(key),map_value);\
}\
/*Takes malloc ownership of string.*/\
ValueAssignE StringMap_##VName##_assign_own(StringMap_##VName##_t* this,char* key,ValueType map_value){\
    if(this->size==this->MaxSize){free(key); return false;}/*Too full. Messes with Robin Hood hashing.*/\
    char* key_to_add=key; /*Assign copied string to free later.*/\
    hash_t swap_difference=0; /*To readjust assign_slot_i (swapping causes a skip in reading HashSlots* in linear order) by the hash difference of Mod(HashF(key)) with Mod(HashF(key_to_add)).*/\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t swap_hash_i=StringMap_##VName##_Mod(this,StringMap_Hash(key_to_add));\
        const hash_t assign_i=StringMap_##VName##_Mod(this,swap_hash_i+offset_i+swap_difference);\
        char** const assign_str=this->keys+assign_i;\
        ValueType* const assign_val=this->map_values+assign_i;\
        if(!*assign_str){/*If empty.*/\
            *assign_str=key_to_add;\
            *assign_val=map_value;\
            this->size++;\
            return VA_Written;\
        }\
        if(!strcmp(*assign_str,key_to_add)){/*Same key. Reassignment.*/\
            free(key_to_add);\
            *assign_val=map_value;\
            return VA_Rewritten;\
        }\
        char* key_next_swap=*assign_str;\
        const ValueType value_next_swap=*assign_val;\
        const hash_t read_hash_i=StringMap_##VName##_Mod(this,StringMap_Hash(*assign_str));\
        if(StringMap_##VName##_SubMod(this,assign_i-swap_hash_i)>StringMap_##VName##_SubMod(this,assign_i-read_hash_i)){/*Swap key/value contents if distance from the original hash of swap_difference is greater.*/\
            *assign_str=key_to_add;\
            *assign_val=map_value;\
            key_to_add=key_next_swap; /*Insert for the next iteration.*/\
            map_value=value_next_swap;\
            swap_difference=StringMap_##VName##_Mod(this,StringMap_Hash(key))-read_hash_i;\
        }\
    }\
    free(key_to_add);/*Is full.*/\
    return VA_Full;\
}\
bool StringMap_##VName##_erase_own(StringMap_##VName##_t* this,char* key){\
    bool b=StringMap_##VName##_erase(this,key);\
    free(key);\
    return b;\
}\
bool StringMap_##VName##_erase(StringMap_##VName##_t* this,const char* key){\
    const hash_t swap_hash_i=StringMap_##VName##_Mod(this,StringMap_Hash(key));\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t assign_i=StringMap_##VName##_Mod(this,swap_hash_i+offset_i);\
        char* const delete_str=this->keys[assign_i];\
        if(delete_str&&!strcmp(delete_str,key)){\
            _StringMap_##VName##_erase(this,assign_i);\
            this->size--;\
            return true;\
        }\
    }\
    return false;\
}\
SomeMapValue_##VName##_t StringMap_##VName##_read_own(const StringMap_##VName##_t* this,char* key){\
    SomeMapValue_##VName##_t smv=StringMap_##VName##_read(this,key);\
    free(key);\
    return smv;\
}\
SomeMapValue_##VName##_t StringMap_##VName##_read(const StringMap_##VName##_t* this,const char* key){/*Bool if key found.*/\
    const hash_t search_key_hash=StringMap_##VName##_Mod(this,StringMap_Hash(key));\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const hash_t current_hash_read=StringMap_##VName##_Mod(this,search_key_hash+hash_i);\
        const char* next_key=this->keys[current_hash_read];\
        size_t next_value=this->map_values[current_hash_read];\
        if(!next_key) return (SomeMapValue_##VName##_t){0}; /*Empty key. Not there.*/\
        const hash_t current_distance=StringMap_##VName##_SubMod(this,current_hash_read-search_key_hash);\
        const hash_t next_key_distance=StringMap_##VName##_SubMod(this,current_hash_read-StringMap_##VName##_Mod(this,StringMap_Hash(next_key)));\
        /*printf("Reading at hash %ld. Read key distance: %ld Next key distance: %ld\n",current_hash_read,current_distance,next_key_distance);*/\
        if(current_distance>next_key_distance) return (SomeMapValue_##VName##_t){0};/*The key's distance greater than next. Not there.*/\
        if(!strcmp(next_key,key)){return (SomeMapValue_##VName##_t){.exists=true,.value=next_value};}\
    }\
    return (SomeMapValue_##VName##_t){0}; /*Not in entire array.*/\
}\
/*Same as _erase and _read, but returns the value as well if it exists.*/\
SomeMapValue_##VName##_t StringMap_##VName##_pop(StringMap_##VName##_t* this,const char* key){\
    const hash_t swap_hash_i=StringMap_##VName##_Mod(this,StringMap_Hash(key));\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t assign_i=StringMap_##VName##_Mod(this,swap_hash_i+offset_i);\
        char* const delete_str=this->keys[assign_i];\
        const ValueType get_value=this->map_values[assign_i];\
        if(delete_str&&!strcmp(delete_str,key)){\
            _StringMap_##VName##_erase(this,assign_i);\
            this->size--;\
            return (SomeMapValue_##VName##_t){.exists=true,.value=get_value};\
        }\
    }\
    return (SomeMapValue_##VName##_t){0};\
}\
SomeMapValue_##VName##_t StringMap_##VName##_pop_own(StringMap_##VName##_t* this,char* key){\
    SomeMapValue_##VName##_t smv=StringMap_##VName##_pop(this,key);\
    free(key);\
    return smv;\
}\
void StringMap_##VName##_print(const StringMap_##VName##_t* this){\
    puts("---Keys in hash---");\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const char* key=this->keys[hash_i];\
        if(key){\
            printf("String: \"%s\"- Hash: %lu\n",key,hash_i);\
        }\
    }\
}\
void StringMap_##VName##_print_debug(const StringMap_##VName##_t* this){\
    printf("Size:%lu [",this->size);\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const char* key=this->keys[hash_i];\
        printf("%s%lu:'%s':[%ld:%ld%s%s"\
            ,key?"\x1B[47;30m":""\
            ,hash_i\
            ,key?key:"N/A"\
            ,key?StringMap_##VName##_Mod(this,StringMap_Hash(key)):-1\
            ,key?StringMap_##VName##_SubMod(this,hash_i-StringMap_##VName##_Mod(this,StringMap_Hash(key))):-1\
            ,"]\x1B[0m"\
            ,(hash_i!=this->MaxSize-1)?", ":""\
        );\
    }/*Format: index hash:'key':[key hash:Distance of key's hash to its original hash]*/\
    puts("]");\
}\
void StringMap_##VName##_free(StringMap_##VName##_t* this){\
    for(size_t i=0;i<this->MaxSize;i++){\
        free(this->keys[i]);\
    }\
    free(this->keys);\
    free(this->map_values);\
    free(this);\
}\
bool StringMap_##VName##_resize(StringMap_##VName##_t** this,size_t new_size){\
    if(new_size<(*this)->size) return false;\
    StringMap_##VName##_t* new_table=StringMap_##VName##_new(new_size);\
    for(size_t i=0;i<(*this)->MaxSize;i++){/*Iterate all non-empty key values to put in the new table.*/\
        char* const key=(*this)->keys[i];\
        if(!key) continue;\
        const ValueType get_value=(*this)->map_values[i];\
        StringMap_##VName##_assign_own(new_table,key,get_value);\
        (*this)->keys[i]=0;/*0 for no double frees.*/\
    }\
    StringMap_##VName##_free(*this);\
    *this=new_table;\
    return true;\
}

StringMap_ImplDef(size_t,SizeT)