#ifndef _RPN_EVALUATOR_H_
#define _RPN_EVALUATOR_H_
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "macros.h"
#include "variable_loader.h"
#include "shared_string.h"
typedef struct rpn_numvar_s{
    LD_e type;
    LD_u LD;
}rpn_numvar_t;
typedef long (*rpn_long_f_noarg)();
typedef long (*rpn_long_f_1long)(long);
typedef long (*rpn_long_f_2long)(long,long);
typedef bool (*rpn_cmp_f)(long,long);
typedef double (*rpn_double_f_noarg)();
typedef double (*rpn_double_f_1double)(double);
typedef double (*rpn_double_f_2double)(double,double);
typedef bool (*rpn_cmp_d)(double,double);
typedef long (*rpn_castas_l)(double);
typedef double (*rpn_castas_d)(long);
typedef enum _RPNFuncType{
    RPNFT_Long_F_1Long,
    RPNFT_Long_F_2Long,
    RPNFT_Double_F_1Double,
    RPNFT_Double_F_2Double,
    RPNFT_Cmp_F,
    RPNFT_Cmp_D,
    RPNFT_Long_F_NoArg,
    RPNFT_Double_F_NoArg,
    RPNFT_CastAsL,
    RPNFT_CastAsD
}RPNFuncType;
typedef union _rpn_function_u{
    rpn_long_f_1long rpn_long_f_1long;
    rpn_long_f_2long rpn_long_f_2long;
    rpn_double_f_1double rpn_double_f_1double;
    rpn_double_f_2double rpn_double_f_2double;
    rpn_cmp_f rpn_cmp_f;
    rpn_cmp_d rpn_cmp_d;
    rpn_long_f_noarg rpn_long_f_noarg;
    rpn_double_f_noarg rpn_double_f_noarg;
    rpn_castas_l rpn_castas_l;
    rpn_castas_d rpn_castas_d;
}rpn_function_u;
typedef struct rpn_func_call_s{
    RPNFuncType type;
    rpn_function_u func;
}rpn_func_call_t;
#include "generics/hash_map.h"
StringMap_ImplDecl(rpn_func_call_t,rpn_func_call)
void RPNEvaluatorInit();
bool _RPNEvaluatorNameUsed(const char* variable);
typedef enum _RPNValidStringE{
    RPNVS_Valid,RPNVS_ImproperBrackets
}RPNValidStringE;
RPNValidStringE RPNEvaluatorValidString(const char* rpn_str,VariableLoader_t* vl,const char* rpn_start_b,const char* rpn_end_b,char rpn_sep);
void RPNEvaluatorFree();
#endif