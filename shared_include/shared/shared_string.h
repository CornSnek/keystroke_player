#ifndef _RC_STRING_H_
#define _RC_STRING_H_
#include "macros.h"
#include <stdbool.h>
typedef struct shared_string_manager_s{//Container of strings that point to the same string with the same contents. To malloc only unique strings.
    int count;
    char** c_strs;
    int* c_str_rc;//Reference count to destroy same pointer strings.
}shared_string_manager_t;
shared_string_manager_t* SSManager_new(void);
char* SSManager_add_string(shared_string_manager_t* this, char** str_p_owned);
int SSManager_count_string(const shared_string_manager_t* this, const char* str_cmp);
void SSManager_print_strings(const shared_string_manager_t* this);
void SSManager_free_string(shared_string_manager_t* this, const char* str_del);
void SSManager_free(shared_string_manager_t* this);
int trim_whitespace(char** strptr_owner);
void replace_str(char** strptr, const char* replace, const char* with);
bool char_is_key(char c);
bool char_is_keystate(char c);
bool char_is_whitespace(char c);
bool char_is_delay(char c);
char* char_string_slice(char* start_p,char* end_p);
int first_innermost_bracket(const char* search_str,const char* begin_pat,const char* end_pat,const char** find_begin_p,const char** find_end_p);
void next_dyck_word(bool* bits,size_t bits_size);
size_t catalan(unsigned int n);
#endif
