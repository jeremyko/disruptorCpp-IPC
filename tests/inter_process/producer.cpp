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

using namespace std;

int gTestIndex;
int SUM_TILL_THIS ;

//SharedMemRingBuffer gSharedMemRingBuffer (YIELDING_WAIT); 
//SharedMemRingBuffer gSharedMemRingBuffer (SLEEPING_WAIT); 
SharedMemRingBuffer gSharedMemRingBuffer (BLOCKING_WAIT); 

///////////////////////////////////////////////////////////////////////////////
void TestFunc()
{
    int64_t nMyIndex = -1;

    for(int i=1; i <= SUM_TILL_THIS  ; i++) 
    {
        OneBufferData my_data;
        nMyIndex = gSharedMemRingBuffer.ClaimIndex(0);
        my_data.producerId = 0;
        my_data.nData = i ;

#ifdef _DEBUG_WRITE_
        char szMsg[1024];
        snprintf(szMsg, sizeof(szMsg), 
                "[id:%d] [%s-%d] write data / nMyIndex[%" PRId64 "] data [%d]", 
                nMyId, __func__, __LINE__, nMyIndex, i );
        {AtomicPrint atomicPrint(szMsg);}
#endif

        gSharedMemRingBuffer.SetData( nMyIndex, &my_data );
        gSharedMemRingBuffer.Commit(0, nMyIndex);
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    int MAX_TEST = 1;
    SUM_TILL_THIS = 10000;
    int MAX_RBUFFER_CAPACITY = 4096; 

    if(! gSharedMemRingBuffer.InitRingBuffer(MAX_RBUFFER_CAPACITY) )
    { 
        //Error!
        return 1; 
    }

    ElapsedTime elapsed;

    for ( gTestIndex=0; gTestIndex < MAX_TEST; gTestIndex++)
    {
        TestFunc();

        long long nElapsedMicro= elapsed.SetEndTime(MICRO_SEC_RESOLUTION);
        std::cout << "**** procucer test " << gTestIndex << " / count:"
                  << SUM_TILL_THIS << " -> elapsed : "<< nElapsedMicro << "(micro sec) /"
                  << (long long) (10000L*1000000L)/nElapsedMicro <<" TPS\n";
    }


    return 0;
}

