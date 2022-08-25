#include "rpn_evaluator.h"
#include <sys/random.h>
#include <limits.h>
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);
StringMap_ImplDef(rpn_func_call_t,rpn_func_call)
Stack_ImplDef(as_number_t,as_number)
StringMap_rpn_func_call_t* DefaultRPNFunctionMap=0;
#define BinOps(Type,Type_suf,f_name,o) Type RPN_##Type_suf##_##f_name(Type a,Type b){return a o b;}
#define BinCmp(Type,Type_suf,f_name,o) bool RPN_##Type_suf##_##f_name(Type a,Type b){return a o b;}
#define UnOps(Type,Type_suf,f_name,o) Type RPN_##Type_suf##_##f_name(Type a){return o a;}

BinOps(char,c,add,+) UnOps(char,c,inc,++)
BinOps(char,c,sub,-) UnOps(char,c,neg,-) UnOps(char,c,dec,--)
BinOps(char,c,mul,*)
BinOps(char,c,div,/)
BinOps(char,c,mod,%)
BinOps(char,c,b_and,&) BinOps(char,c,b_or,|) UnOps(char,c,b_not,~) BinOps(char,c,b_xor,^) BinOps(char,c,bs_l,<<) BinOps(char,c,bs_r,>>)
BinCmp(char,c,eq,==) BinCmp(char,c,neq,!=) BinCmp(char,c,lt,<) BinCmp(char,c,gt,>) BinCmp(char,c,lte,<=) BinCmp(char,c,gte,>=)
char RPN_c_max(char a,char b){return a>b?a:b;}
char RPN_c_min(char a,char b){return a<b?a:b;}
char _cabs(char a){return abs(a);}
char castas_c(char num){return num;}
char random_c(void){
    char rand_st=0;
    do{}while(getrandom(&rand_st,sizeof(rand_st),0)==-1);
    return rand_st;
}
char ternary_c(bool b,char x,char y){return b?x:y;}

BinOps(int,i,add,+) UnOps(int,i,inc,++)
BinOps(int,i,sub,-) UnOps(int,i,neg,-) UnOps(int,i,dec,--)
BinOps(int,i,mul,*)
BinOps(int,i,div,/)
BinOps(int,i,mod,%)
BinOps(int,i,b_and,&) BinOps(int,i,b_or,|) UnOps(int,i,b_not,~) BinOps(int,i,b_xor,^) BinOps(int,i,bs_l,<<) BinOps(int,i,bs_r,>>)
BinCmp(int,i,eq,==) BinCmp(int,i,neq,!=) BinCmp(int,i,lt,<) BinCmp(int,i,gt,>) BinCmp(int,i,lte,<=) BinCmp(int,i,gte,>=)
int RPN_i_max(int a,int b){return a>b?a:b;}
int RPN_i_min(int a,int b){return a<b?a:b;}
int castas_i(int num){return num;}
int random_i(void){
    int rand_st=0;
    do{}while(getrandom(&rand_st,sizeof(rand_st),0)==-1);
    return rand_st;
}
int ternary_i(bool b,int x,int y){return b?x:y;}

BinOps(long,l,add,+) UnOps(long,l,inc,++)
BinOps(long,l,sub,-) UnOps(long,l,neg,-) UnOps(long,l,dec,--)
BinOps(long,l,mul,*)
BinOps(long,l,div,/)
BinOps(long,l,mod,%)
BinOps(long,l,b_and,&) BinOps(long,l,b_or,|) UnOps(long,l,b_not,~) BinOps(long,l,b_xor,^) BinOps(long,l,bs_l,<<) BinOps(long,l,bs_r,>>)
BinCmp(long,l,eq,==) BinCmp(long,l,neq,!=) BinCmp(long,l,lt,<) BinCmp(long,l,gt,>) BinCmp(long,l,lte,<=) BinCmp(long,l,gte,>=)
long RPN_l_max(long a,long b){return a>b?a:b;}
long RPN_l_min(long a,long b){return a<b?a:b;}
long castas_l(long num){return num;}
long random_l(void){
    long rand_st=0;
    do{}while(getrandom(&rand_st,sizeof(rand_st),0)==-1);
    return rand_st;
}
long ternary_l(bool b,long x,long y){return b?x:y;}

BinOps(double,d,add,+) UnOps(double,d,inc,++)
BinOps(double,d,sub,-) UnOps(double,d,dec,--) UnOps(double,d,neg,-)
BinOps(double,d,mul,*)
BinOps(double,d,div,/)
bool RPN_d_eq(double a,double b){ return fabs(a-b)<=DBL_EPSILON; }
bool RPN_d_neq(double a,double b){ return fabs(a-b)>DBL_EPSILON; }
BinCmp(double,d,lt,<) BinCmp(double,d,gt,>) BinCmp(double,d,lte,<=) BinCmp(double,d,gte,>=)
#define __PI 3.141592653589793238
static inline double deg_to_rad(double deg){
    return deg*__PI/180.;
}
#define TrigDegWrap(name) double name##w(double deg){return name(deg_to_rad(deg));}
TrigDegWrap(sin) TrigDegWrap(cos) TrigDegWrap(tan)
double random_d(void){return (double)(size_t)random_l()/ULONG_MAX;}
double castas_d(double num){return num;}
double ternary_d(bool b,double x,double y){return b?x:y;}

bool bool_invert(bool b){return !b;}
void rpn_f_null(void){}
//To make the default StringMap before calling rpn_evaluator_new.
void RPNEvaluatorInit(void){
    if(!DefaultRPNFunctionMap){
        DefaultRPNFunctionMap=StringMap_rpn_func_call_new(199);//157 4
#define SMA(Str,RFT,RFT_u,F,NumArgs,ReturnType) assert(StringMap_rpn_func_call_assign(DefaultRPNFunctionMap,Str,(rpn_func_call_t){.type=RFT,.func.RFT_u=F,.num_args=NumArgs,.return_type=ReturnType})==VA_Written)
        SMA("abs",RPNFT_Null,rpn_null,rpn_f_null,0,VLNT_Invalid);//To check for "Existence" for any name collisions. Will not be calculated.
        SMA("max",RPNFT_Null,rpn_null,rpn_f_null,0,VLNT_Invalid);
        SMA("min",RPNFT_Null,rpn_null,rpn_f_null,0,VLNT_Invalid);

        SMA("__c+",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_add,2,VLNT_Char);
        SMA("__c++",RPNFT_Char_F_1Char,rpn_char_f_1char,RPN_c_inc,1,VLNT_Char);
        SMA("__c-",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_sub,2,VLNT_Char);
        SMA("__c-m",RPNFT_Char_F_1Char,rpn_char_f_1char,RPN_c_neg,1,VLNT_Char);
        SMA("__c--",RPNFT_Char_F_1Char,rpn_char_f_1char,RPN_c_dec,1,VLNT_Char);
        SMA("__c*",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_mul,2,VLNT_Char);
        SMA("__c/",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_div,2,VLNT_Char);
        SMA("__c%",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_mod,2,VLNT_Char);
        SMA("__c&",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_b_and,2,VLNT_Char);
        SMA("__c|",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_b_or,2,VLNT_Char);
        SMA("__c~",RPNFT_Char_F_1Char,rpn_char_f_1char,RPN_c_b_not,1,VLNT_Char);
        SMA("__c^",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_b_xor,2,VLNT_Char);
        SMA("__c<<",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_bs_l,2,VLNT_Char);
        SMA("__c>>",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_bs_r,2,VLNT_Char);
        SMA("__c==",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_eq,2,VLNT_Char);
        SMA("__c!=",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_neq,2,VLNT_Char);
        SMA("__c>",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_gt,2,VLNT_Char);
        SMA("__c<",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_lt,2,VLNT_Char);
        SMA("__c>=",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_gte,2,VLNT_Char);
        SMA("__c<=",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_lte,2,VLNT_Char);
        SMA("__cabs",RPNFT_Char_F_1Char,rpn_char_f_1char,_cabs,1,VLNT_Char);
        SMA("__cmax",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_max,2,VLNT_Char);
        SMA("__cmin",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_min,2,VLNT_Char);
        SMA("random_c",RPNFT_Char_F_NoArg,rpn_char_f_noarg,random_c,0,VLNT_Char);
        SMA("as_c",RPNFT_Char_F_1Char,rpn_char_f_1char,castas_c,1,VLNT_Char);
        SMA("__c?t:f",RPNFT_Char_F_Ternary,rpn_char_f_ternary,ternary_c,3,VLNT_Char);

        SMA("__i+",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_add,2,VLNT_Int);
        SMA("__i++",RPNFT_Int_F_1Int,rpn_int_f_1int,RPN_i_inc,1,VLNT_Int);
        SMA("__i-",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_sub,2,VLNT_Int);
        SMA("__i-m",RPNFT_Int_F_1Int,rpn_int_f_1int,RPN_i_neg,1,VLNT_Int);
        SMA("__i--",RPNFT_Int_F_1Int,rpn_int_f_1int,RPN_i_dec,1,VLNT_Int);
        SMA("__i*",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_mul,2,VLNT_Int);
        SMA("__i/",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_div,2,VLNT_Int);
        SMA("__i%",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_mod,2,VLNT_Int);
        SMA("__i&",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_b_and,2,VLNT_Int);
        SMA("__i|",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_b_or,2,VLNT_Int);
        SMA("__i~",RPNFT_Int_F_1Int,rpn_int_f_1int,RPN_i_b_not,1,VLNT_Int);
        SMA("__i^",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_b_xor,2,VLNT_Int);
        SMA("__i<<",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_bs_l,2,VLNT_Int);
        SMA("__i>>",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_bs_r,2,VLNT_Int);
        SMA("__i==",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_eq,2,VLNT_Int);
        SMA("__i!=",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_neq,2,VLNT_Int);
        SMA("__i>",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_gt,2,VLNT_Int);
        SMA("__i<",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_lt,2,VLNT_Int);
        SMA("__i>=",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_gte,2,VLNT_Int);
        SMA("__i<=",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_lte,2,VLNT_Int);
        SMA("__iabs",RPNFT_Int_F_1Int,rpn_int_f_1int,abs,1,VLNT_Int);
        SMA("__imax",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_max,2,VLNT_Int);
        SMA("__imin",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_min,2,VLNT_Int);
        SMA("random_i",RPNFT_Int_F_NoArg,rpn_int_f_noarg,random_i,0,VLNT_Int);
        SMA("as_i",RPNFT_Int_F_1Int,rpn_int_f_1int,castas_i,1,VLNT_Int);
        SMA("__i?t:f",RPNFT_Int_F_Ternary,rpn_int_f_ternary,ternary_i,3,VLNT_Int);

        SMA("__l+",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_add,2,VLNT_Long);
        SMA("__l++",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_inc,1,VLNT_Long);
        SMA("__l-",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_sub,2,VLNT_Long);
        SMA("__l-m",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_neg,1,VLNT_Long);
        SMA("__l--",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_dec,1,VLNT_Long);
        SMA("__l*",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_mul,2,VLNT_Long);
        SMA("__l/",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_div,2,VLNT_Long);
        SMA("__l%",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_mod,2,VLNT_Long);
        SMA("__l&",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_and,2,VLNT_Long);
        SMA("__l|",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_or,2,VLNT_Long);
        SMA("__l~",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_b_not,1,VLNT_Long);
        SMA("__l^",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_xor,2,VLNT_Long);
        SMA("__l<<",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_bs_l,2,VLNT_Long);
        SMA("__l>>",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_bs_r,2,VLNT_Long);
        SMA("__l==",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_eq,2,VLNT_Int);
        SMA("__l!=",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_neq,2,VLNT_Int);
        SMA("__l>",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_gt,2,VLNT_Int);
        SMA("__l<",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_lt,2,VLNT_Int);
        SMA("__l>=",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_gte,2,VLNT_Int);
        SMA("__l<=",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_lte,2,VLNT_Int);
        SMA("__labs",RPNFT_Long_F_1Long,rpn_long_f_1long,labs,1,VLNT_Long);
        SMA("__lmax",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_max,2,VLNT_Long);
        SMA("__lmin",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_min,2,VLNT_Long);
        SMA("random_l",RPNFT_Long_F_NoArg,rpn_long_f_noarg,random_l,0,VLNT_Long);
        SMA("as_l",RPNFT_Long_F_1Long,rpn_long_f_1long,castas_l,1,VLNT_Long);
        SMA("__l?t:f",RPNFT_Long_F_Ternary,rpn_long_f_ternary,ternary_l,3,VLNT_Long);
        
        SMA("__d+",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_add,2,VLNT_Double);
        SMA("__d++",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_inc,1,VLNT_Double);
        SMA("__d-",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_sub,2,VLNT_Double);
        SMA("__d-m",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_neg,1,VLNT_Double);
        SMA("__d--",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_dec,1,VLNT_Double);
        SMA("__d*",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_mul,2,VLNT_Double);
        SMA("__d/",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_div,2,VLNT_Double);
        SMA("__d%",RPNFT_Double_F_2Double,rpn_double_f_2double,fmod,2,VLNT_Double);
        SMA("__d==",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_eq,2,VLNT_Int);
        SMA("__d!=",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_neq,2,VLNT_Int);
        SMA("__d>",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_gt,2,VLNT_Int);
        SMA("__d<",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_lt,2,VLNT_Int);
        SMA("__d>=",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_gte,2,VLNT_Int);
        SMA("__d<=",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_lte,2,VLNT_Int);
        SMA("__dabs",RPNFT_Double_F_1Double,rpn_double_f_1double,fabs,1,VLNT_Double);
        SMA("__dmax",RPNFT_Double_F_2Double,rpn_double_f_2double,fmax,2,VLNT_Double);
        SMA("__dmin",RPNFT_Double_F_2Double,rpn_double_f_2double,fmin,2,VLNT_Double);
        SMA("random_d",RPNFT_Double_F_NoArg,rpn_double_f_noarg,random_d,0,VLNT_Double);
        SMA("exp",RPNFT_Double_F_1Double,rpn_double_f_1double,exp,1,VLNT_Double);
        SMA("exp2",RPNFT_Double_F_1Double,rpn_double_f_1double,exp2,1,VLNT_Double);
        SMA("log",RPNFT_Double_F_1Double,rpn_double_f_1double,log,1,VLNT_Double);
        SMA("log2",RPNFT_Double_F_1Double,rpn_double_f_1double,log2,1,VLNT_Double);
        SMA("log10",RPNFT_Double_F_1Double,rpn_double_f_1double,log10,1,VLNT_Double);
        SMA("pow",RPNFT_Double_F_2Double,rpn_double_f_2double,pow,2,VLNT_Double);
        SMA("sqrt",RPNFT_Double_F_1Double,rpn_double_f_1double,sqrt,1,VLNT_Double);
        SMA("cbrt",RPNFT_Double_F_1Double,rpn_double_f_1double,cbrt,1,VLNT_Double);
        SMA("hypot",RPNFT_Double_F_2Double,rpn_double_f_2double,hypot,2,VLNT_Double);
        SMA("sin",RPNFT_Double_F_1Double,rpn_double_f_1double,sinw,1,VLNT_Double);
        SMA("cos",RPNFT_Double_F_1Double,rpn_double_f_1double,cosw,1,VLNT_Double);
        SMA("tan",RPNFT_Double_F_1Double,rpn_double_f_1double,tanw,1,VLNT_Double);
        SMA("ceil",RPNFT_Double_F_1Double,rpn_double_f_1double,ceil,1,VLNT_Double);
        SMA("floor",RPNFT_Double_F_1Double,rpn_double_f_1double,floor,1,VLNT_Double);
        SMA("round",RPNFT_Double_F_1Double,rpn_double_f_1double,round,1,VLNT_Double);
        SMA("trunc",RPNFT_Double_F_1Double,rpn_double_f_1double,trunc,1,VLNT_Double);
        SMA("as_d",RPNFT_Double_F_1Double,rpn_double_f_1double,castas_d,1,VLNT_Double);
        SMA("__d?t:f",RPNFT_Double_F_Ternary,rpn_double_f_ternary,ternary_d,3,VLNT_Double);

        SMA("!",RPNFT_Invert,rpn_invert,bool_invert,1,VLNT_Int);
        StringMap_rpn_func_call_print_debug(DefaultRPNFunctionMap);
    }else{
        puts("RPNEvaluatorInit has already been initialized.");
    }
}
#ifndef NDEBUG
#define RPNEvaluatorInitCalledFirst() \
if(!DefaultRPNFunctionMap){\
    fprintf(stderr,"RPNEvaluatorInit should be called first.\n");\
    exit(EXIT_FAILURE);\
} (void)0
#else
#define RPNEvaluatorInitCalledFirst() (void)0
#endif
as_number_opt_t _RPNEvaluatorIsNumber(const char* token);
RPNValidStringE _RPNEvaluatorIsVarNameOk(const char* token,const VariableLoader_t* vl,Stack_as_number_t* stack_an);
bool _ProcessRPNFunctionCall(Stack_as_number_t* stack_an,const rpn_func_call_t* rpn_f_c);
VLNumberType _Stack_get_highest_number_type(const Stack_as_number_t* this,int num_args);
void _Stack_as_number_print(const Stack_as_number_t* this);
RPNValidStringE RPNEvaluatorValidString(const char* rpn_str,const VariableLoader_t* vl,const char* rpn_start_b,const char* rpn_end_b,char rpn_sep){
    RPNEvaluatorInitCalledFirst();
    const char* start_p,* end_p;
    int depth=first_outermost_bracket(rpn_str,rpn_start_b,rpn_end_b,&start_p,&end_p);
    if(!start_p||depth) return RPNVS_ImproperBrackets;
    char* rpn_str_no_b=char_string_slice_no_brackets(start_p,end_p,rpn_start_b);
    Stack_as_number_t* stack_an=Stack_as_number_new();
    const char* token_b_p=rpn_str_no_b,* token_e_p=rpn_str_no_b;
    while(true){
        token_e_p=strchr(token_b_p,rpn_sep);
        char* current_token;
        as_number_opt_t an_opt;
        RPNValidStringE status;
        _Stack_as_number_print(stack_an);
        if(token_e_p){
            current_token=char_string_slice(token_b_p,token_e_p-1);
            puts(current_token);
            if((an_opt=_RPNEvaluatorIsNumber(current_token)).exists) Stack_as_number_push(stack_an,an_opt.v);
            else if((status=_RPNEvaluatorIsVarNameOk(current_token,vl,stack_an))!=RPNVS_IsVLName&&status!=RPNVS_IsFunction){
                free(current_token);
                free(rpn_str_no_b);
                Stack_as_number_free(stack_an);
                return status;
            }else if(status==RPNVS_IsVLName) Stack_as_number_push(stack_an,VL_get_as_number(vl,current_token).value);
            free(current_token);
            token_b_p=token_e_p+1;//Next token in strchr.
            continue;
        }//Token_b_p has the last token.
        current_token=malloc(sizeof(char)*(strlen(token_b_p)+1));
        EXIT_IF_NULL(current_token,char*);
        strcpy(current_token,token_b_p);
        puts(current_token);
        if((an_opt=_RPNEvaluatorIsNumber(current_token)).exists) Stack_as_number_push(stack_an,an_opt.v);
        else if((status=_RPNEvaluatorIsVarNameOk(current_token,vl,stack_an))!=RPNVS_IsVLName&&status!=RPNVS_IsFunction){
            free(current_token);
            Stack_as_number_free(stack_an);
            free(rpn_str_no_b);
            return status;
        }else if(status==RPNVS_IsVLName) Stack_as_number_push(stack_an,VL_get_as_number(vl,current_token).value);
        free(current_token);
        break;
    }
    _Stack_as_number_print(stack_an);
    int stack_size_now=stack_an->size;
    Stack_as_number_free(stack_an);
    free(rpn_str_no_b);
    return (stack_size_now==1)?RPNVS_Ok:RPNVS_TooManyNumbers;
}
void RPNEvaluatorFree(void){
    StringMap_rpn_func_call_free(DefaultRPNFunctionMap);
    DefaultRPNFunctionMap=0;
}
as_number_opt_t _RPNEvaluatorIsNumber(const char* token){
    VLNumberType vlnt=VLNT_Long;//Default
    for(size_t i=0;i<strlen(token);i++){
        const char current_char=token[i];
        if(isdigit(current_char)) continue;
        if(current_char=='.'){
            if(vlnt==VLNT_Double) return (as_number_opt_t){0};//Only one dot.
            if(vlnt!=VLNT_Double) vlnt=VLNT_Double;
            continue;
        }
        if(vlnt!=VLNT_Double&&i==strlen(token)-1){
            switch(current_char){
                case 'c': vlnt=VLNT_Char; goto valid_num;
                case 'i': vlnt=VLNT_Int; goto valid_num;
                case 'l': vlnt=VLNT_Long; goto valid_num;
                case 'd': vlnt=VLNT_Double; goto valid_num;
                default: return (as_number_opt_t){0};
            }
        }
        return (as_number_opt_t){0};
    }
    valid_num:
    switch(vlnt){ 
        case VLNT_Char: return (as_number_opt_t){.exists=true,.v=(as_number_t){.type=vlnt,.c=(char)strtol(token,0,10)}};
        case VLNT_Int: return (as_number_opt_t){.exists=true,.v=(as_number_t){.type=vlnt,.i=(int)strtol(token,0,10)}};
        case VLNT_Long: return (as_number_opt_t){.exists=true,.v=(as_number_t){.type=vlnt,.l=strtol(token,0,10)}};
        case VLNT_Double: return (as_number_opt_t){.exists=true,.v=(as_number_t){.type=vlnt,.d=strtod(token,0)}};
        default: return (as_number_opt_t){0};//Should be unreachable.
    }
}
//Check if VariableLoader names doesn't share any names in DefaultRPNFunctionMap.
RPNValidStringE _RPNEvaluatorIsVarNameOk(const char* token,const VariableLoader_t* vl,Stack_as_number_t* stack_an){
    static const char NumberTypePrefixes[5]={'\0','c','i','l','d'};//Arranged based on the VLNumberType enums.
    bool token_is_var=VL_get_as_number(vl,token).exists;
    //Keep continuing code as if it were a valid string (pop numbers) until an RPNVS_NameCollision.
    StringMapOpt_rpn_func_call_t smorfc_noprf={0},smorfc_wprf={0};
    if((smorfc_noprf=StringMap_rpn_func_call_read(DefaultRPNFunctionMap,token)).exists)
        if(token_is_var) return RPNVS_NameCollision;
    char* token_with_prefix=malloc(sizeof(char)*(strlen(token)+4));//+4 for "__(char)" and '\0'
    EXIT_IF_NULL(token_with_prefix,char*);
    token_with_prefix[0]='_';
    token_with_prefix[1]='_';//Special functions of "__(d/l/i/c)(function name)"
    strcpy(token_with_prefix+3,token);
    StringMapOpt_rpn_func_call_t rpn_f_c_array[5]={0};
    int rpn_f_num_args=0;
    for(int i=1;i<5;i++){//Search for special functions of double/long/int/char.
        StringMapOpt_rpn_func_call_t prefix_smorfc;
        token_with_prefix[2]=NumberTypePrefixes[i];
        if((prefix_smorfc=StringMap_rpn_func_call_read(DefaultRPNFunctionMap,token_with_prefix)).exists){
            if(token_is_var){
                free(token_with_prefix);
                return RPNVS_NameCollision;
            }
            rpn_f_c_array[i]=prefix_smorfc;
            rpn_f_num_args=prefix_smorfc.value.num_args;//To promote any variables to use the proper function.
        }
    }
    free(token_with_prefix);
    if(rpn_f_num_args) smorfc_wprf=rpn_f_c_array[_Stack_get_highest_number_type(stack_an,rpn_f_num_args)];
    if(smorfc_noprf.exists&&smorfc_noprf.value.type!=RPNFT_Null){//Don't process if RPNFT_Null for any name collisions.
        if(!_ProcessRPNFunctionCall(stack_an,&smorfc_noprf.value)) return RPNVS_OutOfNumbers;
    }else if(smorfc_wprf.exists){
        if(!_ProcessRPNFunctionCall(stack_an,&smorfc_wprf.value)) return RPNVS_OutOfNumbers;
    }else{
        return token_is_var?RPNVS_IsVLName:RPNVS_NameNotAdded;//If not VLVar, then NameNotAdded since no match for token.
    }
    return token_is_var?RPNVS_IsVLName:RPNVS_IsFunction;
}
//Bool if out of numbers. TODO: Process possible functions.
bool _ProcessRPNFunctionCall(Stack_as_number_t* stack_an,const rpn_func_call_t* rpn_f_c){
    StackOpt_as_number_t* args=malloc(sizeof(StackOpt_as_number_t)*rpn_f_c->num_args);
    as_number_t result;
    EXIT_IF_NULL(args,StackOpt_as_number_t*);
    for(int i=rpn_f_c->num_args-1;i>=0;i--){
        if(!(args[i]=Stack_as_number_pop(stack_an)).exists){
            free(args);
            return false;
        }
    }
    const rpn_function_u rpn_fu=rpn_f_c->func;
    result.type=rpn_f_c->return_type;
#define ARGS_CAST(I) ((args[I].v.type==VLNT_Double)?args[I].v.d:\
    (args[I].v.type==VLNT_Long)?args[I].v.l:\
    (args[I].v.type==VLNT_Int)?args[I].v.i:args[I].v.c)
    switch(rpn_f_c->type){
        case RPNFT_Char_F_NoArg: result.c=rpn_fu.rpn_char_f_noarg(); break;
        case RPNFT_Char_F_1Char: result.c=rpn_fu.rpn_char_f_1char(ARGS_CAST(0)); break;
        case RPNFT_Char_F_2Char: result.c=rpn_fu.rpn_char_f_2char(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Char_F_Cmp: result.c=rpn_fu.rpn_char_f_cmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Char_F_Ternary: result.l=rpn_fu.rpn_char_f_ternary(ARGS_CAST(0),ARGS_CAST(1),ARGS_CAST(2)); break;
        case RPNFT_Int_F_NoArg: result.i=rpn_fu.rpn_int_f_noarg(); break;
        case RPNFT_Int_F_1Int: result.i=rpn_fu.rpn_int_f_1int(ARGS_CAST(0)); break;
        case RPNFT_Int_F_2Int: result.i=rpn_fu.rpn_int_f_2int(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Int_F_Cmp: result.i=rpn_fu.rpn_int_f_cmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Int_F_Ternary: result.l=rpn_fu.rpn_int_f_ternary(ARGS_CAST(0),ARGS_CAST(1),ARGS_CAST(2)); break;
        case RPNFT_Long_F_NoArg: result.l=rpn_fu.rpn_long_f_noarg(); break;
        case RPNFT_Long_F_1Long: result.l=rpn_fu.rpn_long_f_1long(ARGS_CAST(0)); break;
        case RPNFT_Long_F_2Long: result.l=rpn_fu.rpn_long_f_2long(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Long_F_Cmp: result.i=rpn_fu.rpn_long_f_cmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Long_F_Ternary: result.l=rpn_fu.rpn_long_f_ternary(ARGS_CAST(0),ARGS_CAST(1),ARGS_CAST(2)); break;
        case RPNFT_Double_F_NoArg: result.d=rpn_fu.rpn_double_f_noarg(); break;
        case RPNFT_Double_F_1Double: result.d=rpn_fu.rpn_double_f_1double(ARGS_CAST(0)); break;
        case RPNFT_Double_F_2Double: result.d=rpn_fu.rpn_double_f_2double(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Double_F_Ternary: result.d=rpn_fu.rpn_double_f_ternary(ARGS_CAST(0),ARGS_CAST(1),ARGS_CAST(2)); break;
        case RPNFT_Double_F_Cmp: result.i=rpn_fu.rpn_double_f_cmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Invert: result.i=rpn_fu.rpn_invert(ARGS_CAST(0)); break;
        case RPNFT_Null: free(args); return true; break; //Shouldn't be accessed.
    }
    Stack_as_number_push(stack_an,result);
    free(args);
    return true;
}
VLNumberType _Stack_get_highest_number_type(const Stack_as_number_t* this,int num_args){
    VLNumberType highest_nt=VLNT_Invalid;
    for(int i=0;i<num_args;i++){
        int peek_index;
        if((peek_index=this->size-1-i)>=0){
            highest_nt=(highest_nt>this->stack[peek_index].type)?highest_nt:this->stack[peek_index].type;
            continue;
        }
        break;
    }
    return highest_nt;
}
void _Stack_as_number_print(const Stack_as_number_t* this){
    printf("[");
    for(int i=0;i<this->size;i++){
        as_number_t an;
        switch((an=this->stack[i]).type){
            case VLNT_Char: printf("%dc",an.c); break;
            case VLNT_Int: printf("%di",an.i); break;
            case VLNT_Long: printf("%ldl",an.l); break;
            case VLNT_Double: printf("%lfd",an.d); break;
            default: printf("NaN"); break;//Shouldn't reach here.
        }
        fputs(i!=this->size-1?", ":"",stdout);
    }
    printf("]\n");
}