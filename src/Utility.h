/*

 * R Coffee coffee@slac.stanford.edu
 * updating to perform histogram with intermittent frame storage
 *
 */

#ifndef UTILS_H
#define UTILS

#include <stdint.h>
#include <sstream>
#include <fstream>
#include <vector>

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

namespace Utility{
static inline double mean(UInt16 * Img, UInt16 NPixel)
{
	int i=0;
	double res =0.0;
	for(i=0;i<NPixel;i++)	
		res+= (double)Img[i];
	return res/(double)NPixel;
}
static inline double mean_double(double * Img, UInt16 NPixel)
{
	int i=0;
	double res =0.0;
	for(i=0;i<NPixel;i++)	
		res+= Img[i];
	return res/(double)NPixel;
}
static inline void Hist(UInt16* Img, UInt16* hist)
{
	int i=0;
	memset(hist,'\0',65535*sizeof(UInt16));
	for(i=0;i<2048;i++)
		hist[Img[i]]++;
}
/*
// Calculate the mean value of a UInt16 image
static inline double mean(UInt16 * Img, UInt16 NPixel);

// Calculate the mean value of a double image 
static inline double mean_double(double * Img, UInt16 NPixel);

// Create an histogram of the distribution of photon counts over the imager
static inline void Hist(UInt16* Img, UInt16* hist);
*/
}

//**************************************//
//	Support functions		//
//	From Ryan Coffee		//
//**************************************//

	template <typename T>
extern inline std::istream& operator >> (std::istream & ins, std::vector<T> & record)
{
	record.clear();
	std::string line;
	getline( ins, line );
	const char head='#';
	if (line.find(head) != std::string::npos){
		return ins;
	}
	std::istringstream iss( (std::string)line );
	T value;
	while (iss >> value){
		record.push_back(value);
	}
	return ins;
}

	template <typename T>
extern inline std::istream& operator >> (std::istream & ins, std::vector< std::vector<T> > & matrix)
{
	matrix.clear();
	std::vector<T> record;
	while (ins >> record)
	{
		if (record.size() > 0)
			matrix.push_back( record );
	}
	return ins;
}

	template <typename T>
extern inline std::ostream& operator << (std::ostream & outs, std::vector< T > & record)
{
	for (unsigned i=0;i<record.size();++i){
		outs << record[i] << "\t";
	}
	outs << std::flush;
	return outs;
}

	template <typename T>
extern inline std::ostream& operator << (std::ostream & outs, std::vector< std::vector <T> > & matrix)
{
	for (unsigned i=0;i<matrix.size();++i){
		for (unsigned j=0;j<matrix[i].size();++j){
			outs << matrix[i][j] << "\t";
		}
		outs << "\n";
	}
	outs << std::flush;
	return outs;
}
#endif
