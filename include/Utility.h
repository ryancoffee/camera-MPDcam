/*

 * R Coffee coffee@slac.stanford.edu
 * updating to perform histogram with intermittent frame storage
 *
 */

#ifndef UTILITY_H
#define UTILITY

#include <stdint.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <time.h>
#include <sys/time.h>

// log2 methods from https://stackoverflow.com/questions/994593/how-to-do-an-integer-log2-in-c/994709
// http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html
// http://www.linuxassembly.org/articles/linasm.html
// https://stackoverflow.com/questions/10374839/how-to-port-a-asm-code-to-vs-2008


//**************************************//
//	Support functions		//
//	From Ryan Coffee		//
//**************************************//

namespace Utility{

	template <typename T>
		void writeBinaryFile(T * array,const size_t sz, const std::string fileName)
		{
			std::fstream binaryIo;
			binaryIo.open(fileName.c_str(), std::ios::out| std::ios::binary);
			binaryIo.seekp(0);
			std::cerr << "write out this many:\t" << sz << std::endl;
			binaryIo.write((char*)(&sz), sizeof(size_t));
			binaryIo.write((char*)(array), sz * sizeof(T));
			/*
			std::cerr << "wrote these out:\n" ;
			for (size_t i=0;i<sz;i++){
				std::cerr << (int)array[i] << " ";
			}
			std::cerr << std::endl;
			*/
			binaryIo.close();
			return;
		}

	template <typename T>
		void readBinaryFile(const std::string fileName,const size_t sz,T * array)
		{
			std::fstream binaryIo;
			binaryIo.open(fileName.c_str(), std::ios::in | std::ios::binary );
			binaryIo.read((char*)&sz, sizeof(sz)); // read the number of elements 
			std::cerr << "read in this many:\t" << sz << std::endl;
			if (array == NULL){
				array = (T*) calloc(sz,sizeof(T));
			}
			binaryIo.read((char*)(array), sz * sizeof(T));
			binaryIo.close();
			/*
			std::cerr << "read these in:\n" ;
			for (size_t i=0;i<sz;i++){
				std::cerr << (int)array[i] << " ";
			}
			*/
			return;
		}


	template <typename T>
		inline T log2(const T x) {
			T y;
			asm ( "\tbsr %1, %0\n"
					: "=r"(y)
					: "r" (x)
			    );
			return y;
		}

	// base 2 exponentiation: https://cp-algorithms.com/algebra/binary-exp.html
	template <typename T>
		inline T pow2( T b) {
			T res = 1;
			T a = 2;
			while (b > 0) {
				if (b & 1) // bitwise AND to test if b is odd I guess?
					res += a;
				a *= a;
				b >>= 1; // bitwise right shift 
			}
		return res;
	}

}



namespace 
{
double get_wall_time(){
	struct timeval time;
	if (gettimeofday(&time,NULL)){
		//  Handle error
		return 0;
	}
	return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
double get_cpu_time(){
	return (double)clock() / CLOCKS_PER_SEC;
}

	template <typename T>
		inline std::istream& operator >> (std::istream & ins, std::vector<T> & record)
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
		inline std::istream& operator >> (std::istream & ins, std::vector< std::vector<T> > & matrix)
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
		inline std::ostream& operator << (std::ostream & outs, std::vector< T > & record)
		{
			for (unsigned i=0;i<record.size();++i){
				outs << record[i] << "\t";
			}
			outs << std::flush;
			return outs;
		}

	template <typename T>
		inline std::ostream& operator << (std::ostream & outs, std::vector< std::vector <T> > & matrix)
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

}

//**************************************//
//		Support functions	//
//		from SDK_Example.c	//
//**************************************//

namespace SDK_utils
{

	// Calculate the mean value of a UInt16 image
	template <typename T>
		inline double mean(T * Img, const size_t NPixel)
		{
			double res =0.0;
			for(size_t i=0;i<NPixel;i++)	
				res += (double)Img[i];
			return res/(double)NPixel;
		}

	// Create an histogram of the distribution of photon counts over the image
	template <typename T>
		inline void Hist(T * Img, T * hist)
		{
			memset(hist,'\0',65535*sizeof(T));
			for(size_t i=0;i<2048;i++)
				hist[(size_t)(Img[i])]++;
		}
}

#endif
