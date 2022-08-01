#include "test_utils.h"
#include <stdio.h>
/*Next dyck word in lexicographical order (0 then 1).
If n=4, First: 00001111. Next: 00010111. Next: 00011011...
Last would be alternating 0's then 1's. (01010101)
Using catalan number in for loop would print all of the words.
Bit position and Max bit move is read as the following example:
00001111<Bit size of 8 has one bit of a max bit move of 4, which is the leftmost bit.
       1<Position 1 for this bit. Only has Max bit move of 1 (Is not moved at all in the code).
     o1<Position 1 for this bit. Position 2 is o. Max bit move of 2.
   xx1<Max bit move of 3.
 oxx1<o is Position 4 for this bit. Max bit move of 4.
Each bit from right to left can move left +1 more as shown above.
R>(Bits) Is read as (R)ead, the highlighted bit may (M)ove or not in M>(Bits)
*/
void next_dyck_word(bool* bits,size_t bits_size){
	size_t this_bit=2;//this_bit is also its move
	size_t bit_pos_now=1;
	size_t bit_i=bits_size-1;//Disregarding rightmost bit in while loop (this_bit:=1).
	while((bit_i--)-1){//Shouldn't read bits[-1].
		/*printf("Reading bit %ld(=%d) and bit %ld(=%d). Reading nth bit from right to left: %ld. Bit position: %ld.\n"
        ,bit_i-1,bits[bit_i-1]
		,bit_i,bits[bit_i],
		this_bit,bit_pos_now);*/
		if(!bits[bit_i]){
			if(bits[bit_i-1]){//If (10), swap 1-bit back to position 1. Read 2 bits next loop instead of 1.
				bits[--bit_i]=0; bits[bit_i+(this_bit++)-1]=1;
            }
			bit_pos_now++;
			continue;
		}
		if(bits[bit_i-1]){//If (11), swap 1-bit back to position 1.
			bits[bit_i]=0; bits[bit_i+bit_pos_now-1]=1;
			this_bit++;
			continue;
		}//If (01), shift left and finish algorithm (Is next dyck word).
		bits[bit_i]=0; bits[bit_i-1]=1;
		return;
	}
}
size_t catalan(unsigned int n){
    if (n<=1) return 1;
    size_t res=0;
    for (size_t i=0; i<n; i++)
        res+=catalan(i)*catalan(n-i-1);
    return res;
}
