#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_
#include "macros.h"
#include "frequency.h"
#include <string.h>
#include <stdbool.h>
typedef long hash_t;
typedef enum _ValueAssignE{
    VA_Full,VA_Written,VA_Rewritten,VA_PHCollision
}ValueAssignE;
/*To copy string value.*/
static inline char* _string_copy(const char* key){
    char* this=malloc(sizeof(char)*(strlen(key)+1));
    strcpy(this,key);
    EXIT_IF_NULL(this,char*);
    return this;
}
#define StringMap_ImplDecl(ValueType,VName,HashImpl)\
static inline hash_t StringMap_##VName##_Hash HashImpl \
typedef struct StringMap_##VName##_s{\
    const size_t MaxSize;\
    size_t size;\
    char** keys;\
    ValueType* map_values;\
}StringMap_##VName##_t;\
typedef struct StringMapOpt_##VName##_s{\
    bool exists;\
    ValueType value;\
}StringMapOpt_##VName##_t;\
static inline hash_t StringMap_##VName##_Mod(const StringMap_##VName##_t* this,hash_t num){\
    return num%(this->MaxSize);\
}\
static inline hash_t StringMap_##VName##_SubMod(const StringMap_##VName##_t* this,hash_t negative_num_maybe){\
    return (this->MaxSize+negative_num_maybe)%(this->MaxSize);\
}\
StringMap_##VName##_t* StringMap_##VName##_new(size_t size);\
ValueAssignE StringMap_##VName##_assign(StringMap_##VName##_t* this,const char* key,ValueType map_value);\
ValueAssignE StringMap_##VName##_assign_own(StringMap_##VName##_t* this,char* key,ValueType map_value);\
ValueAssignE StringMap_##VName##_assign_ph(StringMap_##VName##_t* this,const char* key,ValueType map_value);\
ValueAssignE StringMap_##VName##_assign_ph_own(StringMap_##VName##_t* this,char* key,ValueType map_value);\
bool StringMap_##VName##_erase(StringMap_##VName##_t* this,const char* key);\
bool StringMap_##VName##_erase_own(StringMap_##VName##_t* this,char* key);\
StringMapOpt_##VName##_t StringMap_##VName##_read(const StringMap_##VName##_t* this,const char* key);\
StringMapOpt_##VName##_t StringMap_##VName##_read_own(const StringMap_##VName##_t* this,char* key);\
StringMapOpt_##VName##_t StringMap_##VName##_read_ph(const StringMap_##VName##_t* this,const char* key);\
StringMapOpt_##VName##_t StringMap_##VName##_read_ph_own(const StringMap_##VName##_t* this,char* key);\
StringMapOpt_##VName##_t StringMap_##VName##_pop(StringMap_##VName##_t* this,const char* key);\
StringMapOpt_##VName##_t StringMap_##VName##_pop_own(StringMap_##VName##_t* this,char* key);\
void StringMap_##VName##_print(const StringMap_##VName##_t* this);\
void StringMap_##VName##_print_debug(const StringMap_##VName##_t* this);\
bool StringMap_##VName##_has_ph(const StringMap_##VName##_t* this);\
void StringMap_##VName##_free(StringMap_##VName##_t* this);\
bool StringMap_##VName##_resize(StringMap_##VName##_t** this,size_t new_size);

#define IntLongMap_ImplDecl(ValueType,VName)\
typedef struct IntLongMap_##VName##_s{\
    const size_t MaxSize;\
    size_t size;\
    long* keys;\
    bool* key_exists;\
    ValueType* map_values;\
}IntLongMap_##VName##_t;\
typedef struct IntLongMapOpt_##VName##_s{\
    bool exists;\
    ValueType value;\
}IntLongMapOpt_##VName##_t;\
static inline hash_t IntLongMap_##VName##_Mod(const IntLongMap_##VName##_t* this,hash_t num){\
    return num%(this->MaxSize);\
}\
static inline hash_t IntLongMap_##VName##_SubMod(const IntLongMap_##VName##_t* this,hash_t negative_num_maybe){\
    return (this->MaxSize+negative_num_maybe)%(this->MaxSize);\
}\
IntLongMap_##VName##_t* IntLongMap_##VName##_new(size_t size);\
ValueAssignE IntLongMap_##VName##_assign(IntLongMap_##VName##_t* this,long key,ValueType map_value);\
bool IntLongMap_##VName##_erase(IntLongMap_##VName##_t* this,long key);\
IntLongMapOpt_##VName##_t IntLongMap_##VName##_read(const IntLongMap_##VName##_t* this,long key);\
IntLongMapOpt_##VName##_t IntLongMap_##VName##_pop(IntLongMap_##VName##_t* this,long key);\
void IntLongMap_##VName##_print(const IntLongMap_##VName##_t* this);\
void IntLongMap_##VName##_print_debug(const IntLongMap_##VName##_t* this);\
void IntLongMap_##VName##_free(IntLongMap_##VName##_t* this);\
bool IntLongMap_##VName##_resize(IntLongMap_##VName##_t** this,size_t new_size);

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
    EXIT_IF_NULL(this->map_values,ValueType*);\
    return this;\
}\
/*To do robin hood hashing backwards shift*/\
void _StringMap_##VName##_erase(StringMap_##VName##_t* this,hash_t delete_offset_i){\
    free(this->keys[delete_offset_i]);\
    char** null_this=0;\
    for(size_t offset_i=1;offset_i<=this->MaxSize;offset_i++){/*<= sign to get the last key if it somehow needs to shift.*/\
        const hash_t this_i=StringMap_##VName##_Mod(this,delete_offset_i+offset_i-1),next_i=StringMap_##VName##_Mod(this,delete_offset_i+offset_i);\
        null_this=this->keys+this_i;\
        char** const next_erase_k=this->keys+next_i;\
        if(!*next_erase_k) goto null_this;/*No need to shift backwards if next string is non-existant.*/\
        if(StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(*next_erase_k))-next_i){/*Backwards shift if a slot's probe length is not in its intended slot (!=0)*/\
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
    if(this->size==this->MaxSize){free(key); return VA_Full;}\
    char* key_to_add=key; /*Assign copied string to free later.*/\
    hash_t swap_difference=0; /*To readjust assign_slot_i (swapping causes a skip in reading HashSlots* in linear order) by the hash difference of Mod(HashF(key)) with Mod(HashF(key_to_add)).*/\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t swap_hash_i=StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key_to_add));\
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
        const hash_t read_hash_i=StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(*assign_str));\
        if(StringMap_##VName##_SubMod(this,assign_i-swap_hash_i)>StringMap_##VName##_SubMod(this,assign_i-read_hash_i)){/*Swap key/value contents if distance from the original hash of swap_difference is greater.*/\
            *assign_str=key_to_add;\
            *assign_val=map_value;\
            key_to_add=key_next_swap; /*Insert for the next iteration.*/\
            map_value=value_next_swap;\
            swap_difference=StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key))-read_hash_i;\
        }\
    }\
    free(key_to_add);/*Is full.*/\
    return VA_Full;\
}\
ValueAssignE StringMap_##VName##_assign_ph(StringMap_##VName##_t* this,const char* key,ValueType map_value){\
    return StringMap_##VName##_assign_ph_own(this,_string_copy(key),map_value);\
}\
ValueAssignE StringMap_##VName##_assign_ph_own(StringMap_##VName##_t* this,char* key,ValueType map_value){\
    const hash_t assign_key_hash=StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key));\
    const hash_t current_hash_assign=StringMap_##VName##_Mod(this,assign_key_hash);\
    char* const assign_key=this->keys[current_hash_assign];\
    if(!assign_key){\
        this->keys[current_hash_assign]=key;\
        this->map_values[current_hash_assign]=map_value;\
        return VA_Written;\
    }\
    if(!strcmp(assign_key,key)){/*Same key. Reassignment.*/\
        free(key);\
        this->map_values[current_hash_assign]=map_value;\
        return VA_Rewritten;\
    }\
    free(key);/*If not a perfect hash.*/\
    return VA_PHCollision;\
}\
bool StringMap_##VName##_erase_own(StringMap_##VName##_t* this,char* key){\
    bool b=StringMap_##VName##_erase(this,key);\
    free(key);\
    return b;\
}\
bool StringMap_##VName##_erase(StringMap_##VName##_t* this,const char* key){\
    const hash_t swap_hash_i=StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key));\
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
StringMapOpt_##VName##_t StringMap_##VName##_read_own(const StringMap_##VName##_t* this,char* key){\
    StringMapOpt_##VName##_t smv=StringMap_##VName##_read(this,key);\
    free(key);\
    return smv;\
}\
StringMapOpt_##VName##_t StringMap_##VName##_read(const StringMap_##VName##_t* this,const char* key){/*Bool if key found.*/\
    const hash_t search_key_hash=StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key));\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const hash_t current_hash_read=StringMap_##VName##_Mod(this,search_key_hash+hash_i);\
        const char* next_key=this->keys[current_hash_read];\
        ValueType next_value=this->map_values[current_hash_read];\
        if(!next_key) return (StringMapOpt_##VName##_t){0}; /*Empty key. Not there.*/\
        const hash_t current_distance=StringMap_##VName##_SubMod(this,current_hash_read-search_key_hash);\
        const hash_t next_key_distance=StringMap_##VName##_SubMod(this,current_hash_read-StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(next_key)));\
        /*printf("Reading at hash %ld. Read key distance: %ld Next key distance: %ld\n",current_hash_read,current_distance,next_key_distance);*/\
        if(current_distance>next_key_distance) return (StringMapOpt_##VName##_t){0};/*The key's distance greater than next. Not there.*/\
        if(!strcmp(next_key,key)){return (StringMapOpt_##VName##_t){.exists=true,.value=next_value};}\
    }\
    return (StringMapOpt_##VName##_t){0}; /*Not in entire array.*/\
}\
StringMapOpt_##VName##_t StringMap_##VName##_read_ph_own(const StringMap_##VName##_t* this,char* key){\
    StringMapOpt_##VName##_t smv=StringMap_##VName##_read_ph(this,key);\
    free(key);\
    return smv;\
}\
/*Read once if assumed ph or perfect hash*/\
StringMapOpt_##VName##_t StringMap_##VName##_read_ph(const StringMap_##VName##_t* this,const char* key){\
    const hash_t search_key_hash=StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key));\
    const hash_t current_hash_read=StringMap_##VName##_Mod(this,search_key_hash);\
    if(this->keys[current_hash_read]&&!strcmp(this->keys[current_hash_read],key)) return (StringMapOpt_##VName##_t){.exists=true,.value=this->map_values[current_hash_read]};\
    return (StringMapOpt_##VName##_t){0};\
}\
/*Same as _erase and _read, but returns the value as well if it exists.*/\
StringMapOpt_##VName##_t StringMap_##VName##_pop(StringMap_##VName##_t* this,const char* key){\
    const hash_t swap_hash_i=StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key));\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t assign_i=StringMap_##VName##_Mod(this,swap_hash_i+offset_i);\
        char* const delete_str=this->keys[assign_i];\
        const ValueType get_value=this->map_values[assign_i];\
        if(delete_str&&!strcmp(delete_str,key)){\
            _StringMap_##VName##_erase(this,assign_i);\
            this->size--;\
            return (StringMapOpt_##VName##_t){.exists=true,.value=get_value};\
        }\
    }\
    return (StringMapOpt_##VName##_t){0};\
}\
StringMapOpt_##VName##_t StringMap_##VName##_pop_own(StringMap_##VName##_t* this,char* key){\
    StringMapOpt_##VName##_t smv=StringMap_##VName##_pop(this,key);\
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
    Frequency_t* f=Frequency_new();\
    hash_t highest_distance=-1;\
    printf("Size:%lu [",this->size);\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const char* key=this->keys[hash_i];\
        hash_t key_distance=key?StringMap_##VName##_SubMod(this,hash_i-StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key))):-1;\
        if(key_distance>-1) Frequency_add(f,(unsigned int)key_distance);\
        highest_distance=key_distance>highest_distance?key_distance:highest_distance;\
        printf("%s%lu:'%s':[%ld:%ld%s%s"\
            ,key?"\x1B[47;30m":""\
            ,hash_i\
            ,key?key:"N/A"\
            ,key?StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(key)):-1\
            ,key_distance\
            ,"]\x1B[0m"\
            ,(hash_i!=this->MaxSize-1)?", ":""\
        );\
    }/*Format: index hash:'key':[key hash:Distance of key's hash to its original hash]*/\
    printf("]\nhighest_distance: %ld, Frequency: ",highest_distance);\
    Frequency_print(f);\
    Frequency_free(f);\
}\
bool StringMap_##VName##_has_ph(const StringMap_##VName##_t* this){\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++)\
        if(this->keys[hash_i]&&StringMap_##VName##_SubMod(this,hash_i-StringMap_##VName##_Mod(this,StringMap_##VName##_Hash(this->keys[hash_i])))) return false;\
    return true;\
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

#define IntLongMap_ImplDef(ValueType,VName,HashImpl)\
static inline hash_t IntLongMap_##VName##_Hash HashImpl \
IntLongMap_##VName##_t* IntLongMap_##VName##_new(size_t size){\
    IntLongMap_##VName##_t* this=malloc(sizeof(IntLongMap_##VName##_t));\
    EXIT_IF_NULL(this,IntLongMap_##VName##_t);\
    memcpy(this,&(IntLongMap_##VName##_t){\
        .MaxSize=size,\
        .size=0,\
        .keys=calloc(size,sizeof(long)),\
        .key_exists=calloc(size,sizeof(bool)),\
        .map_values=calloc(size,sizeof(ValueType))\
        }\
    ,sizeof(IntLongMap_##VName##_t));/*memcpy+compound literal, because const MaxSize*/\
    EXIT_IF_NULL(this->keys,long*);\
    EXIT_IF_NULL(this->key_exists,bool*);\
    EXIT_IF_NULL(this->map_values,ValueType*);\
    return this;\
}\
/*To do robin hood hashing backwards shift*/\
void _IntLongMap_##VName##_erase(IntLongMap_##VName##_t* this,hash_t delete_offset_i){\
    bool* null_this;\
    for(size_t offset_i=1;offset_i<=this->MaxSize;offset_i++){/*<= sign to get the last key if it somehow needs to shift.*/\
        const hash_t this_i=IntLongMap_##VName##_Mod(this,delete_offset_i+offset_i-1),next_i=IntLongMap_##VName##_Mod(this,delete_offset_i+offset_i);\
        null_this=this->key_exists+this_i;\
        bool* const next_erase_kc=this->key_exists+next_i;\
        if(!*next_erase_kc) goto null_this;/*No need to shift backwards if next key is non-existant.*/\
        if(IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(this->keys[next_i]))-next_i){/*Backwards shift if a slot's probe length is not in its intended slot (!=0)*/\
            this->map_values[this_i]=this->map_values[next_i];\
            this->keys[this_i]=this->keys[next_i];\
            continue;\
        }\
        break;/*0 for a slot*/\
    }\
    null_this:\
    *null_this=false;\
}\
ValueAssignE IntLongMap_##VName##_assign(IntLongMap_##VName##_t* this,long key,ValueType map_value){\
    if(this->size==this->MaxSize) return VA_Full;\
    long key_to_add=key;\
    hash_t swap_difference=0; /*To readjust assign_slot_i (swapping causes a skip in reading HashSlots* in linear order) by the hash difference of Mod(HashF(key)) with Mod(HashF(key_to_add)).*/\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t swap_hash_i=IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(key_to_add));\
        const hash_t assign_i=IntLongMap_##VName##_Mod(this,swap_hash_i+offset_i+swap_difference);\
        long* const assign_long=this->keys+assign_i;\
        ValueType* const assign_val=this->map_values+assign_i;\
        if(!this->key_exists[assign_i]){/*If empty.*/\
            *assign_long=key_to_add;\
            *assign_val=map_value;\
            this->key_exists[assign_i]=true;\
            this->size++;\
            return VA_Written;\
        }\
        if(*assign_long==key_to_add){/*Same key. Reassignment.*/\
            *assign_val=map_value;\
            return VA_Rewritten;\
        }\
        long key_next_swap=*assign_long;\
        const ValueType value_next_swap=*assign_val;\
        const hash_t read_hash_i=IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(*assign_long));\
        if(IntLongMap_##VName##_SubMod(this,assign_i-swap_hash_i)>IntLongMap_##VName##_SubMod(this,assign_i-read_hash_i)){/*Swap key/value contents if distance from the original hash of swap_difference is greater.*/\
            *assign_long=key_to_add;\
            *assign_val=map_value;\
            key_to_add=key_next_swap; /*Insert for the next iteration.*/\
            map_value=value_next_swap;\
            swap_difference=IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(key))-read_hash_i;\
        }\
    }\
    return VA_Full;\
}\
bool IntLongMap_##VName##_erase(IntLongMap_##VName##_t* this,long key){\
    const hash_t swap_hash_i=IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(key));\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t assign_i=IntLongMap_##VName##_Mod(this,swap_hash_i+offset_i);\
        long const delete_long=this->keys[assign_i];\
        if(this->key_exists[assign_i]&&delete_long==key){\
            _IntLongMap_##VName##_erase(this,assign_i);\
            this->size--;\
            return true;\
        }\
    }\
    return false;\
}\
IntLongMapOpt_##VName##_t IntLongMap_##VName##_read(const IntLongMap_##VName##_t* this,long key){/*Bool if key found.*/\
    const hash_t search_key_hash=IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(key));\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        const hash_t current_hash_read=IntLongMap_##VName##_Mod(this,search_key_hash+hash_i);\
        const long next_key=this->keys[current_hash_read];\
        ValueType next_value=this->map_values[current_hash_read];\
        if(!this->key_exists[current_hash_read]) return (IntLongMapOpt_##VName##_t){0}; /*Empty key. Not there.*/\
        const hash_t current_distance=IntLongMap_##VName##_SubMod(this,current_hash_read-search_key_hash);\
        const hash_t next_key_distance=IntLongMap_##VName##_SubMod(this,current_hash_read-IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(next_key)));\
        /*printf("Reading at hash %ld. Read key distance: %ld Next key distance: %ld\n",current_hash_read,current_distance,next_key_distance);*/\
        if(current_distance>next_key_distance) return (IntLongMapOpt_##VName##_t){0};/*The key's distance greater than next. Not there.*/\
        if(next_key==key){return (IntLongMapOpt_##VName##_t){.exists=true,.value=next_value};}\
    }\
    return (IntLongMapOpt_##VName##_t){0}; /*Not in entire array.*/\
}\
/*Same as _erase and _read, but returns the value as well if it exists.*/\
IntLongMapOpt_##VName##_t IntLongMap_##VName##_pop(IntLongMap_##VName##_t* this,long key){\
    const hash_t swap_hash_i=IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(key));\
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){\
        const hash_t assign_i=IntLongMap_##VName##_Mod(this,swap_hash_i+offset_i);\
        const long delete_long=this->keys[assign_i];\
        const ValueType get_value=this->map_values[assign_i];\
        if(this->key_exists[assign_i]&&delete_long==key){\
            _IntLongMap_##VName##_erase(this,assign_i);\
            this->size--;\
            return (IntLongMapOpt_##VName##_t){.exists=true,.value=get_value};\
        }\
    }\
    return (IntLongMapOpt_##VName##_t){0};\
}\
void IntLongMap_##VName##_print(const IntLongMap_##VName##_t* this){\
    puts("---Keys in hash---");\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        long key=this->keys[hash_i];\
        if(this->key_exists[hash_i]){\
            printf("Int: \"%ld\"- Hash: %lu\n",key,hash_i);\
        }\
    }\
}\
void IntLongMap_##VName##_print_debug(const IntLongMap_##VName##_t* this){\
    printf("Size:%lu [",this->size);\
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){\
        long int key=this->keys[hash_i];\
        bool key_exists=this->key_exists[hash_i];\
        printf("%s%lu:'%ld':[%ld:%ld%s%s"\
            ,key_exists?"\x1B[47;30m":""\
            ,hash_i\
            ,key\
            ,key_exists?IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(key)):-1\
            ,key_exists?IntLongMap_##VName##_SubMod(this,hash_i-IntLongMap_##VName##_Mod(this,IntLongMap_##VName##_Hash(key))):-1\
            ,"]\x1B[0m"\
            ,(hash_i!=this->MaxSize-1)?", ":""\
        );\
    }/*Format: index hash:'key':[key hash:Distance of key's hash to its original hash]*/\
    puts("]");\
}\
void IntLongMap_##VName##_free(IntLongMap_##VName##_t* this){\
    free(this->key_exists);\
    free(this->keys);\
    free(this->map_values);\
    free(this);\
}\
bool IntLongMap_##VName##_resize(IntLongMap_##VName##_t** this,size_t new_size){\
    if(new_size<(*this)->size) return false;\
    IntLongMap_##VName##_t* new_table=IntLongMap_##VName##_new(new_size);\
    for(size_t i=0;i<(*this)->MaxSize;i++){/*Iterate all non-empty key values to put in the new table.*/\
        const long key=(*this)->keys[i];\
        if(!(*this)->key_exists[i]) continue;\
        const ValueType get_value=(*this)->map_values[i];\
        IntLongMap_##VName##_assign(new_table,key,get_value);\
    }\
    IntLongMap_##VName##_free(*this);\
    *this=new_table;\
    return true;\
}
#endif
