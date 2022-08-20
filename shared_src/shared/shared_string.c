#include "shared_string.h"
#include "macros.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
shared_string_manager_t* SSManager_new(void){
    shared_string_manager_t* this=calloc(1,sizeof(shared_string_manager_t));
    EXIT_IF_NULL(this,shared_string_manager_t*);
    return this;
}
char* SSManager_add_string(shared_string_manager_t* this, char** str_p_owned){//Returns freed pointer while it modifies into shared string pointer.
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
        this->c_str_rc=realloc(this->c_str_rc,sizeof(int)*this->count);
    }else{
        this->c_strs=(char**)malloc(sizeof(char*));
        this->c_str_rc=malloc(sizeof(int));
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
    puts("}");
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
                this->c_str_rc=realloc(this->c_str_rc,sizeof(int)*this->count);
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
    fprintf(stderr,"String '%s' doesn't exist in SSManager. Program shouldn't execute here.",str_del);
    exit(EXIT_FAILURE);
}
void SSManager_free(shared_string_manager_t* this){
    for(int i=0;i<this->count;i++) free(this->c_strs[i]);
    free(this->c_str_rc);
    free(this->c_strs);
    free(this);
}
macro_paster_t* macro_paster_new(void){
    macro_paster_t* this=calloc(1,sizeof(macro_paster_t));
    EXIT_IF_NULL(this,macro_paster_t);
    return this;
}
bool macro_paster_add_name(macro_paster_t* this,const char* str_name){
    if(!str_name[0]){
        fprintf(stderr,"Macro name shouldn't be empty.\n");
        return false;
    }
    for(size_t i=0;i<this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            fprintf(stderr,"In macro_paster, string '%s' already exists as a macro name.\n",str_name);
            return false;
        }
    }
    this->count++;
    if(this->str_names){
        this->str_names=(char**)realloc(this->str_names,sizeof(char*)*(this->count));
        this->macro_definition=(char**)realloc(this->macro_definition,sizeof(char*)*(this->count));
        this->str_var_count=realloc(this->str_var_count,sizeof(int)*(this->count));
        this->str_vars=(char***)realloc(this->str_vars,sizeof(char**)*(this->count));
        this->str_var_values=(char***)realloc(this->str_var_values,sizeof(char**)*(this->count));
    }else{
        this->str_names=(char**)malloc(sizeof(char*));
        this->macro_definition=(char**)malloc(sizeof(char*));
        this->str_var_count=malloc(sizeof(int));
        this->str_vars=(char***)malloc(sizeof(char**));
        this->str_var_values=(char***)malloc(sizeof(char**));
    }
    EXIT_IF_NULL(this->str_names,char**);
    EXIT_IF_NULL(this->macro_definition,char**);
    EXIT_IF_NULL(this->str_var_count,int*);
    EXIT_IF_NULL(this->str_vars,char***);
    EXIT_IF_NULL(this->str_var_values,char***);
    const int add_i=this->count-1;
    this->str_names[add_i]=malloc(sizeof(char)*(strlen(str_name)+1));
    EXIT_IF_NULL(this->str_names[add_i],char*);
    strcpy(this->str_names[add_i],str_name);
    this->macro_definition[add_i]=calloc(1,sizeof(char));//Null character.
    EXIT_IF_NULL(this->macro_definition[add_i],char*);
    this->str_var_count[add_i]=0;
    this->str_vars[add_i]=0;
    this->str_var_values[add_i]=0;
    return true;
}
bool _macro_paster_valid_name(const macro_paster_t* this,const char* str_name,size_t* at_index){
    for(size_t i=0;i<(size_t)this->count;i++){
        if(!strcmp(this->str_names[i],str_name)){
            *at_index=i;
            return true;
        }
    }
    return false;
}
bool macro_paster_add_var(macro_paster_t* this,const char* str_name,const char* var_name){
    if(!var_name[0]){
        fprintf(stderr,"One of the variable names in a macro shouldn't be empty.\n");
        return false;
    }
    size_t str_name_i;
    if(!_macro_paster_valid_name(this,str_name,&str_name_i)){
        fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not add variable string '%s'.\n",str_name,var_name);
        return false;
    }
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
    EXIT_IF_NULL(*var_array,char**);
    EXIT_IF_NULL(*var_value_array,char**);
    (*var_array)[*var_count-1]=malloc(sizeof(char)*(strlen(var_name)+1));
    (*var_value_array)[*var_count-1]=calloc(1,sizeof(char));//As null string.
    EXIT_IF_NULL((*var_array)[*var_count-1],char*);
    EXIT_IF_NULL((*var_value_array)[*var_count-1],char*);
    strcpy((*var_array)[*var_count-1],var_name);
    return true;
}
bool macro_paster_write_macro_def(macro_paster_t* this,const char* str_name,const char* str_value){
    size_t str_name_i;
    if(!_macro_paster_valid_name(this,str_name,&str_name_i)){
        fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not write macro string value '%s'.\n",str_name,str_value);
        return false;
    }
    this->macro_definition[str_name_i]=realloc(this->macro_definition[str_name_i],sizeof(char)*(strlen(str_value)+1));
    EXIT_IF_NULL(this->macro_definition[str_name_i],char*);
    strcpy(this->macro_definition[str_name_i],str_value);
    return true;
}
void _macro_paster_write_var_value(macro_paster_t* this,int str_name_i,int var_name_i,const char* var_value){
    this->str_var_values[str_name_i][var_name_i]=realloc(this->str_var_values[str_name_i][var_name_i],sizeof(char)*(strlen(var_value)+1));
    EXIT_IF_NULL(this->str_var_values[str_name_i][var_name_i],char*);
    strcpy(this->str_var_values[str_name_i][var_name_i],var_value);
}
bool macro_paster_write_var_by_str(macro_paster_t* this,const char* str_name,const char* var_name,const char* var_value){
    size_t str_name_i;
    int var_name_i;
    if(!_macro_paster_valid_name(this,str_name,&str_name_i)){
        fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not change variable string '%s'.\n",str_name,var_name);
        return false;
    }
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
    size_t str_name_i;
    if(!_macro_paster_valid_name(this,str_name,&str_name_i)){
        fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not change variable index #%d.\n",str_name,var_i);
        return false;
    }
    if(var_i>=0&&var_i<this->str_var_count[str_name_i]){
        _macro_paster_write_var_value(this,str_name_i,var_i,var_value);
        return true;
    }else{
        fprintf(stderr,"In macro_paster, variable index #%d is out of range for string '%s'. Variables may not exist.\n",var_i,str_name);
        return false;
    }
}
MacroProcessStatus file_contains_macro_definitions(const char* file_str,const char* start_m,const char* end_m){
    const char* file_str_p=file_str;
    bool has_begin_bracket=false;
    bool has_end_bracket=false;
    while((file_str_p=strchr(file_str_p,start_m[0]))){
        if(!strncmp(file_str_p++,start_m,strlen(start_m))){
            has_begin_bracket=true;
            break;
        }
    }
    if(!has_begin_bracket){
        puts("File does not contain macro definitions. Using this file to process commands.");
        return MPS_NoMacros;
    }
    while((file_str_p=strchr(file_str_p,end_m[0]))){
        if(!strncmp(file_str_p++,end_m,strlen(end_m))){
            has_end_bracket=true;
            break;
        }
    }
    if(!has_end_bracket){
        fprintf(stderr,"Macro definitions processing error: End bracket '%s' missing.\n",end_m);
        return MPS_ImproperBrackets;
    }
    while((file_str_p=strchr(file_str_p,start_m[0]))){
        if(!strncmp(file_str_p++,start_m,strlen(start_m))){
            fprintf(stderr,"Macro definitions processing error: There should only be one set of '%s' and '%s' to contain the macro definitions.\n",start_m,end_m);
            return MPS_ImproperBrackets;
        }
    }
    puts("This file contains macro definitions. Expanding macros to file "MACRO_PROCESS_F" and processing its commands.");
    return MPS_HasDefinitions;
}
bool macro_paster_process_macros(macro_paster_t* this,const char* file_str,const char* start_m,const char* end_m,const char*start_b,const char* end_b,const char* def_sep,char var_sep){
    const char* s_p,* e_p;
    int depth=first_outermost_bracket(file_str,start_m,end_m,&s_p,&e_p);
    if(depth){
        fprintf(stderr,"Macro definitions need to be separated by %s and %s at the beginning of the file.\n",start_m,end_m);
        return false;
    }
    char* macros_def=char_string_slice_no_brackets(s_p,e_p,start_m);
    const char* macros_p=macros_def;//macros_p to loop until end of macros_def
    while(true){
        depth=first_outermost_bracket(macros_p,start_b,end_b,&s_p,&e_p);
        if(depth){
            fprintf(stderr,"One of the Macros have misplaced start brackets '%s' and end brackets '%s'.\n",start_b,end_b);
            free(macros_def);
            return false;
        }
        if(!s_p) break;//Finished processing all macros.
        char* full_macro_str=char_string_slice_no_brackets(s_p,e_p,start_b),* macro_vars_str,* macro_name_str=0,* macro_vars_p,* def_str;
        const char* def_p;
        int macro_name_len, def_len;
        split_at_sep(full_macro_str,def_sep,&def_p,&macro_name_len,&def_len);
        if(!def_p){
            fprintf(stderr,"Separator %s not found.\n",def_sep);
            free(full_macro_str);
            free(macros_def);
            return false;
        }
        macro_vars_str=calloc(1,sizeof(char)*(macro_name_len+1));
        def_str=calloc(1,sizeof(char)*(def_len+1));
        EXIT_IF_NULL(macro_vars_str,char*);
        EXIT_IF_NULL(def_str,char*);
        strncpy(macro_vars_str,full_macro_str,macro_name_len);
        strncpy(def_str,def_p,def_len);
        free(full_macro_str);
        macro_vars_p=macro_vars_str;
        bool macro_name_set=false;
        for(size_t i=0,str_len=0;i<=strlen(macro_vars_str);i++){//i<=strlen to include '\0'.
            if(macro_vars_str[i]==var_sep||macro_vars_str[i]=='\0'){
                char* str=calloc(1,sizeof(char)*(str_len+1));
                EXIT_IF_NULL(str,char*);
                bool success;
                strncpy(str,macro_vars_p,str_len);
                if(macro_name_set){
                    success=macro_paster_add_var(this,macro_name_str,str);
                    if(!success){
                        free(macro_vars_str); free(macro_name_str); free(str); free(def_str); free(macros_def);
                        return false;
                    }
                    free(str);
                }else{
                    success=macro_paster_add_name(this,str);
                    if(!success){
                        free(macro_vars_str); free(str); free(def_str); free(macros_def);
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
                free(macro_vars_str); free(macro_name_str); free(def_str); free(macros_def);
                return false;
            }
        }
        macro_paster_write_macro_def(this,macro_name_str,def_str);
        free(macro_vars_str); free(macro_name_str); free(def_str);
        macros_p=e_p+strlen(end_b);//Get next macro to process.
    }
    free(macros_def);
    return true;
}
void _write_to_macro_output(const char* mo){
    FILE* f_obj;
    f_obj=fopen(MACRO_PROCESS_F,"w+");
    if(!f_obj) return;
    size_t wrote=fwrite(mo,strlen(mo),1,f_obj);
    fclose(f_obj);
    if(wrote){
        printf("Macro expansion written to %s\n",MACRO_PROCESS_F);
    }
}
bool macro_paster_expand_macros(macro_paster_t* this,const char* file_str,const char* end_m,const char* start_b,const char* end_b,char var_sep,char** output){
    *output=0;
    const char* begin_cmd_p;
    int m_i,cmd_len;
    split_at_sep(file_str,end_m,&begin_cmd_p,&m_i,&cmd_len);
    if(!begin_cmd_p){
        fprintf(stderr,"Error: Couldn't find macro end bracket '%s'.\n",end_m);
        return false;
    }
    char* cmd_str=malloc(sizeof(char)*(cmd_len+1));
    EXIT_IF_NULL(cmd_str,char*);
    strcpy(cmd_str,begin_cmd_p);
    size_t expansion_count=0;
    do{
        if(expansion_count>5000){
            fprintf(stderr,"Error: Possibly recursive macro in code. Ended recursion.\n");
            _write_to_macro_output(cmd_str);
            free(cmd_str);
            return false;
        }
        const char* begin_m_p,*end_m_p;
        int depth=first_innermost_bracket(cmd_str,start_b,end_b,&begin_m_p,&end_m_p);
        if(depth){
            fprintf(stderr,"Mismatched macro brackets in code.\n");
            _write_to_macro_output(cmd_str);
            free(cmd_str);
            return false;
        }
        if(!begin_m_p) break; //All macros expanded/processed.
        char* macro_w_br=char_string_slice_with_brackets(begin_m_p,end_m_p,end_b);//Used to replace for macro_paster_get_val_string
        char* macro_n_br=char_string_slice_no_brackets(begin_m_p,end_m_p,start_b);
        end_m_p+=+strlen(end_b)-1;//From char_string_slice_with_brackets to include the last end bracket.
        char* macro_name_str=0,* macro_vars_p=macro_n_br;
        size_t parse_len=strlen(macro_n_br),str_len=0,macro_name_i;
        int var_i=0;
        bool macro_name_set=false;
        for(size_t parse_i=0;parse_i<=parse_len;parse_i++){
            if(macro_n_br[parse_i]==var_sep||macro_n_br[parse_i]=='\0'){
                char* str=calloc(1,sizeof(char)*(str_len+1));
                EXIT_IF_NULL(str,char*);
                strncpy(str,macro_vars_p,str_len);
                if(macro_name_set){
                    if(!macro_paster_write_var_by_ind(this,macro_name_str,var_i,str)){ //Out of range.
                        _write_to_macro_output(cmd_str);
                        free(str); free(macro_name_str); free(macro_w_br); free(macro_n_br); free(cmd_str);
                        return false;
                    }
                    free(str);
                    var_i++;
                }else{
                    if(!_macro_paster_valid_name(this,str,&macro_name_i)){
                        fprintf(stderr,"Macro name '%s' is not a built-in macro or defined yet.\n",str);
                        _write_to_macro_output(cmd_str);
                        free(str); free(macro_w_br); free(macro_n_br); free(cmd_str);
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
            if(!macro_name_set&&!char_is_key(macro_n_br[parse_i])){
                fprintf(stderr,"Invalid character '%c' for macro name.\n",macro_n_br[parse_i]);
                _write_to_macro_output(cmd_str);
                free(macro_w_br); free(macro_n_br); free(cmd_str);
                return false;
            }
        }
        char* macro_val;
        macro_paster_get_val_string(this,macro_name_str,var_sep,&macro_val);
        trim_whitespace(&macro_val);
        replace_str_at(&cmd_str,macro_w_br,macro_val,begin_m_p,end_m_p);
        free(macro_val); free(macro_name_str); free(macro_w_br); free(macro_n_br);
        expansion_count++;
    }while(true);
    printf("Number of macro expansions: %lu\n",expansion_count);
    *output=cmd_str;
    _write_to_macro_output(cmd_str);
    return true;
}
//String output needs to be freed.
bool macro_paster_get_val_string(const macro_paster_t* this,const char* str_name,char prefix,char** output){
    size_t str_name_i;
    char* string_output;
    if(!_macro_paster_valid_name(this,str_name,&str_name_i)){
        fprintf(stderr,"In macro_paster, string '%s' does not exist. Did not write output.\n",str_name);
        return false;
    }
    string_output=malloc(sizeof(char)*(strlen(this->macro_definition[str_name_i])+1));
    EXIT_IF_NULL(string_output,char*);
    strcpy(string_output,this->macro_definition[str_name_i]);
    const int var_count=this->str_var_count[str_name_i];
    replace_node_t* rep_list=malloc(sizeof(replace_node_t)*var_count);
    EXIT_IF_NULL(rep_list,replace_node_t*);
    for(int str_var_i=0;str_var_i<var_count;str_var_i++){//Add prefix to all variable names and add vars/values to replace node.
        char* prefix_str=malloc(sizeof(char)*(strlen(this->str_vars[str_name_i][str_var_i])+2));
        EXIT_IF_NULL(prefix_str,char*);
        prefix_str[0]=prefix;
        strcpy(prefix_str+1,this->str_vars[str_name_i][str_var_i]);
        rep_list[str_var_i].r=prefix_str;
        rep_list[str_var_i].w=this->str_var_values[str_name_i][str_var_i];
    }
    replace_str_list(&string_output,rep_list,var_count);
    for(int str_var_i=0;str_var_i<var_count;str_var_i++){
        free((void*)rep_list[str_var_i].r);
    }
    free(rep_list);
    *output=string_output;
    return true;
}
void macro_paster_print(const macro_paster_t* this){
    for(size_t str_i=0;str_i<this->count;str_i++){
        printf("Macro name: ['%s'], ",this->str_names[str_i]);
        printf("Variables:[");
        for(int var_i=0;var_i<this->str_var_count[str_i];var_i++){
            char* const str_value=this->str_var_values[str_i][var_i];
            printf("%s=%s%s",this->str_vars[str_i][var_i],(str_value[0])?str_value:"(NULL)",(var_i!=this->str_var_count[str_i]-1)?",":"");
        }
        printf("], Macro Definition=['%s']\n",this->macro_definition[str_i][0]?this->macro_definition[str_i]:"(NULL)");
    }
}
void macro_paster_free(macro_paster_t* this){
    for(size_t str_i=0;str_i<this->count;str_i++){
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
size_t trim_whitespace(char** strptr){//For null-terminated strings only, and reedit pointer to resize for trimmed strings. Returns int to get total length of the trimmed string.
    int str_i=0;
    int whitespace_count=0;
    do{
        if(char_is_whitespace((*strptr)[str_i])) whitespace_count++;
        else (*strptr)[str_i-whitespace_count]=(*strptr)[str_i];//Copy the next non-whitespace character.
    }while((*strptr)[++str_i]);
    (*strptr)[str_i-whitespace_count]='\0';//Null terminate last character and reallocate as whitespace-trimmed string.
    *strptr=realloc(*strptr,(str_i-whitespace_count+1)*sizeof(char));
    EXIT_IF_NULL(*strptr,char*);
    return str_i-whitespace_count;
}
size_t trim_comments(char** strptr){//Same as above, but with comments until newline instead.
    int str_i=0;
    int comment_count=0;
    bool is_in_comment=false;
    do{
        if(!is_in_comment){
            if((*strptr)[str_i]!='#'){
                (*strptr)[str_i-comment_count]=(*strptr)[str_i];
                continue;
            }
            is_in_comment=true;
            comment_count++;
        }else{
            if((*strptr)[str_i]!='\n'){
                comment_count++;
                continue;
            }
            comment_count++;//Trim newline too
            is_in_comment=false;
        }
    }while((*strptr)[++str_i]);
    *strptr=realloc(*strptr,(str_i-comment_count+1)*sizeof(char));
    EXIT_IF_NULL(*strptr,char*);
    (*strptr)[str_i-comment_count]='\0';
    return str_i-comment_count;
}
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wstringop-truncation\"")
void replace_str(char** strptr, const char* replace, const char* with){
    char* new_strptr=(malloc(sizeof(char)));
    EXIT_IF_NULL(new_strptr,char*);
    size_t strptr_i=0;
    size_t new_strptr_i=0;
    const size_t replace_len=strlen(replace);
    const size_t with_len=strlen(with);
    char current_char;
    char* replace_p;
    while((current_char=*(*strptr+strptr_i))){
        if(!(replace_p=strchr(*strptr+strptr_i,*replace))) break;
        const size_t non_match_len=replace_p-*strptr-strptr_i;
        new_strptr=realloc(new_strptr,sizeof(char)*(new_strptr_i+non_match_len));//Strings that don't match replace.
        EXIT_IF_NULL(new_strptr,char*);
        strncpy(new_strptr+new_strptr_i,*strptr+strptr_i,non_match_len);
        new_strptr_i+=non_match_len;
        strptr_i+=non_match_len;
        if(strncmp(*strptr+strptr_i,replace,replace_len)){//First letter may not match the same word (!=0)
            new_strptr=realloc(new_strptr,(sizeof(char)*new_strptr_i+1));//To stop strchr from looping, add first character.
            EXIT_IF_NULL(new_strptr,char*);
            new_strptr[new_strptr_i++]=(*strptr)[strptr_i++];
            continue; 
        }
        new_strptr=realloc(new_strptr,sizeof(char)*(new_strptr_i+with_len));
        EXIT_IF_NULL(new_strptr,char*);
        strncpy(new_strptr+new_strptr_i,with,with_len);
        new_strptr_i+=with_len;//Increment string index by appropriate lengths.
        strptr_i+=replace_len;
    }
    const size_t last_chars_len=strchr(*strptr+strptr_i,'\0')-*strptr-strptr_i;
    new_strptr=realloc(new_strptr,sizeof(char)*(new_strptr_i+last_chars_len+1));
    EXIT_IF_NULL(new_strptr,char*);
    strcpy(new_strptr+new_strptr_i,*strptr+strptr_i);//Null-terminate with strcpy.
    free(*strptr);
    *strptr=new_strptr;//Change freed pointer to new pointer.
}
//Only replaces within range of pointers begin/end (inclusive).
//Will invalidate begin/end pointers after this call.
void replace_str_at(char** strptr_owner, const char* replace, const char* with,const char* begin,const char* end){
    char* new_strptr=(malloc(sizeof(char)));
    EXIT_IF_NULL(new_strptr,char*);
    size_t strptr_i=0;
    size_t new_strptr_i=0;
    const size_t replace_len=strlen(replace);
    const size_t with_len=strlen(with);
    char current_char;
    while((current_char=(*strptr_owner)[strptr_i])){
        char* strptr_owner_p=*strptr_owner+strptr_i;
        if(strptr_owner_p>=begin&&strptr_owner_p<=end&&current_char==replace[0]&&!strncmp(strptr_owner_p,replace,replace_len)){//0 in strncmp for same string contents.
            new_strptr=(realloc(new_strptr,sizeof(char)*(new_strptr_i+1+with_len)));
            EXIT_IF_NULL(new_strptr,char*);
            strncpy(new_strptr+new_strptr_i,with,with_len);//-Wstringop-truncation here (It null terminates after while loop)
            new_strptr_i+=with_len;
            strptr_i+=replace_len;
            continue;
        }
        new_strptr[new_strptr_i++]=current_char;
        strptr_i++;
        new_strptr=(realloc(new_strptr,sizeof(char)*new_strptr_i+1));
        EXIT_IF_NULL(new_strptr,char*);
    }
    new_strptr[new_strptr_i]='\0';//Null-terminate.
    free(*strptr_owner);
    *strptr_owner=new_strptr;//Change freed pointer to new pointer.
}
_Pragma("GCC diagnostic pop")
//Reverse lexicographical order so that longer words that overlap other words get replaced first.
//Ex: For strings ["abcde","abcd","abc"], it will replace "abcde" first.
int replace_node_biggest_first(const void* lhs,const void* rhs){
    return strcmp(((const replace_node_t*)rhs)->r,((const replace_node_t*)lhs)->r);
}
void replace_str_list(char** strptr_owner,replace_node_t* rep_list,size_t rep_list_size){
    qsort(rep_list,rep_list_size,sizeof(replace_node_t),replace_node_biggest_first);
    /*for(size_t i=0;i<rep_list_size;i++){
        printf("r:'%s' w:'%s'\n",rep_list[i].r,rep_list[i].w);
    }*/
    char* new_strptr=(malloc(sizeof(char)));
    EXIT_IF_NULL(new_strptr,char*);
    int strptr_i=0;
    int new_strptr_i=0;
    char current_char;
    while((current_char=(*strptr_owner)[strptr_i])){
        for(size_t i=0;i<rep_list_size;i++){
            const char* const replace=rep_list[i].r,* const with=rep_list[i].w;
            const size_t replace_len=strlen(replace),with_len=strlen(with);
            if(current_char==replace[0]&&!strncmp(*strptr_owner+strptr_i,replace,replace_len)){//0 in strncmp for same string contents.
                new_strptr=(realloc(new_strptr,sizeof(char)*(new_strptr_i+1+with_len)));
                EXIT_IF_NULL(new_strptr,char*);
                strncpy(new_strptr+new_strptr_i,with,with_len);
                new_strptr_i+=with_len;
                strptr_i+=replace_len;
                goto do_next_char;
            }
        }
        new_strptr[new_strptr_i++]=current_char;
        strptr_i++;
        new_strptr=(realloc(new_strptr,sizeof(char)*new_strptr_i+1));
        EXIT_IF_NULL(new_strptr,char*);
        do_next_char: continue;
    }
    new_strptr[new_strptr_i]='\0';//Null-terminate.
    free(*strptr_owner);
    *strptr_owner=new_strptr;//Change freed pointer to new pointer.
}
//Returns string from start_p char to end_p char [[nodiscard]] returns pointer that needs to be freed.
char* char_string_slice(const char* start_p,const char* end_p){
    assert(start_p<=end_p);
    char* char_p=malloc(sizeof(char)*(end_p-start_p+2));//+1 to count characters and +1 for '\0'
    EXIT_IF_NULL(char_p,char);
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
//Gets string length of start (start_len), split pointer after the seperator (split_p), and the length of the second split string (end_len).
//Seperator string is excluded for lengths and the split pointer.
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
static inline void _get_line_column_positions(const char* begin_p,const char* current_p,size_t* line_num,size_t* col_num){
    while(current_p>=begin_p){
        if(*current_p=='\n') break;
        (*col_num)++;
        current_p--;
    }
    while(current_p>=begin_p){ //Now count for only lines.
        if(*current_p=='\n') (*line_num)++;
        current_p--;
    }
}
//Where begin_p>=current_p>=(End of String). Line and column numbers (excluding characters like \n) start at 0.
void get_line_column_positions(const char* begin_p,const char* current_p,size_t* line_num,size_t* col_num){
    *col_num=-1;*line_num=0; _get_line_column_positions(begin_p,current_p,line_num,col_num);
}
//For numbers 1 and above.
void get_line_column_positions_p1(const char* begin_p,const char* current_p,size_t* line_num,size_t* col_num){
    *col_num=0;*line_num=1; _get_line_column_positions(begin_p,current_p,line_num,col_num);
}
//Null-terminated string with line and column numbers 0 or greater. Returns NULL if line/column is out of bounds.
const char* get_pointer_position(const char* str_p,size_t line_num,size_t col_num){
    const char* current_p=(line_num>0)?str_p:str_p-1;//Adjust for 0th line because it doesn't go to the first loop.
    while(current_p&&line_num--) current_p=strchr(++current_p,'\n');
    if(!current_p) return 0; //Out of bounds for line_num.
    do if(!*(++current_p)||*(current_p)=='\n') return 0; //Out of bounds for col_num.
    while(col_num--);
    return current_p;
}
//Slices into 3 strings to return a string with terminal control characters (cc_highlight). Use cc_reset to reset terminal.
char* print_string_highlight(const char* str_begin,const char* highlight_s,const char* highlight_e,const char* cc_highlight,const char* cc_reset){
    const char* const str_end=str_begin+strlen(str_begin)-1;
    const size_t begin_len=highlight_s-str_begin,highlight_len=highlight_e-highlight_s+1,end_len=str_end-highlight_e;
    char* strings[3]={
        malloc(sizeof(char)*(begin_len+1)),malloc(sizeof(char)*(highlight_len+1)),malloc(sizeof(char)*(end_len+1))
    };
    EXIT_IF_NULL(strings[0],char*); EXIT_IF_NULL(strings[1],char*); EXIT_IF_NULL(strings[2],char*);
    strncpy(strings[0],str_begin,begin_len);
    strncpy(strings[1],highlight_s,highlight_len);
    strncpy(strings[2],highlight_e+1,end_len);
    strings[0][begin_len]=strings[1][highlight_len]=strings[2][end_len]='\0';
    char* result=malloc(sizeof(char)*(begin_len+highlight_len+end_len+strlen(cc_highlight)+strlen(cc_reset)+1));
    EXIT_IF_NULL(result,char*);
    sprintf(result,"%s%s%s%s%s",strings[0],cc_highlight,strings[1],cc_reset,strings[2]);
    free(strings[0]); free(strings[1]); free(strings[2]);
    return result;
}
//Reads a portion of a string by maximum amount of lines, depending where str_read is.
char* string_read_view(const char* str_begin,const char* str_read,size_t maximum_lines){
    const char* const str_end=str_begin+strlen(str_begin)-1,* str_slice_b=str_read,* str_slice_e=str_read;
    size_t above=maximum_lines+1,below=maximum_lines+1;
    while(above){
        if(str_slice_b==str_begin) break;
        if(*(--str_slice_b)=='\n') above--;
    }
    if(!above) str_slice_b++;//Exclude new lines.
    below+=above;//Add any remaining lines
    while(below){
        if(str_slice_e==str_end) break;
        if(*(++str_slice_e)=='\n') below--;
    }
    if(!below) str_slice_e--;
    return char_string_slice(str_slice_b,str_slice_e);
}