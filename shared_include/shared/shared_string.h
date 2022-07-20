#ifndef _RC_STRING_H_
#define _RC_STRING_H_
#include "macros.h"
#include <stdbool.h>
typedef struct{//Container of strings that point to the same string with the same contents. To malloc only unique strings.
    int count;
    char** c_strs;
    int* c_str_rc;//Reference count to destroy same pointer strings.
}shared_string_manager;
shared_string_manager* SSManager_new(void);
char* SSManager_add_string(shared_string_manager* this, char** p_owner str_p_owned);
int SSManager_count_string(const shared_string_manager* this, const char* str_cmp);
void SSManager_print_strings(const shared_string_manager* this);
void SSManager_free_string(shared_string_manager* this, const char* str_del);
void SSManager_free(shared_string_manager* this);
#endif
