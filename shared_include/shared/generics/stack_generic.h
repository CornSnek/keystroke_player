#ifndef _STACK_GENERIC_H_
#define _STACK_GENERIC_H_
#include <stdbool.h>
#include "macros.h"
#define Stack_ImplDecl(StackType,TypeName)\
typedef struct Stack_##TypeName##_s{\
    int size;\
    StackType* stack;\
}Stack_##TypeName##_t;\
typedef struct StackV_##TypeName##_s{\
    bool exists;\
    StackType v;\
}StackV_##TypeName##_t;\
Stack_##TypeName##_t* Stack_##TypeName##_new(void);\
void Stack_##TypeName##_push(Stack_##TypeName##_t* this,StackType v);\
StackV_##TypeName##_t Stack_##TypeName##_pop(Stack_##TypeName##_t* this);\
StackV_##TypeName##_t Stack_##TypeName##_peek(Stack_##TypeName##_t* this);\
void Stack_##TypeName##_free(Stack_##TypeName##_t* this);\
void Stack_##TypeName##_empty(Stack_##TypeName##_t* this);
Stack_ImplDecl(int,TypeName)
#define Stack_ImplDef(StackType,TypeName)\
Stack_##TypeName##_t* Stack_##TypeName##_new(void){\
    Stack_##TypeName##_t* this=calloc(1,sizeof(Stack_##TypeName##_t));\
    EXIT_IF_NULL(this,Stack_##TypeName##_t*);\
    return this;\
}\
void Stack_##TypeName##_push(Stack_##TypeName##_t* this,StackType v){\
    this->size++;\
    this->stack=this->stack?realloc(this->stack,sizeof(StackType)*this->size):malloc(sizeof(StackType));\
    EXIT_IF_NULL(this->stack,StackType*);\
    this->stack[this->size-1]=v;\
}\
StackV_##TypeName##_t Stack_##TypeName##_pop(Stack_##TypeName##_t* this){\
    if(this->stack){\
        StackType popped_v=this->stack[--this->size];\
        if(this->size){\
            this->stack=realloc(this->stack,sizeof(Stack_##TypeName##_t)*this->size);\
            EXIT_IF_NULL(this->stack,Stack_##TypeName##_t);\
        }else{\
            free(this->stack);\
            this->stack=0;\
        }\
        return (StackV_##TypeName##_t){.exists=true,.v=popped_v};\
    }else return (StackV_##TypeName##_t){0};\
}\
StackV_##TypeName##_t Stack_##TypeName##_peek(Stack_##TypeName##_t* this){\
    return (this->stack)?(StackV_##TypeName##_t){.exists=true,.v=this->stack[this->size-1]}:(StackV_##TypeName##_t){0};\
}\
void Stack_##TypeName##_empty(Stack_##TypeName##_t* this){\
    free(this->stack);\
    this->stack=0;\
}\
void Stack_##TypeName##_free(Stack_##TypeName##_t* this){\
    free(this->stack);\
    free(this);\
}
//Declarations
#if 0
typedef int StackType;
typedef struct Stack_TypeName_s{
    int size;
    StackType* stack;
}Stack_TypeName_t;
typedef struct StackV_TypeName_s{
    bool exists;
    StackType v;
}StackV_TypeName_t;
Stack_TypeName_t* Stack_TypeName_new(void);
void Stack_TypeName_push(Stack_TypeName_t* this,StackType v);
StackV_TypeName_t Stack_TypeName_pop(Stack_TypeName_t* this);
StackV_TypeName_t Stack_TypeName_peek(Stack_TypeName_t* this);
void Stack_TypeName_free(Stack_TypeName_t* this);
void Stack_TypeName_empty(Stack_TypeName_t* this);
#endif
//Definitions
#if 0
Stack_TypeName_t* Stack_TypeName_new(void){
    Stack_TypeName_t* this=calloc(1,sizeof(Stack_TypeName_t));
    EXIT_IF_NULL(this,Stack_TypeName_t*);
    return this;
}
void Stack_TypeName_push(Stack_TypeName_t* this,StackType v){
    this->size++;
    this->stack=this->stack?realloc(this->stack,sizeof(StackType)*this->size):malloc(sizeof(StackType));
    EXIT_IF_NULL(this->stack,StackType*);
    this->stack[this->size-1]=v;
}
StackV_TypeName_t Stack_TypeName_pop(Stack_TypeName_t* this){
    if(this->stack){
        StackType popped_v=this->stack[--this->size];
        if(this->size){
            this->stack=realloc(this->stack,sizeof(Stack_TypeName_t)*this->size);
            EXIT_IF_NULL(this->stack,Stack_TypeName_t);
        }else{
            free(this->stack);
            this->stack=0;
        }
        return (StackV_TypeName_t){.exists=true,.v=popped_v};
    }else return (StackV_TypeName_t){0};
}
StackV_TypeName_t Stack_TypeName_peek(Stack_TypeName_t* this){
    return (this->stack)?(StackV_TypeName_t){.exists=true,.v=this->stack[this->size-1]}:(StackV_TypeName_t){0};
}
void Stack_TypeName_empty(Stack_TypeName_t* this){
    free(this->stack);
    this->stack=0;
}
void Stack_TypeName_free(Stack_TypeName_t* this){
    free(this->stack);
    free(this);
}
#endif
Stack_ImplDef(int,TypeName)
#endif