#ifndef _RC_STRING_H_
#define _RC_STRING_H_
#include "macros.h"
#include <stdbool.h>
#include <string.h>
typedef struct shared_string_manager_s{//Container of strings that point to the same string with the same contents. To malloc only unique strings.
    int count;
    char** c_strs;
    int* c_str_rc;//Reference count to destroy same pointer strings.
}shared_string_manager_t;
typedef struct macro_paster_s{
    int count;
    char** str_names;
    char** macro_definition;
    int* str_var_count;
    char*** str_vars; //Or an array of "variable strings" for each "string name" in str_names
    char*** str_var_values;
}macro_paster_t;
typedef struct replace_node_s{//To sort and replace words from r to w.
    const char* r;
    const char* w;
}replace_node_t;
shared_string_manager_t* SSManager_new(void);
char* SSManager_add_string(shared_string_manager_t* this, char** str_p_owned);
int SSManager_count_string(const shared_string_manager_t* this, const char* str_cmp);
void SSManager_print_strings(const shared_string_manager_t* this);
void SSManager_free_string(shared_string_manager_t* this, const char* str_del);
void SSManager_free(shared_string_manager_t* this);
macro_paster_t* macro_paster_new(void);
bool macro_paster_add_name(macro_paster_t* this,const char* str_name);
bool macro_paster_add_var(macro_paster_t* this,const char* str_name,const char* var_name);
bool macro_paster_write_macro_def(macro_paster_t* this,const char* str_name,const char* str_value);
bool macro_paster_write_var_by_str(macro_paster_t* this,const char* str_name,const char* var_name,const char* var_value);
bool macro_paster_write_var_by_ind(macro_paster_t* this,const char* str_name,int var_i,const char* var_value);
bool macro_paster_process_macros(macro_paster_t* this,const char* file_str,const char* start_m,const char* end_m,const char*start_b,const char* end_b,const char* def_sep,char var_sep);
bool macro_paster_expand_macros(const macro_paster_t* this,const char* file_str,const char* end_m,const char*start_b,const char* end_b,const char* def_sep,char var_sep);
bool macro_paster_get_string(const macro_paster_t* this,const char* str_name,char prefix,char** output_owner);
void macro_paster_print(const macro_paster_t* this);
void macro_paster_free(macro_paster_t* this);
int trim_whitespace(char** strptr_owner);
int replace_node_biggest_first(const void* lhs_v,const void* rhs_v);
void replace_str(char** strptr, const char* replace, const char* with);
void replace_str_list(char** strptr_owner,replace_node_t* rep_list,size_t rep_list_size);
bool char_is_key(char c);
bool char_is_keystate(char c);
bool char_is_whitespace(char c);
bool char_is_delay(char c);
char* char_string_slice(const char* start_p,const char* end_p);
static inline char* char_string_slice_no_brackets(const char* start_p,const char* end_p,const char* start_b){
    return char_string_slice(start_p+strlen(start_b),end_p-1);
}
static inline char* char_string_slice_with_brackets(const char* start_p,const char* end_p,const char* end_b){
    return char_string_slice(start_p,end_p+strlen(end_b)-1);
}
int first_innermost_bracket(const char* search_str,const char* begin_bracket,const char* end_bracket,const char** find_begin_p,const char** find_end_p);
int first_outermost_bracket(const char* search_str,const char* begin_bracket,const char* end_bracket,const char** find_begin_p,const char** find_end_p);
void split_at_sep(const char* search_str,const char* sep,const char** split_p,int* start_len,int* end_len);
#endif
