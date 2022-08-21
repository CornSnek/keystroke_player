#ifndef _STACK_GENERIC_H_
#define _STACK_GENERIC_H_
#include "macros.h"
//TODO: Add simple stack first.
typedef int StackType;
typedef struct Stack_s{
    int size;
    StackType* v;
}Stack_t;
Stack_t* Stack_new(){
    Stack_t* this=malloc(sizeof(Stack_t));
    return this;
}
#endif