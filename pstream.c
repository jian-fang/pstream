/*---------------------------------------------------------------------------*/
/* Program: STREAM                                                           */
/* Parallel Library: pthread                                                          */
/* Orignial Algorithm by John D. McCalpin                                    */
/* Programmer: Jian Fang (TU Delft)                                          */
/* Copyright: Jian Fang                                                      */
/*                                                                           */
// **************************************************************************//
// Running on defferent machine, you need to change the following parameters //
// CACHE_LINE_SIZE                                                           //
// ALIGNED_SIZE								     //
// int mapping[]							     //
// **************************************************************************//


#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>			//
#include <pthread.h>			// pthread
#include <stdint.h>			// int64_t
#include <stdlib.h>			// posix_memalign

/*
// not sure whether it needs these library
#include <string.h>
#include <time.h>
#include <sched.h>
#include "lock.h"
*/


// Each Array Size
#ifndef ARRAY_SIZE
#define ARRAY_SIZE 1000000000
#elif 	ARRAY_SIZE<=1
#define ARRAY_SIZE 1000000000
#endif

// Repeat Times (at least 2 times)
#ifndef NTIMES
#define NTIMES 10
#elif	NTIMES<=2
#define NTIMES 10
#endif

// Offest (Not used yet)
#ifndef OFFSET
#define OFFSET 0
#endif

// Scale Factor
#ifndef SCALE_FACTOR
#define SCALE_FACTOR 3
#endif

// Stream Data Type
#ifndef STREAM_TYPE
#define STREAM_TYPE int64_t
#endif

// Cacheline Size
#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

typedef STREAM_TYPE data_t;

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#define READ 0
#define WRITE 1
#define SELFINC 2
#define COPY 3
#define MUL  4
#define SELFADD 5
#define ADD  6
#define MULADD 7
#define SELFDADD 8

#ifndef OPERATION_TYPE
#define OPERATION_TYPE ADD
#endif


// Wait for the barrier
#ifndef BARRIER_ARRIVE
#define BARRIER_ARRIVE(B,RV)				\
	RV = pthread_barrier_wait(B);			\
	if(RV!=0 && RV!=PTHREAD_BARRIER_SERIAL_THREAD)	\
	{						\
		printf("wait for barrier errer!\n");	\
		exit(EXIT_FAILURE);			\
	}
#endif

// ------------------------------------- CPU CORE MAPPING --------------------------------//
//int mapping[40]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39};
int mapping[40]={0,1,2,3,4,5,6,7,8,9,20,21,22,23,24,25,26,27,28,29};

// ------------------------------------- Data structure ----------------------------------//
struct arg_t
{
	int32_t tid;
	int64_t sizeA;
	int64_t sizeB;
	int64_t sizeC;
	data_t * arrayA;
	data_t * arrayB;
	data_t * arrayC;
	pthread_barrier_t * barrier;
};






// ------------------------------------- Functions ----------------------------------//
// Function:		get_cpuid
// Functionality:	get the mapping cpuid
// Input:		thread number i
// Output:		cpu id
int get_cpuid(int i)
{
	return mapping[i];
}

// Function: 		alloc_aligned
// Functionality:	allocate new space
// Input:		space size to be allocated
// Output:		a pointer points to this space
void * alloc_aligned(size_t size,size_t aligned_size)
{
    void * ret;
    int rv;
    rv = posix_memalign((void**)&ret, aligned_size, size);

    if (rv) {
        perror("[ERROR] alloc_aligned() failed: out of memory");
        return 0;
    }

    return ret;
};


// Function: 		initial_array
// Functionality:	initial the three arrays
// Input:		the size of the arrays (should be a same value for all three arrays)
// Output:		a data_t pointer that points to the array
data_t * initialArray(int64_t size)
{
	data_t * array = (data_t*) alloc_aligned(size * sizeof(data_t),CACHE_LINE_SIZE);
	if (!array)
	 {
		perror("out of memory");
		exit(1);
	}

	for (int64_t i=0;i<size;i++)
	{
		array[i] = i+1;
	}
#ifdef ARRAY_INFO
	printf("Allocate array done!\tSize:%ld\n",size);
#endif
	return array;
}

void free_array(data_t* array)
{
	free(array);
}

void * pstream_thread(void* para)
{
	int rv;
	arg_t * args = (arg_t*) para;
	struct timeval time1, time2, time3;
	int64_t timeAll = 0;
	double dataAmount;
	double bandwidth;
	int amountFactor;	// $amountFactor * dataAmount is the total data amount
	int64_t sum;

#if OPERATION_TYPE==READ
	amountFactor=1;
#elif OPERATION_TYPE==WRITE || OPERATION_TYPE==SELFINC
	amountFactor=2;
#elif OPERATION_TYPE==COPY || OPERATION_TYPE==MUL || OPERATION_TYPE==SELFADD
	amountFactor=3;
#elif OPERATION_TYPE==ADD || OPERATION_TYPE==MULADD || OPERATION_TYPE==SELFDADD
	amountFactor=4;
#else
	#error "Operation Code Error: OP should be from 0~7"
#endif

	for(int i=0;i<NTIMES;i++)
	{
		sum = 0;

		//wait for the barrier until all the threads are ready
		BARRIER_ARRIVE(args->barrier,rv);

		//timing information
		if(args->tid == 0)
		{
			gettimeofday(&time1,NULL);
		}

		// the operations
		for(int j=0;j<args->sizeC;j++)
		{
#if OPERATION_TYPE==READ	// read only: read c
			sum += args->arrayC[j];
#elif OPERATION_TYPE==WRITE	// write only: write c
			args->arrayC[j] = 5;
#elif OPERATION_TYPE==SELFINC	// selfinc:self increase: c++
			args->arrayC[j] ++;
#elif   OPERATION_TYPE==COPY	// copy: c=a
			args->arrayC[j] = args->arrayA[j];
#elif OPERATION_TYPE==MUL	// multiply: c=a*n
			args->arrayC[j] = args->arrayA[j]*SCALE_FACTOR;
#elif OPERATION_TYPE==SELFADD	// selfadd: c=c+a
			args->arrayC[j] += args->arrayA[j];
#elif OPERATION_TYPE==ADD	// add: c=a+b
			args->arrayC[j] = args->arrayA[j]+args->arrayB[j];
#elif OPERATION_TYPE==MULADD	// muladd: multiply+add c=a*n+b
			args->arrayC[j] = args->arrayA[j]*SCALE_FACTOR+args->arrayB[j];
#elif OPERATION_TYPE==SELFDADD	// selfdadd: c=a+b+c
			args->arrayC[j] += (args->arrayA[j]+args->arrayB[j]);
#endif
		}

		// wait for the barrier until all the threads finish the current operation
		BARRIER_ARRIVE(args->barrier,rv);

#if OPERATION_TYPE==READ
		args->arrayA[0] = sum;
#endif

		//timing information
		if(args->tid == 0)
		{
			gettimeofday(&time2,NULL);
			if(i!=0)
			{
				timeAll += (time2.tv_sec-time1.tv_sec)*1000000L+time2.tv_usec-time1.tv_usec;
			}
		}
	}

	if(args->tid == 0)
	{
		timeAll = timeAll/(NTIMES-1);
		dataAmount = sizeof(data_t) * ARRAY_SIZE * amountFactor / 1024.0 / 1024.0 / 1024.0;
		bandwidth = dataAmount / (timeAll/1000000.0);

		printf("Total data amount being transferred: %.2f GB\n", dataAmount);
		printf("The operation runs %d times, and for each time %ld microsec\n",NTIMES,timeAll);
		printf("The average bandwidth is %.2f GB/s\n", bandwidth);
	}
}


int main(int argc, char* argv[])
{
	if(argc<2)
	{
		printf("Parameters error: use <execute file> <number of thread>\n");
		return 0;
	}
	else
	{
		struct timeval start_time, end_time;
		int nthreads = atoi(argv[1]);


		arg_t args[nthreads];
		pthread_t tid[nthreads];
		pthread_attr_t attr;
		pthread_barrier_t barrier;
		cpu_set_t set;

		// start from CPU0/CPUX to make sure the data is generater there.
		cpu_set_t startset;
		CPU_ZERO(&startset);
		CPU_SET(0, &startset);
		if(sched_setaffinity(0, sizeof(startset), &startset) <0)
		{
			perror("sched_setaffinity\n");
		}

		int64_t sizethr = ARRAY_SIZE/nthreads;
		int64_t sizeA = ARRAY_SIZE;
		int64_t sizeB = ARRAY_SIZE;
		int64_t sizeC = ARRAY_SIZE;

#if OPERATION_TYPE==READ
		printf("Operation type is \'read only\'\n");
#elif OPERATION_TYPE==WRITE
		printf("Operation type is \'write only\'\n");
#elif OPERATION_TYPE==SELFINC
		printf("Operation type is \'selfinc\'\n");
#elif OPERATION_TYPE==COPY
		printf("Operation type is \'copy\'\n");
#elif OPERATION_TYPE==MUL
		printf("Operation type is \'multiply\'\n");
#elif OPERATION_TYPE==SELFADD
		printf("Operation type is \'selfadd\'\n");
#elif OPERATION_TYPE==ADD
		printf("Operation type is \'add\'\n");
#elif OPERATION_TYPE==MULADD
		printf("Operation type is \'muladd\'\n");
#elif OPERATION_TYPE==SELFDADD
		printf("Operation type is \'selfdadd\'\n");
#else
        #error "Operation Code Error: OP should be from 0~7"
#endif

//		srand((unsigned)time(NULL));
		srand(12345);
#ifdef ARRAY_INFO
		printf("ARRAY A: ");
#endif
		data_t * arrayA = initialArray(sizeA);
#ifdef ARRAY_INFO
		printf("ARRAY B: ");
#endif
		data_t * arrayB = initialArray(sizeB);
#ifdef ARRAY_INFO
		printf("ARRAY C: ");
#endif
		data_t * arrayC = initialArray(sizeC);


		// initial the barrier
		int rv = pthread_barrier_init(&barrier,NULL,nthreads);
		if(rv != 0)
		{
			printf("Could not create the barrier!\n");
			exit(EXIT_FAILURE);
		}

		// initial the attr
		pthread_attr_init(&attr);

		int num_cpus = sysconf( _SC_NPROCESSORS_ONLN );

		// Timing information
		gettimeofday(&start_time,NULL);

		// Build and probe the hash table in threads
		for(int i=0;i<nthreads;i++)
		{
			int cpu_idx = get_cpuid(i%num_cpus);
		//	int cpu_idx = i%num_cpus;
			CPU_ZERO(&set);
			CPU_SET(cpu_idx,&set);
			pthread_attr_setaffinity_np(&attr,sizeof(cpu_set_t),&set);

			args[i].tid = i;
			args[i].barrier = &barrier;

			args[i].arrayA = arrayA + sizethr * i;
			args[i].arrayB = arrayB + sizethr * i;
			args[i].arrayC = arrayC + sizethr * i;

			args[i].sizeA = (i==(nthreads-1))?sizeA:sizethr;
			args[i].sizeB = (i==(nthreads-1))?sizeB:sizethr;
			args[i].sizeC = (i==(nthreads-1))?sizeC:sizethr;

			sizeA -= sizethr;
			sizeB -= sizethr;
			sizeC -= sizethr;

			rv = pthread_create(&tid[i], &attr, pstream_thread, (void*)&args[i]);
			if(rv)
			{
				printf("Thread create error with code: %d\n",rv);
				exit(-1);
			}
		}

		// join the threads
		for(int i=0;i<nthreads;i++)
		{
			pthread_join(tid[i],NULL);
		}

		// Timing information
		gettimeofday(&end_time,NULL);

		free_array(arrayA);
		free_array(arrayB);
		free_array(arrayC);

//		printf("The program runs for %ld microsec\n",(end_time.tv_sec-start_time.tv_sec)*1000000L+end_time.tv_usec-start_time.tv_usec);
	}

	return 0;
}
