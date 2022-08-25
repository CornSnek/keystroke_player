#include "hash_map_impl2.h"
StringMap_ImplDef(size_t,SizeT)
IntLongMap_ImplDef(size_t,SizeT,
(long l){
    return l;//Just simple for testing.
})