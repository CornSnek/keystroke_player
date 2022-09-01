#ifndef _RPN_EVALUATOR_H_
#define _RPN_EVALUATOR_H_
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "macros.h"
#include "variable_loader.h"
#include "shared_string.h"
#include "generics/stack_generic.h"
#include "generics/hash_map.h"
Stack_ImplDecl(as_number_t,as_number)
typedef void (*rpn_null)(void);
typedef long (*rpn_long_f_noarg)();
typedef long (*rpn_long_f_1long)(long);
typedef long (*rpn_long_f_2long)(long,long);
typedef bool (*rpn_long_f_ucmp)(unsigned long,unsigned long);//For comparison with unsigned comparison.
typedef bool (*rpn_long_f_cmp)(long,long);
typedef long (*rpn_long_f_ternary)(bool,long,long);
typedef double (*rpn_double_f_noarg)();
typedef double (*rpn_double_f_1double)(double);
typedef double (*rpn_double_f_2double)(double,double);
typedef bool (*rpn_double_f_cmp)(double,double);
typedef double (*rpn_double_f_ternary)(bool,double,double);
typedef int (*rpn_int_f_noarg)();
typedef int (*rpn_int_f_1int)(int);
typedef int (*rpn_int_f_2int)(int,int);
typedef bool (*rpn_int_f_cmp)(int,int);
typedef bool (*rpn_int_f_ucmp)(unsigned int,unsigned int);
typedef int (*rpn_int_f_ternary)(bool,int,int);
typedef char (*rpn_char_f_noarg)();
typedef char (*rpn_char_f_1char)(char);
typedef char (*rpn_char_f_2char)(char,char);
typedef bool (*rpn_char_f_cmp)(char,char);
typedef char (*rpn_char_f_ternary)(bool,char,char);
typedef bool (*rpn_char_f_ucmp)(unsigned char,unsigned char);
typedef bool (*rpn_invert)(bool);
typedef bool (*rpn_f_2_bools)(bool,bool);
typedef enum _RPNFuncType{
    RPNFT_Null,
    RPNFT_Long_F_NoArg,
    RPNFT_Long_F_1Long,
    RPNFT_Long_F_2Long,
    RPNFT_Long_F_Cmp,
    RPNFT_Long_F_UCmp,
    RPNFT_Long_F_Ternary,
    RPNFT_Double_F_NoArg,
    RPNFT_Double_F_1Double,
    RPNFT_Double_F_2Double,
    RPNFT_Double_F_Cmp,
    RPNFT_Double_F_Ternary,
    RPNFT_Int_F_NoArg,
    RPNFT_Int_F_1Int,
    RPNFT_Int_F_2Int,
    RPNFT_Int_F_Cmp,
    RPNFT_Int_F_UCmp,
    RPNFT_Int_F_Ternary,
    RPNFT_Char_F_NoArg,
    RPNFT_Char_F_1Char,
    RPNFT_Char_F_2Char,
    RPNFT_Char_F_Cmp,
    RPNFT_Char_F_UCmp,
    RPNFT_Char_F_Ternary,
    RPNFT_Invert,
    RPNFT_2_Bools,
}RPNFuncType;
typedef union _rpn_function_u{
    rpn_null rpn_null;
    rpn_long_f_noarg rpn_long_f_noarg;
    rpn_long_f_1long rpn_long_f_1long;
    rpn_long_f_2long rpn_long_f_2long;
    rpn_long_f_cmp rpn_long_f_cmp;
    rpn_long_f_ucmp rpn_long_f_ucmp;
    rpn_long_f_ternary rpn_long_f_ternary;
    rpn_double_f_noarg rpn_double_f_noarg;
    rpn_double_f_1double rpn_double_f_1double;
    rpn_double_f_2double rpn_double_f_2double;
    rpn_double_f_ternary rpn_double_f_ternary;
    rpn_double_f_cmp rpn_double_f_cmp;
    rpn_int_f_noarg rpn_int_f_noarg;
    rpn_int_f_1int rpn_int_f_1int;
    rpn_int_f_2int rpn_int_f_2int;
    rpn_int_f_cmp rpn_int_f_cmp;
    rpn_int_f_ucmp rpn_int_f_ucmp;
    rpn_int_f_ternary rpn_int_f_ternary;
    rpn_char_f_noarg rpn_char_f_noarg;
    rpn_char_f_1char rpn_char_f_1char;
    rpn_char_f_2char rpn_char_f_2char;
    rpn_char_f_cmp rpn_char_f_cmp;
    rpn_char_f_ucmp rpn_char_f_ucmp;
    rpn_char_f_ternary rpn_char_f_ternary;
    rpn_invert rpn_invert;
    rpn_f_2_bools rpn_f_2_bools;
}rpn_function_u;//void*, union with types and and a struct with union+enum that refers to the type (Like some tagged union or something).
typedef struct rpn_func_call_s{
    RPNFuncType type;
    rpn_function_u func;
    int num_args;
    VLNumberType return_type;//To truncate to the return type in case of casts that may downgrade a number.
}rpn_func_call_t;
StringMap_ImplDecl(rpn_func_call_t,rpn_func_call,
(const char* str){
    hash_t hash=5381;//djb hashing
    int c;
    while((c=*str++)) hash=((hash<<5)+hash)+c;
    return hash;
})
void RPNEvaluatorInit();
typedef enum _RPNValidStringE{
    RPNVS_Ok,
    RPNVS_ImproperBrackets,
    RPNVS_NameCollision,
    RPNVS_NameUndefined,
    RPNVS_IsFunction,
    RPNVS_IsVLName,
    RPNVS_OutOfNumbers,
    RPNVS_TooManyNumbers,
    RPNVS_DivideByZero
}RPNValidStringE;
RPNValidStringE RPNEvaluatorGetNumber(const char* rpn_str,const VariableLoader_t* vl,as_number_t* get_value,bool see_stack,const char* rpn_start_b,const char* rpn_end_b,char rpn_sep);
void RPNEvaluatorFree();
#endif