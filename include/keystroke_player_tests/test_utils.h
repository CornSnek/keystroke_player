#ifndef _TEST_UTILS_H_
#define _TEST_UTILS_H_
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/random.h>
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);
#include "macros.h"
void next_dyck_word(bool* bits,size_t bits_size);
size_t catalan(unsigned int n);
char* str_dup(const char* str);
inline static size_t random_size_t(void){
    size_t rand_st=0;
    do{}while(getrandom(&rand_st,sizeof(rand_st),0)==-1);
    return rand_st;
}
char* random_str(unsigned char len);
size_t* random_unique_indices(size_t len);
#endif