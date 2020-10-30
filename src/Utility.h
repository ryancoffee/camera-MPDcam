/*

 * R Coffee coffee@slac.stanford.edu
 * updating to perform histogram with intermittent frame storage
 *
 */

#ifndef UTILS_H
#define UTILS

#include <stdint.h>

// log2 methods from https://stackoverflow.com/questions/994593/how-to-do-an-integer-log2-in-c/994709
// http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html
// http://www.linuxassembly.org/articles/linasm.html
// https://stackoverflow.com/questions/10374839/how-to-port-a-asm-code-to-vs-2008
static inline uint16_t log2(const uint16_t x) {
  uint16_t y;
  asm ( "\tbsr %1, %0\n"
      : "=r"(y)
      : "r" (x)
  );
  return y;
}
static inline uint32_t log2(const uint32_t x) {
  uint32_t y;
  asm ( "\tbsr %1, %0\n"
      : "=r"(y)
      : "r" (x)
  );
  return y;
}
static inline uint64_t log2(const uint64_t x) {
  uint64_t y;
  asm ( "\tbsr %1, %0\n"
      : "=r"(y)
      : "r" (x)
  );
  return y;
}

// base 2 exponentiation: https://cp-algorithms.com/algebra/binary-exp.html
static inline uint16_t pow2( uint16_t b) {
    uint16_t res = 1;
    uint16_t a = 2;
    while (b > 0) {
        if (b & 1) // bitwise AND to test if b is odd I guess?
            res += a;
        a *= a;
        b >>= 1; // bitwise right shift 
    }
    return res;
}
static inline uint32_t pow2( uint32_t b) {
    uint32_t res = 1;
    uint32_t a = 2;
    while (b > 0) {
        if (b & 1) // bitwise AND to test if b is odd I guess?
            res += a;
        a *= a;
        b >>= 1; // bitwise right shift 
    }
    return res;
}
static inline uint64_t pow2( uint64_t b) {
    uint64_t res = 1;
    uint64_t a = 2;
    while (b > 0) {
        if (b & 1) // bitwise AND to test if b is odd I guess?
            res += a;
        a *= a;
        b >>= 1; // bitwise right shift 
    }
    return res;
}



//**************************************//
//		Support functions	//
//		from SDK_Example.c	//
//**************************************//

// Calculate the mean value of a UInt16 image
double mean(UInt16 * Img, UInt16 NPixel)
{
	int i=0;
	double res =0.0;
	for(i=0;i<NPixel;i++)	
		res+= (double)Img[i];
	return res/(double)NPixel;
}

// Calculate the mean value of a double image 
double mean_double(double * Img, UInt16 NPixel)
{
	int i=0;
	double res =0.0;
	for(i=0;i<NPixel;i++)	
		res+= Img[i];
	return res/(double)NPixel;
}

// Create an histogram of the distribution of photon counts over the imager
void Hist(UInt16* Img, UInt16* hist)
{
	int i=0;
	memset(hist,'\0',65535*sizeof(UInt16));
	for(i=0;i<2048;i++)
		hist[Img[i]]++;
}

#endif
