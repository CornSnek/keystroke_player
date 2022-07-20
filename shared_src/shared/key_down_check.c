#include <key_down_check.h>

key_down_check_t* key_down_check_new(void){
    key_down_check_t* this=(key_down_check_t*)(malloc(sizeof(key_down_check_t)));
    EXIT_IF_NULL(this,key_down_check_t*)
    this->keys=malloc(sizeof(const char*)*1);//No need to malloc if 0. Just realloc.
    EXIT_IF_NULL(this->keys,const char*)
    this->len=0;
    return this;
}
bool key_down_check_add(key_down_check_t* this,const char* add_key){
    for(int i=0;i<this->len;i++){
        if(!strcmp(this->keys[i],add_key)) return false;//Don't add duplicate keys.
    }
    this->keys[this->len++]=add_key;
    this->keys=(const char**)(realloc(this->keys,sizeof(const char*)*(this->len+1)));
    EXIT_IF_NULL(this->keys,const char*)
    return true;
}
bool key_down_check_remove(key_down_check_t* this,const char* rem_key){
    for(int i=0;i<this->len;i++){
        if(!strcmp(this->keys[i],rem_key)){//If found, just move the last key in the array where the deleted_key is, and NULL last.
            this->keys[i]=this->keys[this->len-1];
            this->keys[(this->len--)-1]=NULL;
            this->keys=(const char**)(realloc(this->keys,sizeof(const char*)*(this->len+1)));
            EXIT_IF_NULL(this->keys,const char*)
            return true;
        }
    }
    return false;
}
void key_down_check_print(key_down_check_t* this){
    for(int i=0;i<this->len;i++){
        printf("%s,",this->keys[i]);
    }
    printf("\n");
}
void key_down_check_key_up(key_down_check_t* this,xdo_t* xdo_obj,Window window){
    for(int i=0;i<this->len;i++){
        xdo_send_keysequence_window_up(xdo_obj,window,this->keys[i],0);
    }
}
void key_down_check_free(key_down_check_t* this){
    free(this->keys);
    free(this);
}