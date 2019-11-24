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

#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <fstream>

#include "../../ring_buffer_on_shmem.hpp" 
#include "../../shared_mem_manager.hpp" 
#include "../../atomic_print.hpp"
#include "../../elapsed_time.hpp"

size_t g_test_index     =0;
size_t MAX_PRODUCER_CNT =0; 
size_t MAX_CONSUMER_CNT =0;
size_t SUM_TILL_THIS    =0;
size_t SUM_ALL          =0; 

//SharedMemRingBuffer g_shared_mem_ring_buffer (YIELDING_WAIT); 
//SharedMemRingBuffer g_shared_mem_ring_buffer (SLEEPING_WAIT); 
SharedMemRingBuffer g_shared_mem_ring_buffer (BLOCKING_WAIT); 

///////////////////////////////////////////////////////////////////////////////
void ThreadWorkWrite(std::string tid, int my_id) 
{
    int64_t my_index = -1;
    for(size_t i=1; i <= SUM_TILL_THIS  ; i++) {
        OneBufferData my_data;
        my_index = g_shared_mem_ring_buffer.ClaimIndex(my_id);
        my_data.producer_id = my_id;
        my_data.data = i ;
        g_shared_mem_ring_buffer.SetData( my_index, &my_data );
        g_shared_mem_ring_buffer.Commit(my_id, my_index); 
    }
#ifdef DEBUG_WRITE
    char msg_buffer[1024];
    snprintf(msg_buffer, sizeof(msg_buffer), 
            "[id:%d]    [%s-%d] Write Done", my_id, __func__, __LINE__ );
    {AtomicPrint atomicPrint(msg_buffer);}
#endif
}

///////////////////////////////////////////////////////////////////////////////
void ThreadWorkRead(std::string tid, int my_id, int64_t index_for_customer_use) 
{
    char msg_buffer[1024];
    size_t  sum =  0;
    size_t  total_fetched = 0; 
    int64_t my_index = index_for_customer_use ; 

    for(size_t i=0; i < MAX_PRODUCER_CNT * SUM_TILL_THIS  ; i++) {
        if( total_fetched >= (MAX_PRODUCER_CNT*  SUM_TILL_THIS) ) {
            break;
        }
        int64_t nReturnedIndex = g_shared_mem_ring_buffer.WaitFor(my_id, my_index);
#ifdef DEBUG_READ
        snprintf(msg_buffer, sizeof(msg_buffer), 
            "[id:%d]    \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d] WaitFor my_index"
            "[%" PRId64 "] nReturnedIndex[%" PRId64 "]", 
            my_id, __func__, __LINE__, my_index, nReturnedIndex );
        {AtomicPrint atomicPrint(msg_buffer);}
#endif
        for(int64_t j = my_index; j <= nReturnedIndex; j++) {
            //batch job 
            OneBufferData* pData =  g_shared_mem_ring_buffer.GetData(j);
            sum += pData->data ;
#ifdef DEBUG_READ
            snprintf(msg_buffer, sizeof(msg_buffer), 
                "[id:%d]   \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d]  my_index"
                "[%" PRId64 ", translated:%" PRId64 "] data [%" PRId64 "] ^^", 
                my_id, __func__, __LINE__, j, g_shared_mem_ring_buffer.GetTranslatedIndex(j), pData->data );
            {AtomicPrint atomicPrint(msg_buffer);}
#endif
            g_shared_mem_ring_buffer.CommitRead(my_id, j );
            total_fetched++;
        } //for
        my_index = nReturnedIndex + 1; 
    }
    if(sum != SUM_ALL) {
        snprintf(msg_buffer, sizeof(msg_buffer), 
            "\t\t\t\t\t\t\t\t\t\t\t\t********* SUM ERROR : ThreadWorkRead "
            "[id:%d] Exiting, Sum =%lu / g_test_index[%lu]", 
            my_id, sum, g_test_index);
        {AtomicPrint atomicPrint(msg_buffer);}
        exit(0);
    } else {
        snprintf(msg_buffer, sizeof(msg_buffer), 
            "\t\t\t\t\t\t\t\t\t\t\t\t********* SUM OK : ThreadWorkRead "
            "[id:%d] Sum =%lu / g_test_index[%lu]", my_id, sum,g_test_index );
        {AtomicPrint atomicPrint(msg_buffer);}
    }
}

///////////////////////////////////////////////////////////////////////////////
void TestFunc()
{
    std::vector<std::thread> consumer_threads ;
    std::vector<std::thread> producer_threads ;
    //Consumer
    //1. register
    std::vector<int64_t> vec_consumer_indexes ;
    for(size_t i = 0; i < MAX_CONSUMER_CNT; i++) {
        int64_t index_for_customer_use = -1;
        if(!g_shared_mem_ring_buffer.RegisterConsumer(i, &index_for_customer_use )) {
            return; //error
        }
        DEBUG_LOG("index_for_customer_use = " <<index_for_customer_use );
        vec_consumer_indexes.push_back(index_for_customer_use);
    }
    //2. run
    for(size_t i = 0; i < MAX_CONSUMER_CNT; i++) {
        consumer_threads.push_back (std::thread (ThreadWorkRead, "consumer", i, vec_consumer_indexes[i] ) );
    }
    //producer run
    for(size_t i = 0; i < MAX_PRODUCER_CNT; i++) {
        producer_threads.push_back (std::thread (ThreadWorkWrite, "procucer", i ) );
    }
    for(size_t i = 0; i < MAX_PRODUCER_CNT; i++) {
        producer_threads[i].join();
    }
    for(size_t i = 0; i < MAX_CONSUMER_CNT; i++) {
        consumer_threads[i].join();
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    size_t MAX_TEST = 1;
    SUM_TILL_THIS = 10;
    size_t MAX_RBUFFER_CAPACITY = 4096*4; 
    MAX_PRODUCER_CNT = 1; 
    MAX_CONSUMER_CNT = 10; 
    SUM_ALL =  ( ( SUM_TILL_THIS * ( SUM_TILL_THIS + 1 ) ) /2 ) ; 
    SUM_ALL *= MAX_PRODUCER_CNT; 
    std::cout << "********* SUM_ALL : " << SUM_ALL << '\n';

    if(! g_shared_mem_ring_buffer.InitRingBuffer(MAX_RBUFFER_CAPACITY) ) { 
        //Error!
        return 1; 
    }
    ElapsedTime elapsed;
    for ( g_test_index=0; g_test_index < MAX_TEST; g_test_index++) {
        TestFunc();
    }
    long long nElapsedMicro= elapsed.SetEndTime(MICRO_SEC_RESOLUTION);
    std::cout << "**** test " << g_test_index << " / count:"<< SUM_TILL_THIS 
        << " -> elapsed : "<< nElapsedMicro << "(micro sec) /"
        << (long long) (10000L*1000000L)/nElapsedMicro <<" TPS\n";
    return 0;
}

