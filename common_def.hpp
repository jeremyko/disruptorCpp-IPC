/****************************************************************************
 Copyright (c) 2015, ko jung hyun
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef COMMON_DEF_HPP
#define COMMON_DEF_HPP

#include <atomic>
#include <iostream>
#include <condition_variable>
#include <pthread.h> //blocking strategy : mutex, condition_var on shared memory
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#define CACHE_LINE_SIZE 64 
#define MAX_CONSUMER    200
#define DEFAULT_RING_BUFFER_SIZE  1024

///////////////////////////////////////////////////////////////////////////////
//color printf
#define COLOR_RED  "\x1B[31m"
#define COLOR_GREEN "\x1B[32m" 
#define COLOR_BLUE "\x1B[34m"
#define COLOR_RESET "\x1B[0m"
#define  LOG_WHERE "("<<__FILE__<<"-"<<__func__<<"-"<<__LINE__<<") "
#define  WHERE_DEF __FILE__,__func__,__LINE__

#ifdef DEBUG_PRINTF
#define  DEBUG_LOG(x)  std::cout<<LOG_WHERE << x << "\n"
#define  DEBUG_RED_LOG(x) std::cout<<LOG_WHERE << COLOR_RED<< x << COLOR_RESET << "\n"
#define  DEBUG_GREEN_LOG(x) std::cout<<LOG_WHERE << COLOR_GREEN<< x << COLOR_RESET << "\n"
#define  DEBUG_ELOG(x) std::cerr<<LOG_WHERE << COLOR_RED<< x << COLOR_RESET << "\n"
#else
#define  DEBUG_LOG(x) 
#define  DEBUG_ELOG(x) 
#define  DEBUG_RED_LOG(x) 
#define  DEBUG_GREEN_LOG(x)
#endif
#define  PRINT_ELOG(x) std::cerr<<LOG_WHERE << COLOR_RED<< x << COLOR_RESET << "\n"

///////////////////////////////////////////////////////////////////////////////
typedef struct _OneBufferData_
{
    int64_t data;
    size_t  producer_id;
} OneBufferData ;
const size_t LEN_ONE_BUFFER_DATA = sizeof(OneBufferData);

typedef struct _RingBufferStatusOnSharedMem_
{
    size_t  buffer_size   ;
    size_t  total_mem_size ;
    std::atomic<size_t> registered_producer_count ;
    std::atomic<size_t> registered_consumer_count;
    std::atomic<int64_t> cursor  alignas(CACHE_LINE_SIZE);  
    std::atomic<int64_t> next    alignas(CACHE_LINE_SIZE); 
    int64_t array_of_consumer_indexes [MAX_CONSUMER] __attribute__ ((aligned (CACHE_LINE_SIZE)));
    //TODO use uint64_t
    pthread_cond_t   cond_var;
    pthread_mutex_t  mtx_lock;
} RingBufferStatusOnSharedMem ;

#endif //COMMON_DEF_HPP

