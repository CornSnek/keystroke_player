#include "rpn_evaluator.h"
StringMap_ImplDef(rpn_func_call_t,rpn_func_call)
StringMap_rpn_func_call_t* DefaultRPNMap=0;
#define BinOps(Type,Type_suf,f_name,o) Type RPN_##Type_suf##_##f_name(Type a,Type b){return a o b;}
#define BinCmp(Type,Type_suf,f_name,o) bool RPN_##Type_suf##_##f_name(Type a,Type b){return a o b;}
#define UnOps(Type,Type_suf,f_name,o) Type RPN_##Type_suf##_##f_name(Type a){return o a;}
BinOps(long,l,add,+) UnOps(long,l,inc,++)
BinOps(long,l,sub,-) UnOps(long,l,neg,-) UnOps(long,l,dec,--)
BinOps(long,l,mul,*)
BinOps(long,l,div,/)
BinOps(long,l,mod,%)
BinOps(long,l,b_and,&) BinOps(long,l,b_or,|) UnOps(long,l,b_not,~) BinOps(long,l,b_xor,^) BinOps(long,l,bs_l,<<) BinOps(long,l,bs_r,>>)
BinCmp(long,l,eq,==) BinCmp(long,l,neq,!=) BinCmp(long,l,lt,<) BinCmp(long,l,gt,>) BinCmp(long,l,lte,<=) BinCmp(long,l,gte,>=)
BinOps(double,d,add,+) UnOps(double,d,inc,++)
BinOps(double,d,sub,-) UnOps(double,d,dec,--) UnOps(double,d,neg,-)
BinOps(double,d,mul,*)
BinOps(double,d,div,/)
bool RPN_d_eq(double a,double b){ return fabs(a-b)<=DBL_EPSILON; }
bool RPN_d_neq(double a,double b){ return fabs(a-b)>DBL_EPSILON; }
long RPN_l_max(long a,long b){ return a>=b?a:b; }
long RPN_l_min(long a,long b){ return a<=b?a:b; }
BinCmp(double,d,lt,<) BinCmp(double,d,gt,>) BinCmp(double,d,lte,<=) BinCmp(double,d,gte,>=)
#define __PI 3.141592653589793238
static inline double deg_to_rad(double deg){
    return deg*__PI/180.;
}
#define TrigDegWrap(name) double name##w(double deg){ return name(deg_to_rad(deg)); }
TrigDegWrap(sin) TrigDegWrap(cos) TrigDegWrap(tan)
#include <sys/random.h>
#include <limits.h>
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);
long random_l(){
    long rand_st=0;
    do{}while(getrandom(&rand_st,sizeof(rand_st),0)==-1);
    return rand_st;
}
double random_d(){
    return (double)(size_t)random_l()/ULONG_MAX;
}
long castas_l(double d){
    return (long)d;
}
double castas_d(long l){
    return (double)l;
}
//To make the default StringMap before calling rpn_evaluator_new.
void RPNEvaluatorInit(){
    if(!DefaultRPNMap){
        DefaultRPNMap=StringMap_rpn_func_call_new(100);
#define SMA(Str,RFT,RFT_u,F) assert(StringMap_rpn_func_call_assign(DefaultRPNMap,Str,(rpn_func_call_t){.type=RFT,.func.RFT_u=F})==VA_Written)
        SMA("l+",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_add);
        SMA("l++",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_inc);
        SMA("l-",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_sub);
        SMA("l-m",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_neg);
        SMA("l--",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_dec);
        SMA("l*",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_mul);
        SMA("l/",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_div);
        SMA("l%",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_mod);
        SMA("l&",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_and);
        SMA("l|",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_or);
        SMA("l~",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_b_not);
        SMA("l^",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_xor);
        SMA("l<<",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_bs_l);
        SMA("l>>",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_bs_r);
        SMA("l==",RPNFT_Cmp_F,rpn_cmp_f,RPN_l_eq);
        SMA("l!=",RPNFT_Cmp_F,rpn_cmp_f,RPN_l_neq);
        SMA("l>",RPNFT_Cmp_F,rpn_cmp_f,RPN_l_gt);
        SMA("l<",RPNFT_Cmp_F,rpn_cmp_f,RPN_l_lt);
        SMA("l>=",RPNFT_Cmp_F,rpn_cmp_f,RPN_l_gte);
        SMA("l<=",RPNFT_Cmp_F,rpn_cmp_f,RPN_l_lte);
        SMA("labs",RPNFT_Long_F_1Long,rpn_long_f_1long,labs);
        SMA("lmax",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_max);
        SMA("lmin",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_min);
        SMA("d+",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_add);
        SMA("d++",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_inc);
        SMA("d-",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_sub);
        SMA("d-m",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_neg);
        SMA("d--",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_dec);
        SMA("d*",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_mul);
        SMA("d/",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_div);
        SMA("d%",RPNFT_Double_F_2Double,rpn_double_f_2double,fmod);
        SMA("d==",RPNFT_Cmp_D,rpn_cmp_d,RPN_d_eq);
        SMA("d!=",RPNFT_Cmp_D,rpn_cmp_d,RPN_d_neq);
        SMA("d>",RPNFT_Cmp_D,rpn_cmp_d,RPN_d_gt);
        SMA("d<",RPNFT_Cmp_D,rpn_cmp_d,RPN_d_lt);
        SMA("d>=",RPNFT_Cmp_D,rpn_cmp_d,RPN_d_gte);
        SMA("d<=",RPNFT_Cmp_D,rpn_cmp_d,RPN_d_lte);
        SMA("dabs",RPNFT_Double_F_1Double,rpn_double_f_1double,fabs);
        SMA("dmax",RPNFT_Double_F_2Double,rpn_double_f_2double,fmax);
        SMA("dmin",RPNFT_Double_F_2Double,rpn_double_f_2double,fmin);
        //Only long without prefix
        SMA("random_l",RPNFT_Long_F_NoArg,rpn_long_f_noarg,random_l);
        //Everything else without prefixes (and double type)
        SMA("exp",RPNFT_Double_F_1Double,rpn_double_f_1double,exp);
        SMA("exp2",RPNFT_Double_F_1Double,rpn_double_f_1double,exp2);
        SMA("log",RPNFT_Double_F_1Double,rpn_double_f_1double,log);
        SMA("log2",RPNFT_Double_F_1Double,rpn_double_f_1double,log2);
        SMA("log10",RPNFT_Double_F_1Double,rpn_double_f_1double,log10);
        SMA("pow",RPNFT_Double_F_2Double,rpn_double_f_2double,pow);
        SMA("sqrt",RPNFT_Double_F_1Double,rpn_double_f_1double,sqrt);
        SMA("cbrt",RPNFT_Double_F_1Double,rpn_double_f_1double,cbrt);
        SMA("hypot",RPNFT_Double_F_2Double,rpn_double_f_2double,hypot);
        SMA("sin",RPNFT_Double_F_1Double,rpn_double_f_1double,sinw);
        SMA("cos",RPNFT_Double_F_1Double,rpn_double_f_1double,cosw);
        SMA("tan",RPNFT_Double_F_1Double,rpn_double_f_1double,tanw);
        SMA("ceil",RPNFT_Double_F_1Double,rpn_double_f_1double,ceil);
        SMA("floor",RPNFT_Double_F_1Double,rpn_double_f_1double,floor);
        SMA("round",RPNFT_Double_F_1Double,rpn_double_f_1double,round);
        SMA("trunc",RPNFT_Double_F_1Double,rpn_double_f_1double,trunc);
        SMA("random_d",RPNFT_Double_F_NoArg,rpn_double_f_noarg,random_d);
        SMA("as_l",RPNFT_CastAsL,rpn_castas_l,castas_l);
        SMA("as_d",RPNFT_CastAsD,rpn_castas_d,castas_d);
    }else{
        puts("RPNEvaluatorInit has already been initialized.");
    }
}
#ifndef NDEBUG
#define RPNEvaluatorInitCalledFirst() \
if(!DefaultRPNMap){\
    fprintf(stderr,"RPNEvaluatorInit should be called first.\n");\
    exit(EXIT_FAILURE);\
} (void)0
#else
#define RPNEvaluatorInitCalledFirst() (void)0
#endif
//To prevent name collisions from functions and variables.
bool _RPNEvaluatorNameUsed(const char* variable){
    RPNEvaluatorInitCalledFirst();
    return StringMap_rpn_func_call_read(DefaultRPNMap,variable).exists;
}
RPNValidStringE RPNEvaluatorValidString(const char* rpn_str,VariableLoader_t* vl,const char* rpn_start_b,const char* rpn_end_b,char rpn_sep){
    RPNEvaluatorInitCalledFirst();
    const char* start_p,* end_p;
    int depth=first_outermost_bracket(rpn_str,rpn_start_b,rpn_end_b,&start_p,&end_p);
    if(!start_p||depth) return RPNVS_ImproperBrackets;
    char* rpn_str_no_b=char_string_slice_no_brackets(start_p,end_p,rpn_start_b);
    //TODO Stack for functions/variables.
    free(rpn_str_no_b);
    return RPNVS_Valid;
}
void RPNEvaluatorFree(){
    StringMap_rpn_func_call_free(DefaultRPNMap);
    DefaultRPNMap=0;
}