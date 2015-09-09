
#ifndef __COMMON_DEF_HPP__
#define __COMMON_DEF_HPP__

#include <atomic>
//#include <mutex>
#include <condition_variable>

#include <pthread.h> //blocking strategy : mutex, condition_var on shared memory
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 

typedef struct _OneBufferData_
{
    int64_t  nData;
    int  producerId;

} OneBufferData ;

#define MAX_CONSUMER 200
typedef struct _RingBufferStatusOnSharedMem_
{
    int  nBufferSize   ;
    int  nTotalMemSize ;
    std::atomic<int> registered_producer_count ;
    std::atomic<int> registered_consumer_count;
    std::atomic<int64_t> cursor  __attribute__ ((aligned (64))) ;
    std::atomic<int64_t> next    __attribute__ ((aligned (64))) ;
    int64_t arrayOfConsumerIndexes [MAX_CONSUMER] __attribute__ ((aligned (64)));

    pthread_cond_t   condVar;
    pthread_mutex_t  mtxLock;


} RingBufferStatusOnSharedMem ;

#endif
