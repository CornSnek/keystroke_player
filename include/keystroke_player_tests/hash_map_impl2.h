#ifndef _HASH_MAP_IMPL2_H_
#define _HASH_MAP_IMPL2_H_
#include "generics/hash_map.h"
StringMap_ImplDecl(size_t,SizeT,
(const char* str){
    hash_t hash=5381;//djb hashing
    int c;
    while((c=*str++)) hash=((hash<<5)+hash)+c;
    return hash;
})
IntLongMap_ImplDecl(size_t,SizeT)
#endif