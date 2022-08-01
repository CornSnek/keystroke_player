#include "shared_string.h"
#include "macros.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
shared_string_manager_t* SSManager_new(void){
    shared_string_manager_t* this=(shared_string_manager_t*)calloc(1,sizeof(shared_string_manager_t));
    EXIT_IF_NULL(this,shared_string_manager_t*);
    return this;
}
char* SSManager_add_string(shared_string_manager_t* this, char** str_p_owned){//Returns freed pointer while it modifies into the new shared pointer.
    for(int i=0;i<this->count;i++){
        if(!strcmp(*str_p_owned,this->c_strs[i])){
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
int SSManager_count_string(const shared_string_manager_t* this, const char* str_cmp){
    for(int i=0;i<this->count;i++){
        if(strcmp(str_cmp,this->c_strs[i])==0){
            return this->c_str_rc[i];
        }
    }
    return 0;//Not initialized or no string found.
}
void SSManager_print_strings(const shared_string_manager_t* this){
    printf("shared_strings: {");
    for(int i=0;i<this->count;i++) printf("(\"%s\",x%d)%s",this->c_strs[i],this->c_str_rc[i],i<this->count-1?", ":"");
    printf("}\n");
}
void SSManager_free_string(shared_string_manager_t* this, const char* str_del){
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
void SSManager_free(shared_string_manager_t* this){
    for(int i=0;i<this->count;i++) free(this->c_strs[i]);
    free(this->c_str_rc);
    free(this->c_strs);
    free(this);
}
macro_paster_t* macro_paster_new(void){
    macro_paster_t* this=(macro_paster_t*)calloc(1,sizeof(macro_paster_t));
    EXIT_IF_NULL(this,macro_paster_t)
    return this;
}
bool macro_paster_add_name(macro_paster_t* this,const char* str_name){//Bool if unique string (true), or duplicate (false).
    for(int i=0;i<this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            return false;
        }
    }
    this->count++;
    if(this->str_names){
        this->str_names=(char**)realloc(this->str_names,sizeof(char*)*(this->count));
        this->str_var_count=(int*)realloc(this->str_var_count,sizeof(int)*(this->count));
        this->str_vars=(char***)realloc(this->str_vars,sizeof(char**)*(this->count));
    }else{
        this->str_names=(char**)malloc(sizeof(char*));
        this->str_var_count=(int*)malloc(sizeof(int));
        this->str_vars=(char***)malloc(sizeof(char**));
    }
    EXIT_IF_NULL(this->str_names,char**)
    EXIT_IF_NULL(this->str_var_count,int*)
    EXIT_IF_NULL(this->str_vars,char***)
    const int add_i=this->count-1;
    this->str_names[add_i]=(char*)malloc(sizeof(char)*(strlen(str_name)+1));
    EXIT_IF_NULL(this->str_names[add_i],char*)
    this->str_var_count[add_i]=0;
    strcpy(this->str_names[add_i],str_name);
    return true;
}
bool macro_paster_add_var(macro_paster_t* this,const char* str_name,const char* var_name){//TODO
    (void)this;(void)str_name;(void)var_name;
    return false;
}
void macro_paster_free(macro_paster_t* this){
    for(int str_i=0;str_i<this->count;str_i++){
        free(this->str_names[str_i]);
        for(int var_i=0;var_i<this->str_var_count[str_i];var_i++){
            free(this->str_vars[var_i]);
        }
    }
    free(this->str_names);
    free(this->str_var_count);
    free(this->str_vars);
    free(this);
}
int trim_whitespace(char** strptr){//For null-terminated strings only, and reedit pointer to resize for trimmed strings. Returns int to get total length of the trimmed string.
    int str_i=0;
    int whitespace_count=0;
    do{
        if(char_is_whitespace((*strptr)[str_i])) whitespace_count++;
        else (*strptr)[str_i-whitespace_count]=(*strptr)[str_i];//Copy the next non-whitespace character.
    }while((*strptr)[++str_i]);
    (*strptr)[str_i-whitespace_count]='\0';//Null terminate last character and reallocate as whitespace-trimmed string.
    *strptr=(char*)realloc(*strptr,(str_i-whitespace_count+1)*sizeof(char));
    EXIT_IF_NULL(*strptr,char*);
    return str_i-whitespace_count;
}
void replace_str(char** strptr_owner, const char* replace, const char* with){//Assume all null-terminated.
    char* new_strptr=(char*)(malloc(sizeof(char)*1));
    EXIT_IF_NULL(new_strptr,char*)
    int strptr_i=0;
    int new_strptr_i=0;
    const int replace_len=strlen(replace);
    const int with_len=strlen(with);
    char current_char;
    while((current_char=(*strptr_owner)[strptr_i])){
        if(current_char==*replace&&!strncmp((*strptr_owner+strptr_i),replace,replace_len)){//0 in strncmp for same string contents.
            new_strptr=(char*)(realloc(new_strptr,sizeof(char)*(new_strptr_i+1+with_len)));
            EXIT_IF_NULL(new_strptr,char*)
            for(int with_i=0;with_i<with_len;with_i++){
                new_strptr[new_strptr_i++]=with[with_i];
            }
            strptr_i+=replace_len;
            continue;
        }
        new_strptr[new_strptr_i++]=current_char;
        strptr_i++;
        new_strptr=(char*)(realloc(new_strptr,sizeof(char)*new_strptr_i+1));
        EXIT_IF_NULL(new_strptr,char*)
    }
    new_strptr[new_strptr_i]='\0';//Null-terminate.
    free(*strptr_owner);
    *strptr_owner=new_strptr;//Change freed pointer to new pointer.
}
bool char_is_key(char c){
    return isalnum(c)||(c=='_')||(c=='+');
}
bool char_is_keystate(char c){
    return (c=='u')||(c=='d')||(c=='c')||(c=='U')||(c=='D')||(c=='C');
}
bool char_is_whitespace(char c){
    return (c==' ')||(c=='\t')||(c=='\n')||(c=='\v')||(c=='\f')||(c=='\r');
}
bool char_is_delay(char c){
    return (c=='m')||(c=='u')||(c=='s')||(c=='M')||(c=='U')||(c=='S');
}
//Returns string from start_p char to end_p char [[nodiscard]] returns pointer that needs to be freed.
char* char_string_slice(char* start_p,char* end_p){
    assert(start_p<=end_p);
    char* char_p=malloc(sizeof(char)*(end_p-start_p+2));//+1 to count characters and +1 for '\0'
    EXIT_IF_NULL(char_p,char)
    strncpy(char_p,start_p,end_p-start_p+1);
    char_p[end_p-start_p+1]='\0';
    return char_p;
}
/*
Assign pointers find_begin_p and find_end_p to the innermost bracket in search_str.
Int means + if more begin brackets, -1 if one more end bracket (terminates immediately), or 0 if brackets all match.
*/
int first_innermost_bracket(const char* search_str,const char* begin_bracket,const char* end_bracket,const char** find_begin_p,const char** find_end_p){
    int depth=0;
    int max_depth=0;
    bool find_end_b=false;
    for(size_t str_i=0;str_i<strlen(search_str);str_i++){
        if(search_str[str_i]==begin_bracket[0]){
            if(!strncmp(search_str+str_i,begin_bracket,strlen(begin_bracket))){
                if(++depth==max_depth+1){
                    max_depth++;
                    *find_begin_p=search_str+str_i;
                    find_end_b=true;
                }
                str_i+=strlen(begin_bracket)-1;//Skip characters of begin bracket string so there's no overlapping. -1 because of ++ in for loop.
            }
        }else if(search_str[str_i]==end_bracket[0]){
            if(!strncmp(search_str+str_i,end_bracket,strlen(end_bracket))){
                if(--depth<0) return -1;//Immediately terminate (-1 means not a dyck word.)
                if(find_end_b){
                    *find_end_p=search_str+str_i;
                    find_end_b=false;
                }
                str_i+=strlen(end_bracket)-1; //No overlapping for end bracket string.
            }
        }
    }
    return depth;//No mismatched brackets.
}