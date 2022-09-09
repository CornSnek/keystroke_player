#ifndef _FREQUENCY_H_
#define _FREQUENCY_H_
typedef struct Frequency_s{//Prints array to count the frequency of unsigned num of 0 to highest_num (Default 0).
    unsigned int highest_num;
    unsigned int* nums;
}Frequency_t;
Frequency_t* Frequency_new();
void Frequency_add(Frequency_t* this,unsigned int this_num);
void Frequency_print(const Frequency_t* this);
void Frequency_free(Frequency_t* this);
#endif
