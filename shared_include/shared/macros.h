#ifndef _MACROS_H_
#define _MACROS_H_
#include <stdlib.h>
#include <stdio.h>
#define EXIT_IF_NULL(token_name,type)\
if(!token_name){\
    fprintf(stderr,"Unable to get pointer of type '" #type "' for token '" #token_name "' at line %d for file %s. Exiting program.\n",__LINE__,__FILE__);\
    exit(EXIT_FAILURE);\
}
#endif
