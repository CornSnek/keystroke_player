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
typedef bool (*rpn_cmp_f_long)(long,long);
typedef double (*rpn_double_f_noarg)();
typedef double (*rpn_double_f_1double)(double);
typedef double (*rpn_double_f_2double)(double,double);
typedef bool (*rpn_cmp_f_double)(double,double);
typedef int (*rpn_int_f_noarg)();
typedef int (*rpn_int_f_1int)(int);
typedef int (*rpn_int_f_2int)(int,int);
typedef bool (*rpn_cmp_f_int)(int,int);
typedef char (*rpn_char_f_noarg)();
typedef char (*rpn_char_f_1char)(char);
typedef char (*rpn_char_f_2char)(char,char);
typedef bool (*rpn_cmp_f_char)(char,char);
typedef enum _RPNFuncType{
    RPNFT_Null,
    RPNFT_Long_F_NoArg,
    RPNFT_Long_F_1Long,
    RPNFT_Long_F_2Long,
    RPNFT_Cmp_F_Long,
    RPNFT_Double_F_NoArg,
    RPNFT_Double_F_1Double,
    RPNFT_Double_F_2Double,
    RPNFT_Cmp_F_Double,
    RPNFT_Int_F_NoArg,
    RPNFT_Int_F_1Int,
    RPNFT_Int_F_2Int,
    RPNFT_Cmp_F_Int,
    RPNFT_Char_F_NoArg,
    RPNFT_Char_F_1Char,
    RPNFT_Char_F_2Char,
    RPNFT_Cmp_F_Char,
}RPNFuncType;
typedef union _rpn_function_u{
    rpn_null rpn_null;
    rpn_long_f_noarg rpn_long_f_noarg;
    rpn_long_f_1long rpn_long_f_1long;
    rpn_long_f_2long rpn_long_f_2long;
    rpn_cmp_f_long rpn_cmp_f_long;
    rpn_double_f_noarg rpn_double_f_noarg;
    rpn_double_f_1double rpn_double_f_1double;
    rpn_double_f_2double rpn_double_f_2double;
    rpn_cmp_f_double rpn_cmp_f_double;
    rpn_int_f_noarg rpn_int_f_noarg;
    rpn_int_f_1int rpn_int_f_1int;
    rpn_int_f_2int rpn_int_f_2int;
    rpn_cmp_f_int rpn_cmp_f_int;
    rpn_char_f_noarg rpn_char_f_noarg;
    rpn_char_f_1char rpn_char_f_1char;
    rpn_char_f_2char rpn_char_f_2char;
    rpn_cmp_f_char rpn_cmp_f_char;
}rpn_function_u;
typedef struct rpn_func_call_s{
    RPNFuncType type;
    rpn_function_u func;
    int num_args;
    VLNumberType return_type;//To truncate to the return type in case of casts that downgrade a number.
}rpn_func_call_t;
StringMap_ImplDecl(rpn_func_call_t,rpn_func_call)
void RPNEvaluatorInit();
typedef enum _RPNValidStringE{
    RPNVS_Ok,
    RPNVS_ImproperBrackets,
    RPNVS_NameCollision,
    RPNVS_NameNotAdded,
    RPNVS_IsFunction,
    RPNVS_IsVLName,
    RPNVS_OutOfNumbers,
    RPNVS_TooManyNumbers
}RPNValidStringE;
RPNValidStringE RPNEvaluatorValidString(const char* rpn_str,const VariableLoader_t* vl,const char* rpn_start_b,const char* rpn_end_b,char rpn_sep);
void RPNEvaluatorFree();
#endif