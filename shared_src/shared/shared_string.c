#include "shared_string.h"
#include "macros.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
shared_string_manager* SSManager_new(void){
    shared_string_manager* this=(shared_string_manager*)calloc(1,sizeof(shared_string_manager));
    EXIT_IF_NULL(this,shared_string_manager*);
    return this;
}
char* SSManager_add_string(shared_string_manager* this, char**  str_p_owned){//Returns freed pointer while it modifies into the new shared pointer.
    for(int i=0;i<this->count;i++){
        if(strcmp(*str_p_owned,this->c_strs[i])==0){
            char* freed_pointer=*str_p_owned;
            if(*str_p_owned!=this->c_strs[i]) free(*str_p_owned);//Remove duplicate char* contents from strcmp==0.
            *str_p_owned=this->c_strs[i];
            this->c_str_rc[i]++;
            return freed_pointer;//To check for uniqueness (Ex: bool is_unique=(str==SSManager_add_string(this->ssm,&str)));
        }
    }
    this->count++;
    if(this->c_strs){
        this->c_strs=(char**)realloc(this->c_strs,sizeof(char*)*this->count);
        this->c_str_rc=(int*)realloc(this->c_str_rc,sizeof(int)*this->count);
    }else{
        this->c_strs=(char**)malloc(sizeof(char*));
        this->c_str_rc=(int*)malloc(sizeof(int));
    }
    EXIT_IF_NULL(this->c_strs,char**);
    EXIT_IF_NULL(this->c_str_rc,int*);
    this->c_strs[this->count-1]=*str_p_owned;
    this->c_str_rc[this->count-1]=1;
    return *str_p_owned;
}
int SSManager_count_string(const shared_string_manager* this, const char* str_cmp){
    for(int i=0;i<this->count;i++){
        if(strcmp(str_cmp,this->c_strs[i])==0){
            return this->c_str_rc[i];
        }
    }
    return 0;//Not initialized or no string found.
}
void SSManager_print_strings(const shared_string_manager* this){
    printf("shared_strings: {");
    for(int i=0;i<this->count;i++) printf("(\"%s\",x%d)%s",this->c_strs[i],this->c_str_rc[i],i<this->count-1?", ":"");
    printf("}\n");
}
void SSManager_free_string(shared_string_manager* this, const char* str_del){
    for(int del_i=0;del_i<this->count;del_i++){
        if(strcmp(str_del,this->c_strs[del_i])==0){
            if(--this->c_str_rc[del_i]) return;//RC-1
            //Deallocate string by moving the last index to the one that will be removed.
            free(this->c_strs[del_i]);
            if(--this->count){
                this->c_strs[del_i]=this->c_strs[this->count];
                this->c_str_rc[del_i]=this->c_str_rc[this->count];
                this->c_strs=(char**)realloc(this->c_strs,sizeof(char*)*this->count);
                EXIT_IF_NULL(this->c_strs,char**);
                this->c_str_rc=(int*)realloc(this->c_str_rc,sizeof(int)*this->count);
                EXIT_IF_NULL(this->c_str_rc,int*);
                return;
            }
            free(this->c_str_rc);
            free(this->c_strs);
            this->c_str_rc=NULL;
            this->c_strs=NULL;//No double frees in SSManager_free.
            return;
        }
    }
    fprintf(stderr,"String '%s' doesn't exist in SSManager.",str_del);
    exit(EXIT_FAILURE);
}
void SSManager_free(shared_string_manager* this){
    for(int i=0;i<this->count;i++) free(this->c_strs[i]);
    free(this->c_str_rc);
    free(this->c_strs);
    free(this);
}
