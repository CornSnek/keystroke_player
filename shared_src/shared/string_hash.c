#include "string_hash.h"
#include <limits.h>
#ifndef NDEBUG
//Checks for 0 for each empty key if it does robin hood hashing.
#define _SM_CHECK_RHH_VALID\
    {\
        hash_t hash_i;\
        bool check_next_key_zero=false;\
        for(hash_i=0;hash_i<=(hash_t)this->MaxSize;hash_i++){\
            const char* const key=this->string_keys[SM_Mod(this,hash_i)];\
            printf("reading hash #%ld",SM_Mod(this,hash_i));\
            if(!key){\
                puts(", empty key. Next distance_of_key should be 0.");\
                check_next_key_zero=true;\
                continue;\
            }\
            const hash_t distance_of_key=SM_SubMod(this,hash_i-SM_Mod(this,SM_Hash(key)));\
            printf(", distance_of_key: %ld\n",distance_of_key);\
            if(!check_next_key_zero) continue;\
            if(!distance_of_key){check_next_key_zero=false; continue;}\
            puts("\x1B[31mOne key after an empty string key is not 0.\x1B[0m\n");\
            break;\
        }\
        if(hash_i==(hash_t)this->MaxSize) puts("\x1B[32mOk\x1B[0m");\
    }((void)0)
#else
#define _SM_CHECK_RHH_VALID ((void)0)
#endif
StringMap_t* SM_new(size_t size){
    StringMap_t* this_p=malloc(sizeof(StringMap_t));
    EXIT_IF_NULL(this_p,StringMap_t);
    memcpy(this_p,&(StringMap_t){
        .MaxSize=size,
        .size=0,
        .string_keys=calloc(size,sizeof(char*)),
        .map_values=calloc(size,sizeof(map_value_t))
        }
    ,sizeof(StringMap_t));//memcpy+compound literal, because const MaxSize
    EXIT_IF_NULL(this_p->string_keys,char**);
    EXIT_IF_NULL(this_p->map_values,map_value_t);
    return this_p;
}
//To do robin hood hashing backwards shift
void _SM_erase(StringMap_t* this,hash_t delete_offset_i){
    free(this->string_keys[delete_offset_i]);
    char** null_this_str;
    for(size_t offset_i=1;offset_i<=this->MaxSize;offset_i++){//<= sign to get the last key if it somehow needs to shift.
        const hash_t this_i=SM_Mod(this,delete_offset_i+offset_i-1),next_i=SM_Mod(this,delete_offset_i+offset_i);
        null_this_str=this->string_keys+this_i;
        char** const next_erase_k=this->string_keys+next_i;
        if(!*next_erase_k) goto null_this;//No need to shift backwards if next string is non-existant.
        if(SM_Mod(this,SM_Hash(*next_erase_k))-next_i){//Backwards shift if a slot's hash_offset is not in its intended slot (!=0)
            this->map_values[this_i]=this->map_values[next_i];
            this->string_keys[this_i]=this->string_keys[next_i];
            continue;
        }
        break;//0 for a slot
    }
    null_this:
    *null_this_str=0;
}
char* _SM_string_copy(const char* key){//To copy string value.
    char* this=malloc(sizeof(char)*strlen(key)+1);
    strcpy(this,key);
    EXIT_IF_NULL(this,char*);
    return this;
}
//Copies string.
bool SM_assign(StringMap_t* this,const char* key,map_value_t map_value){
    return SM_assign_own(this,_SM_string_copy(key),map_value);
}
//Takes malloc ownership of string.
bool SM_assign_own(StringMap_t* this,char* key,map_value_t map_value){
    if(this->size==this->MaxSize){free(key); return false;}//Too full. Messes with Robin Hood hashing.
    char* key_to_add=key; //Assign copied string to free later.
    hash_t swap_difference=0; //To readjust assign_slot_i (swapping causes a skip in reading HashSlots* in linear order) by the hash difference of Mod(HashF(key)) with Mod(HashF(key_to_add)).
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){
        const hash_t swap_hash_i=SM_Mod(this,SM_Hash(key_to_add));
        const hash_t assign_i=SM_Mod(this,swap_hash_i+offset_i+swap_difference);
        char** const assign_str=this->string_keys+assign_i;
        map_value_t* const assign_val=this->map_values+assign_i;
        if(!*assign_str){//If empty.
            *assign_str=key_to_add;
            *assign_val=map_value;
            this->size++;
            _SM_CHECK_RHH_VALID;
            return true;
        }
        if(!strcmp(*assign_str,key_to_add)){//Same key. Reassignment.
            free(key_to_add);
            *assign_val=map_value;
            return true;
        }
        char* key_next_swap=*assign_str;
        const map_value_t value_next_swap=*assign_val;
        const hash_t read_hash_i=SM_Mod(this,SM_Hash(*assign_str));
        if(SM_SubMod(this,assign_i-swap_hash_i)>SM_SubMod(this,assign_i-read_hash_i)){//Swap key/value contents if distance from the original hash of swap_difference is greater.
            *assign_str=key_to_add;
            *assign_val=map_value;
            key_to_add=key_next_swap; //Insert for the next iteration.
            map_value=value_next_swap;
            swap_difference=SM_Mod(this,SM_Hash(key))-read_hash_i;
        }
    }
    free(key_to_add);//Is full.
    return false;
}
bool SM_erase(StringMap_t* this,const char* key){
    const hash_t swap_hash_i=SM_Mod(this,SM_Hash(key));
    for(size_t offset_i=0;offset_i<this->MaxSize;offset_i++){
        const hash_t assign_i=SM_Mod(this,swap_hash_i+offset_i);
        char* const delete_str=this->string_keys[assign_i];
        if(delete_str&&!strcmp(delete_str,key)){
            _SM_erase(this,assign_i);
            this->size--;
            _SM_CHECK_RHH_VALID;
            return true;
        }
    }
    return false;
}
bool SM_read(StringMap_t* this,const char* key,map_value_t* value){//Bool if no key.
    const hash_t search_key_hash=SM_Mod(this,SM_Hash(key));
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){
        const hash_t current_hash_read=SM_Mod(this,search_key_hash+hash_i);
        const char* next_key=this->string_keys[current_hash_read];
        map_value_t next_value=this->map_values[current_hash_read];
        if(!next_key) return false; //Empty key. Not there in robin hood hashing.
        const hash_t current_distance=SM_SubMod(this,current_hash_read-search_key_hash);
        const hash_t next_key_distance=SM_SubMod(this,current_hash_read-SM_Mod(this,SM_Hash(next_key)));
        printf("Reading at hash %ld. Read key distance: %ld Next key distance: %ld\n",current_hash_read,current_distance,next_key_distance);
        if(current_distance>next_key_distance) return false;//The key's distance greater than next. Not there in robin hood hashing.
        if(!strcmp(next_key,key)){*value=next_value; return true;}
    }
    return false; //Not in entire array.
}
void SM_print_debug(StringMap_t* this){
    printf("Size:%lu [",this->size);
    for(size_t hash_i=0;hash_i<this->MaxSize;hash_i++){
        const char* key=this->string_keys[hash_i];
        printf("%s%lu:'%s':%lu:[%ld:%ld%s%s"
            ,key?"\x1B[47;30m":""
            ,hash_i
            ,key?key:"N/A"
            ,this->map_values[hash_i]
            ,key?SM_Mod(this,SM_Hash(key)):-1
            ,key?SM_SubMod(this,hash_i-SM_Mod(this,SM_Hash(key))):-1
            ,"]\x1B[0m"
            ,(hash_i!=this->MaxSize-1)?", ":""
        );
    }//Format: index hash:'key':value:[key hash:Distance of key's hash to its original hash]
    printf("]\n");
}
void SM_free(StringMap_t* this){
    for(size_t i=0;i<this->MaxSize;i++){
        free(this->string_keys[i]);
    }
    free(this->string_keys);
    free(this->map_values);
    free(this);
}