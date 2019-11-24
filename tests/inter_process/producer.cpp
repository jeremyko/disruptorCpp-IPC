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

int g_test_index = 0 ;
size_t SUM_TILL_THIS = 0;

//SharedMemRingBuffer g_shared_mem_ring_buffer (YIELDING_WAIT); 
//SharedMemRingBuffer g_shared_mem_ring_buffer (SLEEPING_WAIT); 
SharedMemRingBuffer g_shared_mem_ring_buffer (BLOCKING_WAIT); 

///////////////////////////////////////////////////////////////////////////////
void TestFunc()
{
    int64_t my_index = -1;
    for(size_t i=1; i <= SUM_TILL_THIS  ; i++) {
        OneBufferData my_data;
        my_index = g_shared_mem_ring_buffer.ClaimIndex(0);
        my_data.producer_id = 0;
        my_data.data = i ;
#ifdef DEBUG_WRITE_
        char msg_buffer[1024];
        snprintf(msg_buffer, sizeof(msg_buffer), 
                "[id:%d] [%s-%d] write data / my_index[%" PRId64 "] data [%d]", 
                0, __func__, __LINE__, my_index, i );
        {AtomicPrint atomicPrint(msg_buffer);}
#endif
        g_shared_mem_ring_buffer.SetData( my_index, &my_data );
        g_shared_mem_ring_buffer.Commit(0, my_index);
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    int MAX_TEST = 1;
    SUM_TILL_THIS = 10000;
    int MAX_RBUFFER_CAPACITY = 4096; 
    if(! g_shared_mem_ring_buffer.InitRingBuffer(MAX_RBUFFER_CAPACITY) ) { 
        //Error!
        return 1; 
    }
    ElapsedTime elapsed;
    for ( g_test_index=0; g_test_index < MAX_TEST; g_test_index++) {
        TestFunc();
        long long nElapsedMicro= elapsed.SetEndTime(MICRO_SEC_RESOLUTION);
        std::cout << "**** procucer test " << g_test_index << " / count:"
                  << SUM_TILL_THIS << " -> elapsed : "<< nElapsedMicro << "(micro sec) /"
                  << (long long) (10000L*1000000L)/nElapsedMicro <<" TPS\n";
    }
    return 0;
}

