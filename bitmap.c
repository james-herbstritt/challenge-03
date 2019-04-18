#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <assert.h>

#include "bitmap.h"


enum { BITS_PER_WORD = sizeof(uint8_t) * CHAR_BIT };

#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b) ((b) % BITS_PER_WORD)


int 
bitmap_get(void* bm, int ii)
{
    assert(ii < 256);
    uint8_t bit = ((uint8_t*) bm)[WORD_OFFSET(ii)] & (1 << BIT_OFFSET(ii));                                       
    return bit != 0;
}

void 
bitmap_put(void* bm, int ii, int vv)
{
    assert(ii < 256);
    ((uint8_t*)bm)[WORD_OFFSET(ii)] &= ~(1 << BIT_OFFSET(ii));
    ((uint8_t*)bm)[WORD_OFFSET(ii)] |= (vv << BIT_OFFSET(ii));

}
void 
bitmap_print(void* bm, int size)
{
    for (int i = 0; i < size; i++) {
        printf("%d ", bitmap_get(bm, i));
    }
    printf("\n");
}
