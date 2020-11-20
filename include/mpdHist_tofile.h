#ifndef MPDHIST_H
#define MPDHIST_H

#define SLEEP usleep
#define MILLIS 1000
#include <time.h>
#include <sys/time.h>

template <typename T>
T get_wall_time(){
	struct timeval time;
	if (gettimeofday(&time,NULL)){
		//  Handle error
		return 0;
	}
	return (T)time.tv_sec + (T)time.tv_usec * .000001;
}

template <typename T>
T get_cpu_time(){
	return (T)clock() / CLOCKS_PER_SEC;
}

#endif
