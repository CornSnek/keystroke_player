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
bool macro_paster_add_name(macro_paster_t* this,const char* str_name){
    for(int i=0;i<this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            fprintf(stderr,"In macro_paster, string '%s' already exists as a macro name.\n",str_name);
            return false;
        }
    }
    this->count++;
    if(this->str_names){
        this->str_names=(char**)realloc(this->str_names,sizeof(char*)*(this->count));
        this->macro_definition=(char**)realloc(this->macro_definition,sizeof(char*)*(this->count));
        this->str_var_count=(int*)realloc(this->str_var_count,sizeof(int)*(this->count));
        this->str_vars=(char***)realloc(this->str_vars,sizeof(char**)*(this->count));
        this->str_var_values=(char***)realloc(this->str_var_values,sizeof(char**)*(this->count));
    }else{
        this->str_names=(char**)malloc(sizeof(char*));
        this->macro_definition=(char**)malloc(sizeof(char*));
        this->str_var_count=(int*)malloc(sizeof(int));
        this->str_vars=(char***)malloc(sizeof(char**));
        this->str_var_values=(char***)malloc(sizeof(char**));
    }
    EXIT_IF_NULL(this->str_names,char**)
    EXIT_IF_NULL(this->macro_definition,char**)
    EXIT_IF_NULL(this->str_var_count,int*)
    EXIT_IF_NULL(this->str_vars,char***)
    EXIT_IF_NULL(this->str_var_values,char***)
    const int add_i=this->count-1;
    this->str_names[add_i]=(char*)malloc(sizeof(char)*(strlen(str_name)+1));
    EXIT_IF_NULL(this->str_names[add_i],char*)
    strcpy(this->str_names[add_i],str_name);
    this->macro_definition[add_i]=(char*)calloc(1,sizeof(char));//Null character.
    EXIT_IF_NULL(this->macro_definition[add_i],char*)
    this->str_var_count[add_i]=0;
    this->str_vars[add_i]=0;
    this->str_var_values[add_i]=0;
    return true;
}
bool macro_paster_add_var(macro_paster_t* this,const char* str_name,const char* var_name){
    int str_name_i;
    for(int i=0;i<this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            str_name_i=i;
            goto string_exists;
        }
    }
    fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not add variable string '%s'.\n",str_name,var_name);
    return false;
    string_exists:
    for(int i=0;i<this->str_var_count[str_name_i];i++){
        if(!strcmp(this->str_vars[str_name_i][i],var_name)){
            fprintf(stderr,"In macro_paster, variable '%s' already exists for string '%s'. Did not add variable.\n",var_name,str_name);
            return false;
        }
    }
    int* const var_count=this->str_var_count+str_name_i;
    char*** const var_array=this->str_vars+str_name_i;
    char*** const var_value_array=this->str_var_values+str_name_i;
    (*var_count)++;
    if(*var_array){
        *var_array=(char**)realloc(*var_array,sizeof(char*)*(*var_count));
        *var_value_array=(char**)realloc(*var_value_array,sizeof(char*)*(*var_count));
    }else{
        *var_array=(char**)malloc(sizeof(char*));
        *var_value_array=(char**)malloc(sizeof(char*));
    }
    EXIT_IF_NULL(*var_array,char**)
    EXIT_IF_NULL(*var_value_array,char**)
    (*var_array)[*var_count-1]=(char*)malloc(sizeof(char)*(strlen(var_name)+1));
    (*var_value_array)[*var_count-1]=(char*)calloc(1,sizeof(char));//As null string.
    EXIT_IF_NULL((*var_array)[*var_count-1],char*)
    EXIT_IF_NULL((*var_value_array)[*var_count-1],char*)
    strcpy((*var_array)[*var_count-1],var_name);
    return true;
}
bool macro_paster_write_macro_def(macro_paster_t* this,const char* str_name,const char* str_value){
    int str_name_i;
    for(int i=0;i<this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            str_name_i=i;
            goto string_exists;
        }
    }
    fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not write macro string value '%s'.\n",str_name,str_value);
    return false;
    string_exists:
    this->macro_definition[str_name_i]=(char*)realloc(this->macro_definition[str_name_i],sizeof(char)*(strlen(str_value)+1));
    EXIT_IF_NULL(this->macro_definition[str_name_i],char*)
    strcpy(this->macro_definition[str_name_i],str_value);
    return true;
}
void _macro_paster_write_var_value(macro_paster_t* this,int str_name_i,int var_name_i,const char* var_value){
    this->str_var_values[str_name_i][var_name_i]=(char*)realloc(this->str_var_values[str_name_i][var_name_i],sizeof(char)*(strlen(var_value)+1));
    EXIT_IF_NULL(this->str_var_values[str_name_i][var_name_i],char*)
    strcpy(this->str_var_values[str_name_i][var_name_i],var_value);
}
bool macro_paster_write_var_by_str(macro_paster_t* this,const char* str_name,const char* var_name,const char* var_value){
    int str_name_i;
    int var_name_i;
    for(int i=0;i<this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            str_name_i=i;
            goto string_exists;
        }
    }
    fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not change variable string '%s'.\n",str_name,var_name);
    return false;
    string_exists:
    for(int i=0;i<this->str_var_count[str_name_i];i++){
        if(!strcmp(this->str_vars[str_name_i][i],var_name)){
            var_name_i=i;
            goto variable_exists;
        }
    }
    fprintf(stderr,"In macro_paster, variable '%s' does not exist for string '%s'.\n",var_name,str_name);
    return false;
    variable_exists:
    _macro_paster_write_var_value(this,str_name_i,var_name_i,var_value);
    return true;
}
//Same as above, but uses var indices.
bool macro_paster_write_var_by_ind(macro_paster_t* this,const char* str_name,int var_i,const char* var_value){
    int str_name_i;
    for(int i=0;i<this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            str_name_i=i;
            goto string_exists;
        }
    }
    fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not change variable index #%d.\n",str_name,var_i);
    return false;
    string_exists:
    if(var_i>=0&&var_i<this->str_var_count[str_name_i]){
        _macro_paster_write_var_value(this,str_name_i,var_i,var_value);
        return true;
    }else{
        fprintf(stderr,"In macro_paster, variable index #%d is out of range for string '%s'.\n",var_i,str_name);
        return false;
    }
}
bool macro_paster_process_macros(macro_paster_t* this,const char* file_str,const char* start_m,const char* end_m,const char*start_b,const char* end_b,const char* def_sep,char var_sep){
    const char* s_p,* e_p;
    int depth=first_outermost_bracket(file_str,start_m,end_m,&s_p,&e_p);
    if(depth){
        fprintf(stderr,"Macro definitions need to be separated by %s and %s.\n",start_m,end_m);
        return false;
    }
    char* macros_def=char_string_slice_no_brackets(s_p,e_p,start_m);
    const char* macros_p=macros_def;//macros_p to loop until end of macros_def
    while(true){
        depth=first_outermost_bracket(macros_p,start_b,end_b,&s_p,&e_p);
        if(!depth&&!s_p) break;
        char* full_macro_str=char_string_slice_no_brackets(s_p,e_p,start_b),* macro_vars_str,* macro_name_str=0,* macro_vars_p,* def_str;
        const char* def_p;
        int macro_name_len, def_len;
        split_at_sep(full_macro_str,def_sep,&def_p,&macro_name_len,&def_len);
        macro_vars_str=(char*)calloc(1,sizeof(char)*(macro_name_len+1));
        def_str=(char*)calloc(1,sizeof(char)*(def_len+1));
        strncpy(macro_vars_str,full_macro_str,macro_name_len);
        strncpy(def_str,def_p,def_len);
        free(full_macro_str);
        macro_vars_p=macro_vars_str;
        bool macro_name_set=false;
        for(size_t i=0,str_len=0;i<=strlen(macro_vars_str);i++){//i<=strlen to include '\0'.
            if(macro_vars_str[i]==var_sep||macro_vars_str[i]=='\0'){
                char* str=(char*)calloc(1,sizeof(char)*(str_len+1));
                bool success;
                strncpy(str,macro_vars_p,str_len);
                if(macro_name_set){
                    success=macro_paster_add_var(this,macro_name_str,str);
                    if(!success){
                        free(macro_vars_str);
                        free(str);
                        free(def_str);
                        free(macros_def);
                        return false;
                    }
                    free(str);
                }else{
                    success=macro_paster_add_name(this,str);
                    if(!success){
                        free(macro_vars_str);
                        free(str);
                        free(def_str);
                        free(macros_def);
                        return false;
                    }
                    macro_name_str=str;//Free name later.
                    macro_name_set=true;
                }
                macro_vars_p+=str_len+1;//+1 To exclude var separator.
                str_len=0;
                continue;
            }
            str_len++;
            if(!char_is_key(macro_vars_str[i])){
                fprintf(stderr,"Invalid character '%c' for macro name/variable.\n",macro_vars_str[i]);
                free(macro_vars_str);
                free(macro_name_str);
                free(def_str);
                free(macros_def);
                return false;
            }
        }
        free(macro_vars_str);
        macro_paster_write_macro_def(this,macro_name_str,def_str);
        free(macro_name_str);
        free(def_str);
        macros_p=e_p+strlen(end_b);//Get next macro to process.
    }
    free(macros_def);
    return true;
}
bool macro_paster_get_string(const macro_paster_t* this,const char* str_name,char prefix,char** output){
    int str_name_i;
    char* string_output;
    for(int i=0;i<this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            str_name_i=i;
            goto string_exists;
        }
    }
    fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not write output.\n",str_name);
    return false;
    string_exists:
    string_output=(char*)malloc(sizeof(char)*(strlen(this->macro_definition[str_name_i])+1));
    strcpy(string_output,this->macro_definition[str_name_i]);
    for(int str_var_i=0;str_var_i<this->str_var_count[str_name_i];str_var_i++){
        char* prefix_str=(char*)malloc(sizeof(char)*(strlen(this->str_vars[str_name_i][str_var_i])+2));
        EXIT_IF_NULL(prefix_str,char*)
        prefix_str[0]=prefix;
        strcpy(prefix_str+1,this->str_vars[str_name_i][str_var_i]);
        replace_str(&string_output,prefix_str,this->str_var_values[str_name_i][str_var_i]);//Replace all instances of the prefixed variable with the value.
        free(prefix_str);
    }
    *output=string_output;
    return true;
}
void macro_paster_print(const macro_paster_t* this){
    for(int str_i=0;str_i<this->count;str_i++){
        printf("Macro name: %s: ",this->str_names[str_i]);
        printf("Variables:[");
        for(int var_i=0;var_i<this->str_var_count[str_i];var_i++){
            char* const str_value=this->str_var_values[str_i][var_i];
            printf("%s=%s%s",this->str_vars[str_i][var_i],(str_value[0])?str_value:"(NULL)",(var_i!=this->str_var_count[str_i]-1)?",":"");
        }
        printf("], Macro Definition:[%s",this->macro_definition[str_i][0]?this->macro_definition[str_i]:"(NULL)");
        printf("]\n");
    }
}
void macro_paster_free(macro_paster_t* this){
    for(int str_i=0;str_i<this->count;str_i++){
        free(this->str_names[str_i]);
        free(this->macro_definition[str_i]);
        for(int var_i=0;var_i<this->str_var_count[str_i];var_i++){
            free(this->str_vars[str_i][var_i]);
            free(this->str_var_values[str_i][var_i]);
        }
        free(this->str_vars[str_i]);
        free(this->str_var_values[str_i]);
    }
    free(this->str_names);
    free(this->macro_definition);
    free(this->str_var_count);
    free(this->str_vars);
    free(this->str_var_values);
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
        if(current_char==replace[0]&&!strncmp((*strptr_owner+strptr_i),replace,replace_len)){//0 in strncmp for same string contents.
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
char* char_string_slice(const char* start_p,const char* end_p){
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
    *find_begin_p=0;//If no brackets, return null.
    *find_end_p=0;
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
    return depth;//0 == No mismatched brackets.
}
//Same as above, but for outermost brackets instead. TODO: Check.
int first_outermost_bracket(const char* search_str,const char* begin_bracket,const char* end_bracket,const char** find_begin_p,const char** find_end_p){
    *find_begin_p=0;
    *find_end_p=0;
    int depth=0;
    bool find_end_b=false;
    for(size_t str_i=0;str_i<strlen(search_str);str_i++){
        if(search_str[str_i]==begin_bracket[0]){
            if(!strncmp(search_str+str_i,begin_bracket,strlen(begin_bracket))){
                ++depth;
                if(!*find_begin_p){//Only do it once.
                    *find_begin_p=search_str+str_i;
                    find_end_b=true;
                }
                str_i+=strlen(begin_bracket)-1;
            }
        }else if(search_str[str_i]==end_bracket[0]){
            if(!strncmp(search_str+str_i,end_bracket,strlen(end_bracket))){
                if(--depth<0) return -1;
                if(find_end_b&&!depth){//Depth is 0.
                    *find_end_p=search_str+str_i;
                    find_end_b=false;//Continue the code to check depth.
                }
                str_i+=strlen(end_bracket)-1;
            }
        }
    }
    return depth;
}
//Gets string length of start (start_len), pointer after the split seperator (split_p), and the length of the second split string (end_len).
void split_at_sep(const char* search_str,const char* sep,const char** split_p,int* start_len,int* end_len){
    *start_len=*end_len=0;
    *split_p=0;
    for(size_t str_i=0;str_i<strlen(search_str);str_i++){
        if(search_str[str_i]==sep[0]&&!strncmp(search_str+str_i,sep,strlen(sep))){
            *start_len=(int)str_i;
            *split_p=search_str+str_i+strlen(sep);
            *end_len=strlen(search_str)-strlen(sep)-str_i;
            break;
        }
    }
}