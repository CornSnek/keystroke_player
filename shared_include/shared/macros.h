#ifndef _MACROS_H_
#define _MACROS_H_
#include <stdlib.h>
#include <stdio.h>
#ifndef NDEBUG
#define EXIT_IF_NULL(token_name,type)\
if(!token_name){\
    fprintf(stderr,ERR("Unable to get pointer of type '" #type "' for token '" #token_name "' at line %d for file %s. Exiting program.\n"),__LINE__,__FILE__);\
    exit(EXIT_FAILURE);\
}((void)0)
#else
#define EXIT_IF_NULL(token_name,type) ((void)0)
#endif
#define ERR(string) "\x1b[1;33m" string "\x1b[0m"
#define CLEAR_TERM "\x1B[H\x1B[0J"
#define NSEC_TO_SEC 1000000000
#define MICSEC_TO_SEC 1000000
//101 to include '\0'.
#define INPUT_BUFFER_LEN 200
#define LAST_CMD_BUFFER_LEN 100
#define LAST_COMMANDS_LEN 50

#define CONFIG_FILE_F "./config.bin"
#define LAST_FILE_F "./last_file.conf"
#define MACRO_PROCESS_F "./macro_expand.out"
#define MACROS_DEF_START_B "[!!"
#define MACROS_DEF_END_B "!!]"
#define MACRO_START_B "[!"
#define MACRO_END_B "!]"
#define MACRO_DEF_SEP ":="
#define MACRO_VAR_SEP ':'
#define RPN_EVAL_START_B "("
#define RPN_EVAL_END_B ")"
#define RPN_EVAL_SEP ','

//Counts number of arguments in a list from 1 up to 100
#define NUM_ARGS2(X100, X99, X98, X97, X96, X95, X94, X93, X92, X91, X90, X89, X88, X87, X86, X85, X84, X83, X82, X81, X80, X79, X78, X77, X76, X75, X74, X73, X72, X71, X70, X69, X68, X67, X66, X65, X64, X63, X62, X61, X60, X59, X58, X57, X56, X55, X54, X53, X52, X51, X50, X49, X48, X47, X46, X45, X44, X43, X42, X41, X40, X39, X38, X37, X36, X35, X34, X33, X32, X31, X30, X29, X28, X27, X26, X25, X24, X23, X22, X21, X20, X19, X18, X17, X16, X15, X14, X13, X12, X11, X10, X9, X8, X7, X6, X5, X4, X3, X2, X1, N, ...)   N
#define NUM_ARGS(...) NUM_ARGS2(__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define __SL_1(a) #a
#define __SL_2(a,...) #a,__SL_1(__VA_ARGS__)
#define __SL_3(a,...) #a,__SL_2(__VA_ARGS__)
#define __SL_4(a,...) #a,__SL_3(__VA_ARGS__)
#define __SL_5(a,...) #a,__SL_4(__VA_ARGS__)
#define __SL_6(a,...) #a,__SL_5(__VA_ARGS__)
#define __SL_7(a,...) #a,__SL_6(__VA_ARGS__)
#define __SL_8(a,...) #a,__SL_7(__VA_ARGS__)
#define __SL_9(a,...) #a,__SL_8(__VA_ARGS__)
#define __SL_10(a,...) #a,__SL_9(__VA_ARGS__)
#define __SL_11(a,...) #a,__SL_10(__VA_ARGS__)
#define __SL_12(a,...) #a,__SL_11(__VA_ARGS__)
#define __SL_13(a,...) #a,__SL_12(__VA_ARGS__)
#define __SL_14(a,...) #a,__SL_13(__VA_ARGS__)
#define __SL_15(a,...) #a,__SL_14(__VA_ARGS__)
#define __SL_16(a,...) #a,__SL_15(__VA_ARGS__)
#define __SL_17(a,...) #a,__SL_16(__VA_ARGS__)
#define __SL_18(a,...) #a,__SL_17(__VA_ARGS__)
#define __SL_19(a,...) #a,__SL_18(__VA_ARGS__)
#define __SL_20(a,...) #a,__SL_19(__VA_ARGS__)
#define __SL_21(a,...) #a,__SL_20(__VA_ARGS__)
#define __SL_22(a,...) #a,__SL_21(__VA_ARGS__)
#define __SL_23(a,...) #a,__SL_22(__VA_ARGS__)
#define __SL_24(a,...) #a,__SL_23(__VA_ARGS__)
#define __SL_25(a,...) #a,__SL_24(__VA_ARGS__)
#define __SL_26(a,...) #a,__SL_25(__VA_ARGS__)
#define __SL_27(a,...) #a,__SL_26(__VA_ARGS__)
#define __SL_28(a,...) #a,__SL_27(__VA_ARGS__)
#define __SL_29(a,...) #a,__SL_28(__VA_ARGS__)
#define __SL_30(a,...) #a,__SL_29(__VA_ARGS__)
#define __SL_31(a,...) #a,__SL_30(__VA_ARGS__)
#define __SL_32(a,...) #a,__SL_31(__VA_ARGS__)
#define __SL_33(a,...) #a,__SL_32(__VA_ARGS__)
#define __SL_34(a,...) #a,__SL_33(__VA_ARGS__)
#define __SL_35(a,...) #a,__SL_34(__VA_ARGS__)
#define __SL_36(a,...) #a,__SL_35(__VA_ARGS__)
#define __SL_37(a,...) #a,__SL_36(__VA_ARGS__)
#define __SL_38(a,...) #a,__SL_37(__VA_ARGS__)
#define __SL_39(a,...) #a,__SL_38(__VA_ARGS__)
#define __SL_40(a,...) #a,__SL_39(__VA_ARGS__)
#define __SL_41(a,...) #a,__SL_40(__VA_ARGS__)
#define __SL_42(a,...) #a,__SL_41(__VA_ARGS__)
#define __SL_43(a,...) #a,__SL_42(__VA_ARGS__)
#define __SL_44(a,...) #a,__SL_43(__VA_ARGS__)
#define __SL_45(a,...) #a,__SL_44(__VA_ARGS__)
#define __SL_46(a,...) #a,__SL_45(__VA_ARGS__)
#define __SL_47(a,...) #a,__SL_46(__VA_ARGS__)
#define __SL_48(a,...) #a,__SL_47(__VA_ARGS__)
#define __SL_49(a,...) #a,__SL_48(__VA_ARGS__)
#define __SL_50(a,...) #a,__SL_49(__VA_ARGS__)

//Finding a way to concatenate a number with the __SL_(Number) macros was tricky. Using just __SL_##N(__VA_ARGS__) concatenates as __SL_NUM_ARGS and not __SL_(Number)
#define __STR_LIST_LOOKUP2(N) __SL_##N
#define __STR_LIST_LOOKUP(N,...) __STR_LIST_LOOKUP2(N)(__VA_ARGS__)
//1st argument counts number of arguments.
#define STR_LIST(...) __STR_LIST_LOOKUP(NUM_ARGS(__VA_ARGS__),__VA_ARGS__)
#endif