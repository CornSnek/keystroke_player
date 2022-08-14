#include "string_hash.h"
#define StringMap_ImplDef(ValueType,TypeName)\
StringMap_##TypeName##_t* StringMap_##TypeName##_new(size_t size){\
    StringMap_##TypeName##_t* this_p=malloc(sizeof(StringMap_##TypeName##_t));\
    EXIT_IF_NULL(this_p,StringMap_##TypeName##_t);\
    memcpy(this_p,&(StringMap_##TypeName##_t){\
        .MaxSize=size,\
        .size=0,\
        .string_keys=calloc(size,sizeof(char*)),\
        .map_values=calloc(size,sizeof(ValueType))\
        }\
    ,sizeof(StringMap_##TypeName##_t));/*memcpy+compound literal, because const MaxSize*/\
    EXIT_IF_NULL(this_p->string_keys,char**);\
    EXIT_IF_NULL(this_p->map_values,ValueType);\
    return this_p;\
}\
/*To do robin hood hashing backwards shift*/\
void _StringMap_##TypeName##_erase(StringMap_##TypeName##_t* this,hash_t delete_offset_i){\
    free(this->string_keys[delete_offset_i]);\
    char** null_this_str;\
    for(size_t offset_i=1;offset_i<=this->MaxSize;offset_i++){/*<= sign to get the last key if it somehow needs to shift.*/\
        const hash_t this_i=StringMap_##TypeName##_Mod(this,delete_offset_i+offset_i-1),next_i=StringMap_##TypeName##_Mod(this,delete_offset_i+offset_i);\
        null_this_str=this->string_keys+this_i;\
        char** const next_erase_k=this->string_keys+next_i;\
        if(!*next_erase_k) goto null_this;/*No need to shift backwards if next string is non-existant.*/\
        if(StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(*next_erase_k))-next_i){/*Backwards shift if a slot's probe length is not in its intended slot (!=0)*/\
            this->map_values[this_i]=this->map_values[next_i];\
            this->string_keys[this_i]=this->string_keys[next_i];\
            continue;\
        }\
        break;/*0 for a slot*/\
    }\
    null_this:\
    *null_this_str=0;\
}\
char* _StringMap_##TypeName##_string_copy(const char* key){/*To copy string value.*/\
    char* this=malloc(sizeof(char)*strlen(key)+1);\
    strcpy(this,key);\
    EXIT_IF_NULL(this,char*);\
    return this;\
}\
/*Copies string.*/\
bool StringMap_##TypeName##_assign(StringMap_##TypeName##_t* this,const char* key,ValueType map_value){\
    return StringMap_##TypeName##_assign_own(this,_StringMap_##TypeName##_string_copy(key),map_value);\
}\
/*Takes malloc ownership of string.*/\
bool StringMap_##TypeName##_assign_own(StringMap_##TypeName##_t* this,char* key,ValueType map_value){\
    if(this->size==this->MaxSize){free(key); return false;}/*Too full. Messes with Robin Hood hashing.*/\
    char* key_to_add=key; /*Assign copied string to free later.*/\
    hash_t swap_difference=0; /*To readjust assign_slot_i (swapping causes a skip in reading HashSlots* in linear order) by the hash difference of Mod(HashF(key)) with Mod(HashF(key_to_add)).*/\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t swap_hash_i=StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(key_to_add));\
        const hash_t assign_i=StringMap_##TypeName##_Mod(this,swap_hash_i+offset_i+swap_difference);\
        char** const assign_str=this->string_keys+assign_i;\
        ValueType* const assign_val=this->map_values+assign_i;\
        if(!*assign_str){/*If empty.*/\
            *assign_str=key_to_add;\
            *assign_val=map_value;\
            this->size++;\
            return true;\
        }\
        if(!strcmp(*assign_str,key_to_add)){/*Same key. Reassignment.*/\
            free(key_to_add);\
            *assign_val=map_value;\
            return true;\
        }\
        char* key_next_swap=*assign_str;\
        const ValueType value_next_swap=*assign_val;\
        const hash_t read_hash_i=StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(*assign_str));\
        if(StringMap_##TypeName##_SubMod(this,assign_i-swap_hash_i)>StringMap_##TypeName##_SubMod(this,assign_i-read_hash_i)){/*Swap key/value contents if distance from the original hash of swap_difference is greater.*/\
            *assign_str=key_to_add;\
            *assign_val=map_value;\
            key_to_add=key_next_swap; /*Insert for the next iteration.*/\
            map_value=value_next_swap;\
            swap_difference=StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(key))-read_hash_i;\
        }\
    }\
    free(key_to_add);/*Is full.*/\
    return false;\
}\
bool StringMap_##TypeName##_erase_own(StringMap_##TypeName##_t* this,char* key){\
    bool b=StringMap_##TypeName##_erase(this,key);\
    free(key);\
    return b;\
}\
bool StringMap_##TypeName##_erase(StringMap_##TypeName##_t* this,const char* key){\
    const hash_t swap_hash_i=StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(key));\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t assign_i=StringMap_##TypeName##_Mod(this,swap_hash_i+offset_i);\
        char* const delete_str=this->string_keys[assign_i];\
        if(delete_str&&!strcmp(delete_str,key)){\
            _StringMap_##TypeName##_erase(this,assign_i);\
            this->size--;\
            return true;\
        }\
    }\
    return false;\
}\
bool StringMap_##TypeName##_read_own(const StringMap_##TypeName##_t* this,char* key,ValueType* map_value){\
    bool b=StringMap_##TypeName##_read(this,key,map_value);\
    free(key);\
    return b;\
}\
bool StringMap_##TypeName##_read(const StringMap_##TypeName##_t* this,const char* key,ValueType* map_value){/*Bool if key found.*/\
    const hash_t search_key_hash=StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(key));\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const hash_t current_hash_read=StringMap_##TypeName##_Mod(this,search_key_hash+hash_i);\
        const char* next_key=this->string_keys[current_hash_read];\
        size_t next_value=this->map_values[current_hash_read];\
        if(!next_key) return false; /*Empty key. Not there in robin hood hashing.*/\
        const hash_t current_distance=StringMap_##TypeName##_SubMod(this,current_hash_read-search_key_hash);\
        const hash_t next_key_distance=StringMap_##TypeName##_SubMod(this,current_hash_read-StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(next_key)));\
        /*printf("Reading at hash %ld. Read key distance: %ld Next key distance: %ld\n",current_hash_read,current_distance,next_key_distance);*/\
        if(current_distance>next_key_distance) return false;/*The key's distance greater than next. Not there in robin hood hashing.*/\
        if(!strcmp(next_key,key)){*map_value=next_value; return true;}\
    }\
    return false; /*Not in entire array.*/\
}\
void StringMap_##TypeName##_print_debug(const StringMap_##TypeName##_t* this){\
    printf("Size:%lu [",this->size);\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const char* key=this->string_keys[hash_i];\
        printf("%s%lu:'%s':[%ld:%ld%s%s"\
            ,key?"\x1B[47;30m":""\
            ,hash_i\
            ,key?key:"N/A"\
            ,key?StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(key)):-1\
            ,key?StringMap_##TypeName##_SubMod(this,hash_i-StringMap_##TypeName##_Mod(this,StringMap_##TypeName##_Hash(key))):-1\
            ,"]\x1B[0m"\
            ,(hash_i!=this->MaxSize-1)?", ":""\
        );\
    }/*Format: index hash:'key':[key hash:Distance of key's hash to its original hash]*/\
    puts("]");\
}\
void StringMap_##TypeName##_print(const StringMap_##TypeName##_t* this){\
    puts("---Keys in hash---");\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const char* key=this->string_keys[hash_i];\
        if(key){\
            printf("String: \"%s\"- Hash: %lu\n",key,hash_i);\
        }\
    }\
}\
void StringMap_##TypeName##_free(StringMap_##TypeName##_t* this){\
    for(size_t i=0;i<this->MaxSize;i++){\
        free(this->string_keys[i]);\
    }\
    free(this->string_keys);\
    free(this->map_values);\
    free(this);\
}

StringMap_ImplDef(size_t,SizeT)