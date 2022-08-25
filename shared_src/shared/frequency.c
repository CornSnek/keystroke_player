#include "frequency.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
Frequency_t* Frequency_new(){
    Frequency_t* this=malloc(sizeof(Frequency_t));
    EXIT_IF_NULL(this,Frequency_t*);
    *this=(Frequency_t){.nums=calloc(1,sizeof(unsigned int))};//For this->nums[0].
    EXIT_IF_NULL(this->nums,unsigned int*);
    return this;
}
void Frequency_add(Frequency_t* this,unsigned int this_num){
    bool do_realloc=this_num>this->highest_num;
    int difference=this_num-this->highest_num;//To 0 other realloc variables that are < this_num.
    this->highest_num=do_realloc?this_num:this->highest_num;
    if(do_realloc){
        this->nums=realloc(this->nums,sizeof(unsigned int)*(this_num+1));
        EXIT_IF_NULL(this->nums,unsigned int*);//this_num may be too high.
        for(unsigned int i=this_num-difference+1;i<this_num;i++) this->nums[i]=0; //+1 for i because it would 0 an already initialized count.
        this->nums[this_num]=1;
    }else this->nums[this_num]++;
}
void Frequency_print(const Frequency_t* this){
    printf("[");
    unsigned int total=0;
    for(unsigned int i=0;i<=this->highest_num;i++){
        printf("[%u]=%u%s",i,this->nums[i],i!=this->highest_num?",":"");
        total+=this->nums[i];
    }
    printf("] Total: %u\n",total);
}
void Frequency_free(Frequency_t* this){
    free(this->nums);
    free(this);
}