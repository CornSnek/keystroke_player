#ifndef _RC_STRING_H_
#define _RC_STRING_H_
#include "macros.h"
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
typedef struct shared_string_manager_s{//Container of strings that point to the same string with the same contents. Frees duplicate strings.
    int count;
    char** c_strs;
    int* c_str_rc;//Reference count to destroy same pointer strings.
}shared_string_manager_t;
typedef struct ts_macro_paster_s{
    size_t count;
    char** str_names;
    char** macro_definition;
    int* str_var_count;
    char*** str_vars; //Or an array of "variable strings" for each "string name" in str_names
    char*** str_var_values;
}ts_macro_paster_t;
typedef struct replace_node_s{//To sort and replace words from r to w.
    const char* r;
    const char* w;
}replace_node_t;
shared_string_manager_t* SSManager_new(void);
bool SSManager_add_string(shared_string_manager_t* this, char** str_p_owned);
int SSManager_count_string(const shared_string_manager_t* this, const char* str_cmp);
void SSManager_print_strings(const shared_string_manager_t* this);
void SSManager_free_string(shared_string_manager_t* this, const char* str_del);
void SSManager_free(shared_string_manager_t* this);
ts_macro_paster_t* ts_macro_paster_new(void);
bool ts_macro_paster_add_name(ts_macro_paster_t* this,const char* str_name);
bool ts_macro_paster_valid_name(const ts_macro_paster_t* this,const char* str_name,size_t* at_index);
bool ts_macro_paster_add_var(ts_macro_paster_t* this,const char* str_name,const char* var_name);
bool ts_macro_paster_write_macro_def(ts_macro_paster_t* this,const char* str_name,const char* str_value);
bool ts_macro_paster_write_var_by_str(ts_macro_paster_t* this,const char* str_name,const char* var_name,const char* var_value);
bool ts_macro_paster_write_var_by_ind(ts_macro_paster_t* this,const char* str_name,int var_i,const char* var_value);
typedef enum _MacroProcessStatus{
    MPS_NoMacros,
    MPS_ImproperBrackets,
    MPS_HasDefinitions,
    MPS_HasBuiltins
}MacroProcessStatus;
MacroProcessStatus file_contains_macro_definitions(const char* file_str,const char* start_m,const char* end_m);
bool ts_macro_paster_process_macros(ts_macro_paster_t* this,const char* file_str,const char* start_m,const char* end_m,const char*start_b,const char* end_b,const char* def_sep,char var_sep);
bool ts_macro_paster_expand_macros(ts_macro_paster_t* this,const char* file_str,const char* end_m,const char*start_b,const char* end_b,char var_sep,char** output);
bool ts_macro_paster_get_val_string(const ts_macro_paster_t* this,const char* str_name,char prefix,char** output_owner);
void ts_macro_paster_print(const ts_macro_paster_t* this);
void ts_macro_paster_free(ts_macro_paster_t* this);
size_t trim_whitespace(char** strptr);
size_t trim_comments(char** strptr);
int replace_node_biggest_first(const void* lhs_v,const void* rhs_v);
void replace_str(char** strptr, const char* replace, const char* with);
char* replace_str_array(const char* str, const char* replace,int n,const char** with_arr);
void replace_str_at(char** strptr_owner, const char* replace, const char* with,const char* begin,const char* end);
void replace_str_list(char** strptr_owner,replace_node_t* rep_list,size_t rep_list_size);
static inline bool char_is_x11_key(char c){
    return isalnum(c)||(c=='_')||(c=='+');
}
static inline bool char_is_rmacro(char c){
    return isalnum(c)||(c=='_')||(c=='@');
}
static inline bool char_is_key(char c){
    return isalnum(c)||(c=='_');
}
static inline bool char_is_keystate(char c){
    return (c=='u')||(c=='d')||(c=='c')||(c=='U')||(c=='D')||(c=='C');
}
static inline bool char_is_whitespace(char c){
    return (c==' ')||(c=='\t')||(c=='\n')||(c=='\v')||(c=='\f')||(c=='\r');
}
static inline bool char_is_delay(char c){
    return (c=='m')||(c=='u')||(c=='s')||(c=='M')||(c=='U')||(c=='S');
}
char* char_string_slice(const char* start_p,const char* end_p);
//Assuming they're enclosed in const char* brackets from the ts_macro_paster_t codes.
static inline char* char_string_slice_no_brackets(const char* start_p,const char* end_p,const char* start_b){
    return char_string_slice(start_p+strlen(start_b),end_p-1);
}
static inline char* char_string_slice_with_brackets(const char* start_p,const char* end_p,const char* end_b){
    return char_string_slice(start_p,end_p+strlen(end_b)-1);
}
int first_innermost_bracket(const char* search_str,const char* begin_bracket,const char* end_bracket,const char** find_begin_p,const char** find_end_p);
int first_outermost_bracket(const char* search_str,const char* begin_bracket,const char* end_bracket,const char** find_begin_p,const char** find_end_p);
void split_at_sep(const char* search_str,const char* sep,const char** split_p,int* start_len,int* end_len);
void get_line_column_positions(const char* begin_p,const char* current_p,size_t* line_num,size_t* char_num);
void get_line_column_positions_p1(const char* begin_p,const char* current_p,size_t* line_num,size_t* char_num);
const char* get_pointer_position(const char* str_p,size_t line_num,size_t column_num);
char* print_string_highlight(const char* str_begin,const char* highlight_s,const char* highlight_e,const char* cc_highlight,const char* cc_reset);
char* string_read_view(const char* str_begin,const char* str_read,size_t maximum_lines);
#endif
