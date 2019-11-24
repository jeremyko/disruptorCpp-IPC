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
#include <thread>

#include "../../ring_buffer_on_shmem.hpp" 
#include "../../shared_mem_manager.hpp" 
#include "../../atomic_print.hpp"
#include "../../elapsed_time.hpp"

size_t   g_test_index  = 0;
size_t   SUM_TILL_THIS = 0;
size_t   SUM_ALL       = 0; 

//Wait Strategy 
//SharedMemRingBuffer g_shared_mem_ring_buffer (YIELDING_WAIT); 
//SharedMemRingBuffer g_shared_mem_ring_buffer (SLEEPING_WAIT); 
SharedMemRingBuffer g_shared_mem_ring_buffer (BLOCKING_WAIT); 

ElapsedTime g_elapsed;

///////////////////////////////////////////////////////////////////////////////
void TestFunc(int customer_id)
{
    //1. register
    int64_t index_for_customer_use = -1;
    if(!g_shared_mem_ring_buffer.RegisterConsumer(customer_id, &index_for_customer_use )) {
        return; //error
    }
    //2. run
    size_t   sum =  0;
    size_t   total_fetched = 0; 
    int64_t  my_index = index_for_customer_use ; 
    bool is_first=true;
    char msg_buffer[1024];

    for(size_t i=0; i < SUM_TILL_THIS   ; i++) {
        if( total_fetched >= SUM_TILL_THIS ) {
            break;
        }
        int64_t returned_index = g_shared_mem_ring_buffer.WaitFor(customer_id, my_index);
        if(is_first) {
            is_first=false;
            g_elapsed.SetStartTime();
        }
#ifdef DEBUG_READ
        snprintf(msg_buffer, sizeof(msg_buffer), 
                "[id:%d]    \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d] WaitFor "
                "my_index[%" PRId64 "] returned_index[%" PRId64 "]", 
                customer_id, __func__, __LINE__, my_index, returned_index );
        {AtomicPrint atomicPrint(msg_buffer);}
#endif

        for(int64_t j = my_index; j <= returned_index; j++) {
            //batch job 
            OneBufferData* pData =  g_shared_mem_ring_buffer.GetData(j);
            sum += pData->data ;
#ifdef DEBUG_READ
            snprintf(msg_buffer, sizeof(msg_buffer), 
                "[id:%d]   \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d]  "
                "my_index[%" PRId64 ", translated:%" PRId64 "] data [%" PRId64 "] ^^", 
                customer_id, __func__, __LINE__, j, 
                g_shared_mem_ring_buffer.GetTranslatedIndex(j), pData->data );
            {AtomicPrint atomicPrint(msg_buffer);}
#endif
            g_shared_mem_ring_buffer.CommitRead(customer_id, j );
            total_fetched++;
        } //for
        my_index = returned_index + 1; 
    } //for

    long long nElapsedMicro= g_elapsed.SetEndTime(MICRO_SEC_RESOLUTION);
    snprintf(msg_buffer, sizeof(msg_buffer), "**** consumer test %lu count %lu -> "
            "elapsed :%lld (micro sec) TPS %lld", 
        g_test_index , SUM_TILL_THIS , nElapsedMicro, 
        (long long) (10000L*1000000L)/nElapsedMicro );
    DEBUG_LOG(msg_buffer);

    if(sum != SUM_ALL) {
        snprintf(msg_buffer, sizeof(msg_buffer), 
            "\t\t\t\t\t\t\t\t\t\t\t\t********* SUM ERROR : ThreadWorkRead "
            "[id:%d] Exiting, Sum =%" PRId64 " / g_test_index[%lu]", 
            customer_id, sum, g_test_index);
        {AtomicPrint atomicPrint(msg_buffer);}
        exit(0);
    } else {
        snprintf(msg_buffer, sizeof(msg_buffer), 
                "\t\t\t\t\t\t\t\t\t\t\t\t********* SUM OK : ThreadWorkRead "
                "[id:%d] Sum =%" PRId64 " / g_test_index[%lu]", 
                customer_id, sum,g_test_index );
        {AtomicPrint atomicPrint(msg_buffer);}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    if(argc != 2) {
        std::cout << "usage: "<< argv[0]<<" customer_index"  << '\n';
        std::cout << "ex: "<< argv[0]<<" 0"  << '\n';
        std::cout << "ex: "<< argv[0]<<" 1"  << '\n';
        return 1;
    }
    int customer_id = atoi(argv[1]);
    size_t MAX_TEST = 1;
    SUM_TILL_THIS = 10000;
    size_t MAX_RBUFFER_CAPACITY = 4096; 
    SUM_ALL =  ( ( SUM_TILL_THIS * ( SUM_TILL_THIS + 1 ) ) /2 ) ; 
    std::cout << "********* SUM_ALL : " << SUM_ALL << '\n';

    if(! g_shared_mem_ring_buffer.InitRingBuffer(MAX_RBUFFER_CAPACITY) ) { 
        //Error!
        return 1; 
    }
    for ( g_test_index=0; g_test_index < MAX_TEST; g_test_index++) {
        TestFunc(customer_id);
    }
    return 0;
}

