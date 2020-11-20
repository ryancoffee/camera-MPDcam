#include "Utility.h"

#include <ctime>


double Utility::get_cpu_time(){
std::clock_t c_start = std::clock();
std::clock_t c_end = std::clock();
long_double time_elapsed_ms = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
std::cout << "CPU time used: " << time_elapsed_ms << " ms\n";
