#include "rpn_evaluator.h"
#include <sys/random.h>
#include <limits.h>
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);
StringMap_ImplDef(rpn_func_call_t,rpn_func_call)
Stack_ImplDef(as_number_t,as_number)
StringMap_rpn_func_call_t* DefaultRPNFunctionMap=0;
StringMap_as_number_t* DefaultRPNVariablesMap=0;
bool _DividedByZero=false;
bool _BitShiftNegative=false;
#define BinOps(Type,Type_suf,f_name,o) Type RPN_##Type_suf##_##f_name(Type a,Type b){return a o b;}
//For '/' and '%'
#define BinOpsDiv(Type,Type_suf,f_name,o) Type RPN_##Type_suf##_##f_name(Type a,Type b){if(!b){ _DividedByZero=true; return 0; }return a o b;}
#define BinOpsUDiv(SignedType,Type_suf,f_name,o) SignedType RPN_##Type_suf##_##f_name(unsigned SignedType a,unsigned SignedType b){if(!b){ _DividedByZero=true; return 0; }return a o b;}
#define BinOpsShift(Type,Type_suf,f_name,o) Type RPN_##Type_suf##_##f_name(Type a,Type b){if(b<0){ _BitShiftNegative=true; return 0; }return a o b;}
#define BinOpsUShift(SignedType,Type_suf,f_name,o) SignedType RPN_##Type_suf##_##f_name(unsigned SignedType a,unsigned SignedType b){if((SignedType)b<(SignedType)0){ _BitShiftNegative=true; return 0; }return a o b;}
#define BinCmp(Type,Type_suf,f_name,o) bool RPN_##Type_suf##_##f_name(Type a,Type b){return a o b;}
#define UnOps(Type,Type_suf,f_name,o) Type RPN_##Type_suf##_##f_name(Type a){return o a;}
BinOps(char,c,add,+) UnOps(char,c,inc,++)
BinOps(char,c,sub,-) UnOps(char,c,neg,-) UnOps(char,c,dec,--)
BinOps(char,c,mul,*)
BinOpsDiv(char,c,div,/)
BinOpsDiv(char,c,mod,%)
BinOps(char,c,b_and,&) BinOps(char,c,b_or,|) UnOps(char,c,b_not,~) BinOps(char,c,b_xor,^) BinOpsShift(char,c,bs_l,<<) BinOpsShift(char,c,bs_r,>>)
BinCmp(char,c,eq,==) BinCmp(char,c,neq,!=) BinCmp(char,c,lt,<) BinCmp(char,c,gt,>) BinCmp(char,c,lte,<=) BinCmp(char,c,gte,>=)
BinCmp(unsigned char,uc,eq,==) BinCmp(unsigned char,uc,neq,!=) BinCmp(unsigned char,uc,lt,<) BinCmp(unsigned char,uc,gt,>) BinCmp(unsigned char,uc,lte,<=) BinCmp(unsigned char,uc,gte,>=)
char RPN_c_max(char a,char b){return a>b?a:b;}
char RPN_c_min(char a,char b){return a<b?a:b;}
char RPN_uc_max(unsigned char a,unsigned char b){return a>b?a:b;}
char RPN_uc_min(unsigned char a,unsigned char b){return a<b?a:b;}
char _cabs(char a){return abs(a);}
char castas_c(char num){return num;}
char random_c(void){
    char rand_st=0;
    do{}while(getrandom(&rand_st,sizeof(rand_st),0)==-1);
    return rand_st;
}
char ternary_c(bool b,char x,char y){return b?x:y;}
BinOpsUDiv(char,uc,div,/)
BinOpsUDiv(char,uc,mod,%)
BinOpsUShift(char,uc,bs_l,<<)
BinOpsUShift(char,uc,bs_r,>>)

BinOps(int,i,add,+) UnOps(int,i,inc,++)
BinOps(int,i,sub,-) UnOps(int,i,neg,-) UnOps(int,i,dec,--)
BinOps(int,i,mul,*)
BinOpsDiv(int,i,div,/)
BinOpsDiv(int,i,mod,%)
BinOps(int,i,b_and,&) BinOps(int,i,b_or,|) UnOps(int,i,b_not,~) BinOps(int,i,b_xor,^) BinOpsShift(int,i,bs_l,<<) BinOpsShift(int,i,bs_r,>>)
BinCmp(int,i,eq,==) BinCmp(int,i,neq,!=) BinCmp(int,i,lt,<) BinCmp(int,i,gt,>) BinCmp(int,i,lte,<=) BinCmp(int,i,gte,>=)
BinCmp(unsigned int,ui,eq,==) BinCmp(unsigned int,ui,neq,!=) BinCmp(unsigned int,ui,lt,<) BinCmp(unsigned int,ui,gt,>) BinCmp(unsigned int,ui,lte,<=) BinCmp(unsigned int,ui,gte,>=)
int RPN_i_max(int a,int b){return a>b?a:b;}
int RPN_i_min(int a,int b){return a<b?a:b;}
int RPN_ui_max(unsigned int a,unsigned int b){return a>b?a:b;}
int RPN_ui_min(unsigned int a,unsigned int b){return a<b?a:b;}
int castas_i(int num){return num;}
int random_i(void){
    int rand_st=0;
    do{}while(getrandom(&rand_st,sizeof(rand_st),0)==-1);
    return rand_st;
}
int ternary_i(bool b,int x,int y){return b?x:y;}
BinOpsUDiv(int,ui,div,/)
BinOpsUDiv(int,ui,mod,%)
BinOpsUShift(int,ui,bs_l,<<)
BinOpsUShift(int,ui,bs_r,>>)

BinOps(long,l,add,+) UnOps(long,l,inc,++)
BinOps(long,l,sub,-) UnOps(long,l,neg,-) UnOps(long,l,dec,--)
BinOps(long,l,mul,*)
BinOpsDiv(long,l,div,/)
BinOpsDiv(long,l,mod,%)
BinOps(long,l,b_and,&) BinOps(long,l,b_or,|) UnOps(long,l,b_not,~) BinOps(long,l,b_xor,^) BinOpsShift(long,l,bs_l,<<) BinOpsShift(long,l,bs_r,>>)
BinCmp(long,l,eq,==) BinCmp(long,l,neq,!=) BinCmp(long,l,lt,<) BinCmp(long,l,gt,>) BinCmp(long,l,lte,<=) BinCmp(long,l,gte,>=)
BinCmp(unsigned long,ul,eq,==) BinCmp(unsigned long,ul,neq,!=) BinCmp(unsigned long,ul,lt,<) BinCmp(unsigned long,ul,gt,>) BinCmp(unsigned long,ul,lte,<=) BinCmp(unsigned long,ul,gte,>=)
long RPN_l_max(long a,long b){return a>b?a:b;}
long RPN_l_min(long a,long b){return a<b?a:b;}
long RPN_ul_max(unsigned long a,unsigned long b){return a>b?a:b;}
long RPN_ul_min(unsigned long a,unsigned long b){return a<b?a:b;}
long castas_l(long num){return num;}
long random_l(void){
    long rand_st=0;
    do{}while(getrandom(&rand_st,sizeof(rand_st),0)==-1);
    return rand_st;
}
long ternary_l(bool b,long x,long y){return b?x:y;}
BinOpsUDiv(long,ul,div,/)
BinOpsUDiv(long,ul,mod,%)
BinOpsUShift(long,ul,bs_l,<<)
BinOpsUShift(long,ul,bs_r,>>)

BinOps(double,d,add,+) UnOps(double,d,inc,++)
BinOps(double,d,sub,-) UnOps(double,d,dec,--) UnOps(double,d,neg,-)
BinOps(double,d,mul,*)
BinOps(double,d,div,/)
bool RPN_d_eq(double a,double b){ return fabs(a-b)<=DBL_EPSILON; }
bool RPN_d_neq(double a,double b){ return fabs(a-b)>DBL_EPSILON; }
BinCmp(double,d,lt,<) BinCmp(double,d,gt,>) BinCmp(double,d,lte,<=) BinCmp(double,d,gte,>=)//No unsigned double comparisons. 
#define __PI 3.141592653589793238
static inline double deg_to_rad(double deg){
    return deg*__PI/180.;
}
static inline double rad_to_deg(double rad){
    return (rad*180)/__PI;
}
#define TrigDegWrap(name) double name##d(double deg){return name(deg_to_rad(deg));}
#define ArcTrigDegWrap(name) double name##d(double v){return rad_to_deg(name(v));}
TrigDegWrap(sin) TrigDegWrap(cos) TrigDegWrap(tan)
ArcTrigDegWrap(asin) ArcTrigDegWrap(acos) ArcTrigDegWrap(atan)
double random_d(void){return (double)((size_t)random_l()/ULONG_MAX);}
double castas_d(double num){return num;}
double ternary_d(bool b,double x,double y){return b?x:y;}

bool bool_invert(bool b){return !b;}
bool bool_and(bool b1,bool b2){return b1&&b2;}
bool bool_or(bool b1,bool b2){return b1||b2;}
void rpn_f_null(void){}
//To make the default StringMap before calling rpn_evaluator_new.
void RPNEvaluatorInit(void){
    if(!DefaultRPNFunctionMap){
        DefaultRPNVariablesMap=StringMap_as_number_new(15);
        ValueAssignE status;
        bool is_ph=true;
#define SMVA_DEBUG 0
#if SMVA_DEBUG==1
    #define SMVA(Str,NumberType,NumberMem,InitVar)\
    status=StringMap_as_number_assign_ph(DefaultRPNVariablesMap,Str,(as_number_t){.NumberMem=InitVar,.type=NumberType});\
    printf("Placing token: "  Str ": ");\
    puts((status==VA_Written)?"OK":ERR("Collision has Occured! Not a Perfect Hash."));\
    if(is_ph) is_ph=(status==VA_Written);\
    (void)0
#else
    #define SMVA(Str,NumberType,NumberMem,InitVar)\
    status=StringMap_as_number_assign_ph(DefaultRPNVariablesMap,Str,(as_number_t){.NumberMem=InitVar,.type=NumberType});\
    if(is_ph) is_ph=(status==VA_Written);\
    (void)0
#endif
        SMVA("@mma_x",VLNT_Int,i,0);//MouseMoveAbsolute x when load_mma is used.
        SMVA("@mma_y",VLNT_Int,i,0);
        SMVA("@ci_now",VLNT_Int,i,1);//Prints the program counter.
        SMVA("@ci_prev",VLNT_Int,i,1);
        SMVA("@ci_last",VLNT_Int,i,1);//Last command number.
        SMVA("@time_s",VLNT_Long,l,0);//Time since start of program.
        SMVA("@time_ns",VLNT_Long,l,0);
#undef SMVA
#if SMVA_DEBUG==1
        StringMap_as_number_print_debug(DefaultRPNVariablesMap);
#endif
        if(!is_ph){
            fprintf(stderr,ERR("DefaultRPNVariablesMap should be a perfect hash. Exiting program.\n"));
            exit(EXIT_FAILURE);
        }
        DefaultRPNFunctionMap=StringMap_rpn_func_call_new(228);
        //Current size: 167
        //228 is [[0]=87,[1]=62,[2]=13,[3]=2,[4]=2,[5]=1] with djb hash
        //210 is [[0]=68,[1]=59,[2]=26,[3]=8,[4]=5,[5]=1]
#define SMFA(Str,RFT,RFT_u,F,NumArgs,ReturnType) assert(StringMap_rpn_func_call_assign(DefaultRPNFunctionMap,Str,(rpn_func_call_t){.type=RFT,.func.RFT_u=F,.num_args=NumArgs,.return_type=ReturnType})==VA_Written)
        SMFA("abs",RPNFT_Null,rpn_null,rpn_f_null,0,VLNT_Invalid);//To check for "Existence" for any name collisions for char/int/long/double. Will not be calculated.
        SMFA("max",RPNFT_Null,rpn_null,rpn_f_null,0,VLNT_Invalid);
        SMFA("min",RPNFT_Null,rpn_null,rpn_f_null,0,VLNT_Invalid);
        SMFA("maxu",RPNFT_Null,rpn_null,rpn_f_null,0,VLNT_Invalid);
        SMFA("minu",RPNFT_Null,rpn_null,rpn_f_null,0,VLNT_Invalid);

        SMFA("__c+",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_add,2,VLNT_Char);
        SMFA("__c++",RPNFT_Char_F_1Char,rpn_char_f_1char,RPN_c_inc,1,VLNT_Char);
        SMFA("__c-",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_sub,2,VLNT_Char);
        SMFA("__c-m",RPNFT_Char_F_1Char,rpn_char_f_1char,RPN_c_neg,1,VLNT_Char);
        SMFA("__c--",RPNFT_Char_F_1Char,rpn_char_f_1char,RPN_c_dec,1,VLNT_Char);
        SMFA("__c*",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_mul,2,VLNT_Char);
        SMFA("__c/",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_div,2,VLNT_Char);
        SMFA("__c/u",RPNFT_Char_F_2UChar,rpn_char_f_2uchar,RPN_uc_div,2,VLNT_Char);
        SMFA("__c%",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_mod,2,VLNT_Char);
        SMFA("__c%u",RPNFT_Char_F_2UChar,rpn_char_f_2uchar,RPN_uc_mod,2,VLNT_Char);
        SMFA("__c&",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_b_and,2,VLNT_Char);
        SMFA("__c|",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_b_or,2,VLNT_Char);
        SMFA("__c~",RPNFT_Char_F_1Char,rpn_char_f_1char,RPN_c_b_not,1,VLNT_Char);
        SMFA("__c^",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_b_xor,2,VLNT_Char);
        SMFA("__c<<",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_bs_l,2,VLNT_Char);
        SMFA("__c<<u",RPNFT_Char_F_2UChar,rpn_char_f_2uchar,RPN_uc_bs_l,2,VLNT_Char);
        SMFA("__c>>",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_bs_r,2,VLNT_Char);
        SMFA("__c>>u",RPNFT_Char_F_2UChar,rpn_char_f_2uchar,RPN_uc_bs_r,2,VLNT_Char);
        SMFA("__c==",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_eq,2,VLNT_Int);//Int as boolean.
        SMFA("__c!=",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_neq,2,VLNT_Int);
        SMFA("__c>",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_gt,2,VLNT_Int);
        SMFA("__c<",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_lt,2,VLNT_Int);
        SMFA("__c>=",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_gte,2,VLNT_Int);
        SMFA("__c<=",RPNFT_Char_F_Cmp,rpn_char_f_cmp,RPN_c_lte,2,VLNT_Int);
        SMFA("__c==u",RPNFT_Char_F_UCmp,rpn_char_f_ucmp,RPN_uc_eq,2,VLNT_Int);
        SMFA("__c!=u",RPNFT_Char_F_UCmp,rpn_char_f_ucmp,RPN_uc_neq,2,VLNT_Int);
        SMFA("__c>u",RPNFT_Char_F_UCmp,rpn_char_f_ucmp,RPN_uc_gt,2,VLNT_Int);
        SMFA("__c<u",RPNFT_Char_F_UCmp,rpn_char_f_ucmp,RPN_uc_lt,2,VLNT_Int);
        SMFA("__c>=u",RPNFT_Char_F_UCmp,rpn_char_f_ucmp,RPN_uc_gte,2,VLNT_Int);
        SMFA("__c<=u",RPNFT_Char_F_UCmp,rpn_char_f_ucmp,RPN_uc_lte,2,VLNT_Int);
        SMFA("__cabs",RPNFT_Char_F_1Char,rpn_char_f_1char,_cabs,1,VLNT_Char);
        SMFA("__cmax",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_max,2,VLNT_Char);
        SMFA("__cmin",RPNFT_Char_F_2Char,rpn_char_f_2char,RPN_c_min,2,VLNT_Char);
        SMFA("__cmaxu",RPNFT_Char_F_2UChar,rpn_char_f_2uchar,RPN_uc_max,2,VLNT_Char);
        SMFA("__cminu",RPNFT_Char_F_2UChar,rpn_char_f_2uchar,RPN_uc_min,2,VLNT_Char);
        SMFA("random_c",RPNFT_Char_F_NoArg,rpn_char_f_noarg,random_c,0,VLNT_Char);
        SMFA("as_c",RPNFT_Char_F_1Char,rpn_char_f_1char,castas_c,1,VLNT_Char);
        SMFA("__cb?t:f",RPNFT_Char_F_Ternary,rpn_char_f_ternary,ternary_c,3,VLNT_Char);

        SMFA("__i+",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_add,2,VLNT_Int);
        SMFA("__i++",RPNFT_Int_F_1Int,rpn_int_f_1int,RPN_i_inc,1,VLNT_Int);
        SMFA("__i-",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_sub,2,VLNT_Int);
        SMFA("__i-m",RPNFT_Int_F_1Int,rpn_int_f_1int,RPN_i_neg,1,VLNT_Int);
        SMFA("__i--",RPNFT_Int_F_1Int,rpn_int_f_1int,RPN_i_dec,1,VLNT_Int);
        SMFA("__i*",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_mul,2,VLNT_Int);
        SMFA("__i/",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_div,2,VLNT_Int);
        SMFA("__i/u",RPNFT_Int_F_2UInt,rpn_int_f_2uint,RPN_ui_div,2,VLNT_Int);
        SMFA("__i%",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_mod,2,VLNT_Int);
        SMFA("__i%u",RPNFT_Int_F_2UInt,rpn_int_f_2uint,RPN_ui_mod,2,VLNT_Int);
        SMFA("__i&",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_b_and,2,VLNT_Int);
        SMFA("__i|",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_b_or,2,VLNT_Int);
        SMFA("__i~",RPNFT_Int_F_1Int,rpn_int_f_1int,RPN_i_b_not,1,VLNT_Int);
        SMFA("__i^",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_b_xor,2,VLNT_Int);
        SMFA("__i<<",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_bs_l,2,VLNT_Int);
        SMFA("__i<<u",RPNFT_Int_F_2UInt,rpn_int_f_2uint,RPN_ui_bs_l,2,VLNT_Int);
        SMFA("__i>>",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_bs_r,2,VLNT_Int);
        SMFA("__i>>u",RPNFT_Int_F_2UInt,rpn_int_f_2uint,RPN_ui_bs_r,2,VLNT_Int);
        SMFA("__i==",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_eq,2,VLNT_Int);
        SMFA("__i!=",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_neq,2,VLNT_Int);
        SMFA("__i>",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_gt,2,VLNT_Int);
        SMFA("__i<",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_lt,2,VLNT_Int);
        SMFA("__i>=",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_gte,2,VLNT_Int);
        SMFA("__i<=",RPNFT_Int_F_Cmp,rpn_int_f_cmp,RPN_i_lte,2,VLNT_Int);
        SMFA("__i==u",RPNFT_Int_F_UCmp,rpn_int_f_ucmp,RPN_ui_eq,2,VLNT_Int);
        SMFA("__i!=u",RPNFT_Int_F_UCmp,rpn_int_f_ucmp,RPN_ui_neq,2,VLNT_Int);
        SMFA("__i>u",RPNFT_Int_F_UCmp,rpn_int_f_ucmp,RPN_ui_gt,2,VLNT_Int);
        SMFA("__i<u",RPNFT_Int_F_UCmp,rpn_int_f_ucmp,RPN_ui_lt,2,VLNT_Int);
        SMFA("__i>=u",RPNFT_Int_F_UCmp,rpn_int_f_ucmp,RPN_ui_gte,2,VLNT_Int);
        SMFA("__i<=u",RPNFT_Int_F_UCmp,rpn_int_f_ucmp,RPN_ui_lte,2,VLNT_Int);
        SMFA("__iabs",RPNFT_Int_F_1Int,rpn_int_f_1int,abs,1,VLNT_Int);
        SMFA("__imax",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_max,2,VLNT_Int);
        SMFA("__imin",RPNFT_Int_F_2Int,rpn_int_f_2int,RPN_i_min,2,VLNT_Int);
        SMFA("__imaxu",RPNFT_Int_F_2UInt,rpn_int_f_2uint,RPN_ui_max,2,VLNT_Int);
        SMFA("__iminu",RPNFT_Int_F_2UInt,rpn_int_f_2uint,RPN_ui_min,2,VLNT_Int);
        SMFA("random_i",RPNFT_Int_F_NoArg,rpn_int_f_noarg,random_i,0,VLNT_Int);
        SMFA("as_i",RPNFT_Int_F_1Int,rpn_int_f_1int,castas_i,1,VLNT_Int);
        SMFA("__ib?t:f",RPNFT_Int_F_Ternary,rpn_int_f_ternary,ternary_i,3,VLNT_Int);

        SMFA("__l+",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_add,2,VLNT_Long);
        SMFA("__l++",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_inc,1,VLNT_Long);
        SMFA("__l-",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_sub,2,VLNT_Long);
        SMFA("__l-m",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_neg,1,VLNT_Long);
        SMFA("__l--",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_dec,1,VLNT_Long);
        SMFA("__l*",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_mul,2,VLNT_Long);
        SMFA("__l/",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_div,2,VLNT_Long);
        SMFA("__l/u",RPNFT_Long_F_2ULong,rpn_long_f_2ulong,RPN_ul_div,2,VLNT_Long);
        SMFA("__l%",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_mod,2,VLNT_Long);
        SMFA("__l%u",RPNFT_Long_F_2ULong,rpn_long_f_2ulong,RPN_ul_mod,2,VLNT_Long);
        SMFA("__l&",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_and,2,VLNT_Long);
        SMFA("__l|",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_or,2,VLNT_Long);
        SMFA("__l~",RPNFT_Long_F_1Long,rpn_long_f_1long,RPN_l_b_not,1,VLNT_Long);
        SMFA("__l^",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_b_xor,2,VLNT_Long);
        SMFA("__l<<",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_bs_l,2,VLNT_Long);
        SMFA("__l<<u",RPNFT_Long_F_2ULong,rpn_long_f_2ulong,RPN_ul_bs_l,2,VLNT_Long);
        SMFA("__l>>",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_bs_r,2,VLNT_Long);
        SMFA("__l>>u",RPNFT_Long_F_2ULong,rpn_long_f_2ulong,RPN_ul_bs_r,2,VLNT_Long);
        SMFA("__l==",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_eq,2,VLNT_Int);
        SMFA("__l!=",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_neq,2,VLNT_Int);
        SMFA("__l>",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_gt,2,VLNT_Int);
        SMFA("__l<",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_lt,2,VLNT_Int);
        SMFA("__l>=",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_gte,2,VLNT_Int);
        SMFA("__l<=",RPNFT_Long_F_Cmp,rpn_long_f_cmp,RPN_l_lte,2,VLNT_Int);
        SMFA("__l==u",RPNFT_Long_F_UCmp,rpn_long_f_ucmp,RPN_ul_eq,2,VLNT_Int);
        SMFA("__l!=u",RPNFT_Long_F_UCmp,rpn_long_f_ucmp,RPN_ul_neq,2,VLNT_Int);
        SMFA("__l>u",RPNFT_Long_F_UCmp,rpn_long_f_ucmp,RPN_ul_gt,2,VLNT_Int);
        SMFA("__l<u",RPNFT_Long_F_UCmp,rpn_long_f_ucmp,RPN_ul_lt,2,VLNT_Int);
        SMFA("__l>=u",RPNFT_Long_F_UCmp,rpn_long_f_ucmp,RPN_ul_gte,2,VLNT_Int);
        SMFA("__l<=u",RPNFT_Long_F_UCmp,rpn_long_f_ucmp,RPN_ul_lte,2,VLNT_Int);
        SMFA("__labs",RPNFT_Long_F_1Long,rpn_long_f_1long,labs,1,VLNT_Long);
        SMFA("__lmax",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_max,2,VLNT_Long);
        SMFA("__lmin",RPNFT_Long_F_2Long,rpn_long_f_2long,RPN_l_min,2,VLNT_Long);
        SMFA("__lmaxu",RPNFT_Long_F_2ULong,rpn_long_f_2ulong,RPN_ul_max,2,VLNT_Long);
        SMFA("__lminu",RPNFT_Long_F_2ULong,rpn_long_f_2ulong,RPN_ul_min,2,VLNT_Long);
        SMFA("random_l",RPNFT_Long_F_NoArg,rpn_long_f_noarg,random_l,0,VLNT_Long);
        SMFA("as_l",RPNFT_Long_F_1Long,rpn_long_f_1long,castas_l,1,VLNT_Long);
        SMFA("__lb?t:f",RPNFT_Long_F_Ternary,rpn_long_f_ternary,ternary_l,3,VLNT_Long);
        
        SMFA("__d+",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_add,2,VLNT_Double);
        SMFA("__d++",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_inc,1,VLNT_Double);
        SMFA("__d-",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_sub,2,VLNT_Double);
        SMFA("__d-m",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_neg,1,VLNT_Double);
        SMFA("__d--",RPNFT_Double_F_1Double,rpn_double_f_1double,RPN_d_dec,1,VLNT_Double);
        SMFA("__d*",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_mul,2,VLNT_Double);
        SMFA("__d/",RPNFT_Double_F_2Double,rpn_double_f_2double,RPN_d_div,2,VLNT_Double);
        SMFA("__d%",RPNFT_Double_F_2Double,rpn_double_f_2double,fmod,2,VLNT_Double);
        SMFA("__d==",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_eq,2,VLNT_Int);
        SMFA("__d!=",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_neq,2,VLNT_Int);
        SMFA("__d>",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_gt,2,VLNT_Int);
        SMFA("__d<",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_lt,2,VLNT_Int);
        SMFA("__d>=",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_gte,2,VLNT_Int);
        SMFA("__d<=",RPNFT_Double_F_Cmp,rpn_double_f_cmp,RPN_d_lte,2,VLNT_Int);
        SMFA("__dabs",RPNFT_Double_F_1Double,rpn_double_f_1double,fabs,1,VLNT_Double);
        SMFA("__dmax",RPNFT_Double_F_2Double,rpn_double_f_2double,fmax,2,VLNT_Double);
        SMFA("__dmin",RPNFT_Double_F_2Double,rpn_double_f_2double,fmin,2,VLNT_Double);
        SMFA("random_d",RPNFT_Double_F_NoArg,rpn_double_f_noarg,random_d,0,VLNT_Double);
        SMFA("exp",RPNFT_Double_F_1Double,rpn_double_f_1double,exp,1,VLNT_Double);
        SMFA("exp2",RPNFT_Double_F_1Double,rpn_double_f_1double,exp2,1,VLNT_Double);
        SMFA("log",RPNFT_Double_F_1Double,rpn_double_f_1double,log,1,VLNT_Double);
        SMFA("log2",RPNFT_Double_F_1Double,rpn_double_f_1double,log2,1,VLNT_Double);
        SMFA("log10",RPNFT_Double_F_1Double,rpn_double_f_1double,log10,1,VLNT_Double);
        SMFA("pow",RPNFT_Double_F_2Double,rpn_double_f_2double,pow,2,VLNT_Double);
        SMFA("sqrt",RPNFT_Double_F_1Double,rpn_double_f_1double,sqrt,1,VLNT_Double);
        SMFA("cbrt",RPNFT_Double_F_1Double,rpn_double_f_1double,cbrt,1,VLNT_Double);
        SMFA("hypot",RPNFT_Double_F_2Double,rpn_double_f_2double,hypot,2,VLNT_Double);
        SMFA("sin",RPNFT_Double_F_1Double,rpn_double_f_1double,sin,1,VLNT_Double);
        SMFA("cos",RPNFT_Double_F_1Double,rpn_double_f_1double,cos,1,VLNT_Double);
        SMFA("tan",RPNFT_Double_F_1Double,rpn_double_f_1double,tan,1,VLNT_Double);
        SMFA("asin",RPNFT_Double_F_1Double,rpn_double_f_1double,asin,1,VLNT_Double);
        SMFA("acos",RPNFT_Double_F_1Double,rpn_double_f_1double,acos,1,VLNT_Double);
        SMFA("atan",RPNFT_Double_F_1Double,rpn_double_f_1double,atan,1,VLNT_Double);
        SMFA("sind",RPNFT_Double_F_1Double,rpn_double_f_1double,sind,1,VLNT_Double);
        SMFA("cosd",RPNFT_Double_F_1Double,rpn_double_f_1double,cosd,1,VLNT_Double);
        SMFA("tand",RPNFT_Double_F_1Double,rpn_double_f_1double,tand,1,VLNT_Double);
        SMFA("asind",RPNFT_Double_F_1Double,rpn_double_f_1double,asind,1,VLNT_Double);
        SMFA("acosd",RPNFT_Double_F_1Double,rpn_double_f_1double,acosd,1,VLNT_Double);
        SMFA("atand",RPNFT_Double_F_1Double,rpn_double_f_1double,atand,1,VLNT_Double);
        SMFA("ceil",RPNFT_Double_F_1Double,rpn_double_f_1double,ceil,1,VLNT_Double);
        SMFA("floor",RPNFT_Double_F_1Double,rpn_double_f_1double,floor,1,VLNT_Double);
        SMFA("round",RPNFT_Double_F_1Double,rpn_double_f_1double,round,1,VLNT_Double);
        SMFA("trunc",RPNFT_Double_F_1Double,rpn_double_f_1double,trunc,1,VLNT_Double);
        SMFA("as_d",RPNFT_Double_F_1Double,rpn_double_f_1double,castas_d,1,VLNT_Double);
        SMFA("__db?t:f",RPNFT_Double_F_Ternary,rpn_double_f_ternary,ternary_d,3,VLNT_Double);

        SMFA("!",RPNFT_Invert,rpn_invert,bool_invert,1,VLNT_Int);
        SMFA("&&",RPNFT_2_Bools,rpn_f_2_bools,bool_and,2,VLNT_Int);
        SMFA("||",RPNFT_2_Bools,rpn_f_2_bools,bool_or,2,VLNT_Int);
        #if 0
        while(true){
            StringMap_rpn_func_call_print_debug(DefaultRPNFunctionMap);
            char buf[20]={0};
            fgets(buf,19,stdin);
            StringMap_rpn_func_call_resize(&DefaultRPNFunctionMap,strtol(buf,0,10));
        }
        #elif 0
        while(true){
            StringMap_as_number_print_debug(DefaultRPNVariablesMap);
            char buf[20]={0};
            fgets(buf,19,stdin);
            StringMap_as_number_resize(&DefaultRPNVariablesMap,strtol(buf,0,10));
        }
        #endif
#undef SMFA
    }else{
        puts("RPNEvaluatorInit has already been initialized.");
    }
}
#ifndef NDEBUG
#define RPNEvaluatorInitCalledFirst() \
if(!DefaultRPNFunctionMap){\
    fprintf(stderr,ERR("RPNEvaluatorInit should be called first.\n"));\
    exit(EXIT_FAILURE);\
} (void)0
#else
#define RPNEvaluatorInitCalledFirst() (void)0
#endif
as_number_opt_t String_to_as_number_t(const char* token);
RPNValidStringE _RPNEvaluatorIsVarNameOk(const char* token,const VariableLoader_t* vl,Stack_as_number_t* stack_an,bool process_num);
bool _ProcessRPNFunctionCall(Stack_as_number_t* stack_an,const rpn_func_call_t* rpn_f_c,bool process_num);
VLNumberType _Stack_get_highest_number_type(const Stack_as_number_t* this,int num_args);
void _Stack_as_number_print(const Stack_as_number_t* this);
RPNValidStringE RPNEvaluatorEvaluate(const char* rpn_str,const VariableLoader_t* vl,as_number_t* get_value,bool see_stack,bool process_num,const char* rpn_start_b,const char* rpn_end_b,char rpn_sep){
    RPNEvaluatorInitCalledFirst();
    _DividedByZero=false;
    _BitShiftNegative=false;
    if(process_num) *get_value=(as_number_t){0};
    const char* start_p,* end_p;
    int depth=first_outermost_bracket(rpn_str,rpn_start_b,rpn_end_b,&start_p,&end_p);
    if(!start_p||depth){
        fprintf(stderr,ERR("RPN needs to be enclosed in brackets '%s' and '%s'.\n"),rpn_start_b,rpn_end_b);
        return RPNVS_ImproperBrackets;
    }
    if(start_p+1==end_p){
        fprintf(stderr,ERR("RPN needs to have at least one variable.\n"));
        return RPNVS_NotEnoughNumbers;//Only just "()"
    }
    char* rpn_str_no_b=char_string_slice_no_brackets(start_p,end_p,rpn_start_b);
    Stack_as_number_t* stack_an=Stack_as_number_new();
    const char* token_b_p=rpn_str_no_b,* token_e_p=rpn_str_no_b;
    while(true){
        token_e_p=strchr(token_b_p,rpn_sep);
        char* current_token;
        as_number_opt_t an_opt;
        RPNValidStringE status;
        if(see_stack) _Stack_as_number_print(stack_an);
        if(token_e_p){
            current_token=char_string_slice(token_b_p,token_e_p-1);
#define DOPARSETOKENROUTINE()\
if(see_stack) puts(current_token);\
if((an_opt=String_to_as_number_t(current_token)).exists) Stack_as_number_push(stack_an,an_opt.v);\
else if((status=_RPNEvaluatorIsVarNameOk(current_token,vl,stack_an,process_num))!=RPNVS_IsVLName&&status!=RPNVS_IsFunction&&status!=RPNVS_IsRPNVar){\
    switch(status){\
        case RPNVS_NameFunctionCollision:\
            fprintf(stderr,ERR("Name Collision Error: Token '%s' within the program is already a name for an existing function.\n"),current_token);\
            break;\
        case RPNVS_NameUndefined:\
            fprintf(stderr,ERR("Name Undefined Error: Token '%s' within the program is neither a number, varible, nor function (or function with double support).\n"),current_token);\
            break;\
        case RPNVS_NotEnoughNumbers:\
            fprintf(stderr,ERR("Not Enough Numbers Error: Token '%s' (function) doesn't have enough numbers to pop. Stack is empty.\n"),current_token);\
            break;\
        case RPNVS_DivisionByZero:\
            fprintf(stderr,ERR("Illegal Operation Error: Division by Zero with non-double numbers has occured.\n"));\
            break;\
        case RPNVS_NegativeBitShift:\
            fprintf(stderr,ERR("Illegal Operation Error: Bit-shifting using a negative number on the second argument is not supported.\n"));\
            break;\
        default: SHOULD_BE_UNREACHABLE(); break;\
    }\
    free(current_token);\
    free(rpn_str_no_b);\
    Stack_as_number_free(stack_an);\
    return status;\
}else if(status==RPNVS_IsVLName) Stack_as_number_push(stack_an,VL_get_as_number(vl,current_token).value);\
free(current_token)
            DOPARSETOKENROUTINE();
            token_b_p=token_e_p+1;//Next token in strchr.
            continue;
        }//Token_b_p has the last token.
        current_token=malloc(sizeof(char)*(strlen(token_b_p)+1));
        EXIT_IF_NULL(current_token,char*);
        strcpy(current_token,token_b_p);
        DOPARSETOKENROUTINE();
        #undef DOPARSETOKENROUTINE
        break;
    }
    if(see_stack) _Stack_as_number_print(stack_an);
    bool stack_is_one=(stack_an->size==1);
    if(stack_is_one){if(process_num) *get_value=stack_an->stack[0];}
    else fprintf(stderr,ERR("Too Many Numbers Error: RPN String has more than 1 number in the stack. Not a valid RPN string.\n"));
    Stack_as_number_free(stack_an);
    free(rpn_str_no_b);
    return stack_is_one?RPNVS_Ok:RPNVS_TooManyNumbers;
}
void RPNEvaluatorFree(void){
    StringMap_rpn_func_call_free(DefaultRPNFunctionMap);
    StringMap_as_number_free(DefaultRPNVariablesMap);
    DefaultRPNFunctionMap=0;
    DefaultRPNVariablesMap=0;
}
//Check if VariableLoader names doesn't share any names in DefaultRPNFunctionMap.
RPNValidStringE _RPNEvaluatorIsVarNameOk(const char* token,const VariableLoader_t* vl,Stack_as_number_t* stack_an,bool process_num){
    static const char NumberTypePrefixes[5]={'\0','c','i','l','d'};//Arranged based on the VLNumberType enums.
    bool token_is_var=VL_get_as_number(vl,token).exists;
    StringMapOpt_as_number_t smoant;
    if((smoant=StringMap_as_number_read_ph(DefaultRPNVariablesMap,token)).exists){
        Stack_as_number_push(stack_an,smoant.value); //Just push since RPNVariables are prefixed with @. User-created variables cannot use @.
        return RPNVS_IsRPNVar;
    }
    //Keep continuing code as if it were a valid string (pop numbers) until an RPNVS_NameFunctionCollision.
    StringMapOpt_rpn_func_call_t smorfc_noprf={0},smorfc_wprf={0};
    if((smorfc_noprf=StringMap_rpn_func_call_read(DefaultRPNFunctionMap,token)).exists&&token_is_var) return RPNVS_NameFunctionCollision;
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
                return RPNVS_NameFunctionCollision;
            }
            rpn_f_c_array[i]=prefix_smorfc;
            rpn_f_num_args=prefix_smorfc.value.num_args;//To promote any variables to use the proper function.
        }
    }
    free(token_with_prefix);
    if(rpn_f_num_args) smorfc_wprf=rpn_f_c_array[_Stack_get_highest_number_type(stack_an,rpn_f_num_args)];
    if(smorfc_noprf.exists&&smorfc_noprf.value.type!=RPNFT_Null){//Don't process if RPNFT_Null for any name collisions.
        if(!_ProcessRPNFunctionCall(stack_an,&smorfc_noprf.value,process_num)) return RPNVS_NotEnoughNumbers;
        if(_DividedByZero) return RPNVS_DivisionByZero;
        if(_BitShiftNegative) return RPNVS_NegativeBitShift;
    }else if(smorfc_wprf.exists){
        if(!_ProcessRPNFunctionCall(stack_an,&smorfc_wprf.value,process_num)) return RPNVS_NotEnoughNumbers;
        if(_DividedByZero) return RPNVS_DivisionByZero;
        if(_BitShiftNegative) return RPNVS_NegativeBitShift;
    }else{
        return token_is_var?RPNVS_IsVLName:RPNVS_NameUndefined;//If not VLVar, then NameNotAdded since no match for token.
    }
    return token_is_var?RPNVS_IsVLName:RPNVS_IsFunction;
}
//Bool if out of numbers.
bool _ProcessRPNFunctionCall(Stack_as_number_t* stack_an,const rpn_func_call_t* rpn_f_c,bool process_num){
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
    if(!process_num){//To do compilation errors and not runtime errors (Checking if dividing by 0 or bit-shifting using a negative number)
        result.l=0;
        goto skip_process_num;
    }
    #define ARGS_CAST(I) ((args[I].v.type==VLNT_Double)?args[I].v.d:\
    (args[I].v.type==VLNT_Long)?args[I].v.l:\
    (args[I].v.type==VLNT_Int)?args[I].v.i:\
    (args[I].v.type==VLNT_Char)?args[I].v.c:0)
    switch(rpn_f_c->type){
        case RPNFT_Char_F_NoArg: result.c=rpn_fu.rpn_char_f_noarg(); break;
        case RPNFT_Char_F_1Char: result.c=rpn_fu.rpn_char_f_1char(ARGS_CAST(0)); break;
        case RPNFT_Char_F_2Char: result.c=rpn_fu.rpn_char_f_2char(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Char_F_2UChar: result.c=rpn_fu.rpn_char_f_2uchar(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Char_F_Cmp: result.i=rpn_fu.rpn_char_f_cmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Char_F_UCmp: result.i=rpn_fu.rpn_char_f_ucmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Char_F_Ternary: result.c=rpn_fu.rpn_char_f_ternary(ARGS_CAST(0),ARGS_CAST(1),ARGS_CAST(2)); break;
        case RPNFT_Int_F_NoArg: result.i=rpn_fu.rpn_int_f_noarg(); break;
        case RPNFT_Int_F_1Int: result.i=rpn_fu.rpn_int_f_1int(ARGS_CAST(0)); break;
        case RPNFT_Int_F_2Int: result.i=rpn_fu.rpn_int_f_2int(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Int_F_2UInt: result.i=rpn_fu.rpn_int_f_2uint(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Int_F_Cmp: result.i=rpn_fu.rpn_int_f_cmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Int_F_UCmp: result.i=rpn_fu.rpn_int_f_ucmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Int_F_Ternary: result.i=rpn_fu.rpn_int_f_ternary(ARGS_CAST(0),ARGS_CAST(1),ARGS_CAST(2)); break;
        case RPNFT_Long_F_NoArg: result.l=rpn_fu.rpn_long_f_noarg(); break;
        case RPNFT_Long_F_1Long: result.l=rpn_fu.rpn_long_f_1long(ARGS_CAST(0)); break;
        case RPNFT_Long_F_2Long: result.l=rpn_fu.rpn_long_f_2long(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Long_F_2ULong: result.l=rpn_fu.rpn_long_f_2ulong(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Long_F_Cmp: result.i=rpn_fu.rpn_long_f_cmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Long_F_UCmp: result.i=rpn_fu.rpn_long_f_ucmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Long_F_Ternary: result.l=rpn_fu.rpn_long_f_ternary(ARGS_CAST(0),ARGS_CAST(1),ARGS_CAST(2)); break;
        case RPNFT_Double_F_NoArg: result.d=rpn_fu.rpn_double_f_noarg(); break;
        case RPNFT_Double_F_1Double: result.d=rpn_fu.rpn_double_f_1double(ARGS_CAST(0)); break;
        case RPNFT_Double_F_2Double: result.d=rpn_fu.rpn_double_f_2double(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Double_F_Ternary: result.d=rpn_fu.rpn_double_f_ternary(ARGS_CAST(0),ARGS_CAST(1),ARGS_CAST(2)); break;
        case RPNFT_Double_F_Cmp: result.i=rpn_fu.rpn_double_f_cmp(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Invert: result.i=rpn_fu.rpn_invert(ARGS_CAST(0)); break;
        case RPNFT_2_Bools: result.i=rpn_fu.rpn_f_2_bools(ARGS_CAST(0),ARGS_CAST(1)); break;
        case RPNFT_Null: SHOULD_BE_UNREACHABLE(); free(args); return true; break;
    }
    skip_process_num:
    Stack_as_number_push(stack_an,result);
    free(args);
    return true;
    #undef ARGS_CAST
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
        VLNumberPrintNumber(this->stack[i],10);
        fputs(i!=this->size-1?", ":"",stdout);
    }
    printf("]\n");
}
void RPNEvaluatorAssignVar(const char* token,as_number_t num){
    assert(StringMap_as_number_assign_ph(DefaultRPNVariablesMap,token,num)==VA_Rewritten);
}
StringMapOpt_as_number_t RPNEvaluatorReadVar(const char* token){
    return StringMap_as_number_read_ph(DefaultRPNVariablesMap,token);
}