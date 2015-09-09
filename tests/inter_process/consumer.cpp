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
int64_t SUM_ALL ; 

//SharedMemRingBuffer gSharedMemRingBuffer (YIELDING_WAIT); 
//SharedMemRingBuffer gSharedMemRingBuffer (SLEEPING_WAIT); 
SharedMemRingBuffer gSharedMemRingBuffer (BLOCKING_WAIT); 

///////////////////////////////////////////////////////////////////////////////
void ThreadWorkRead(string tid, int nMyId, int64_t nIndexforCustomerUse) 
{
    char szMsg[1024];
    int64_t nSum =  0;
    int64_t nTotalFetched = 0; 
    int64_t nMyIndex = nIndexforCustomerUse ; 

    for(int i=0; i < SUM_TILL_THIS   ; i++)
    {
        if( nTotalFetched >= SUM_TILL_THIS ) 
        {
            break;
        }

        int64_t nReturnedIndex = gSharedMemRingBuffer.WaitFor(nMyId, nMyIndex);

#ifdef _DEBUG_READ
        snprintf(szMsg, sizeof(szMsg), 
                "[id:%d]    \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d] WaitFor nMyIndex[%" PRId64 "] nReturnedIndex[%" PRId64 "]", 
                nMyId, __func__, __LINE__, nMyIndex, nReturnedIndex );
        {AtomicPrint atomicPrint(szMsg);}
#endif

        for(int64_t j = nMyIndex; j <= nReturnedIndex; j++)
        {
            //batch job 
            OneBufferData* pData =  gSharedMemRingBuffer.GetData(j);

            nSum += pData->nData ;

#ifdef _DEBUG_READ
            snprintf(szMsg, sizeof(szMsg), 
                    "[id:%d]   \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d]  nMyIndex[%" PRId64 ", translated:%" PRId64 "] data [%" PRId64 "] ^^", 
                    nMyId, __func__, __LINE__, j, gSharedMemRingBuffer.GetTranslatedIndex(j), pData->nData );
            {AtomicPrint atomicPrint(szMsg);}
#endif

            gSharedMemRingBuffer.CommitRead(nMyId, j );
            nTotalFetched++;

        } //for

        nMyIndex = nReturnedIndex + 1; 
    }

    if(nSum != SUM_ALL)
    {
        snprintf(szMsg, sizeof(szMsg), 
            "\t\t\t\t\t\t\t\t\t\t\t\t********* SUM ERROR : ThreadWorkRead [id:%d] Exiting, Sum =%" PRId64 " / gTestIndex[%d]", 
            nMyId, nSum, gTestIndex);
        {AtomicPrint atomicPrint(szMsg);}

        exit(0);
    }
    else
    {
#ifdef _DEBUG_RSLT_
        snprintf(szMsg, sizeof(szMsg), 
            "\t\t\t\t\t\t\t\t\t\t\t\t********* SUM OK : ThreadWorkRead [id:%d] Sum =%" PRId64 " / gTestIndex[%d]", nMyId, nSum,gTestIndex );
        {AtomicPrint atomicPrint(szMsg);}
#endif
    }
}

///////////////////////////////////////////////////////////////////////////////
void TestFunc(int nCustomerId)
{
    std::vector<std::thread> consumerThreads ;

    //Consumer
    //1. register
    std::vector<int64_t> vecConsumerIndexes ;
    int64_t nIndexforCustomerUse = -1;
    if(!gSharedMemRingBuffer.RegisterConsumer(nCustomerId, &nIndexforCustomerUse ) )
    {
        return; //error
    }
    vecConsumerIndexes.push_back(nIndexforCustomerUse);

    //2. run
    consumerThreads.push_back (std::thread (ThreadWorkRead, "consumer", nCustomerId, vecConsumerIndexes[0] ) );

    consumerThreads[0].join();
}


///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout << "usage: "<< argv[0]<<" customer_index"  << '\n';
        std::cout << "ex: "<< argv[0]<<" 0"  << '\n';
        std::cout << "ex: "<< argv[0]<<" 1"  << '\n';
        return 1;
    }

    int nCustomerId = atoi(argv[1]);
    int MAX_TEST = 1;
    SUM_TILL_THIS = 10000;
    int MAX_RBUFFER_CAPACITY = 4096; 
    SUM_ALL =  ( ( SUM_TILL_THIS * ( SUM_TILL_THIS + 1 ) ) /2 ) ; 

    std::cout << "********* SUM_ALL : " << SUM_ALL << '\n';

    if(! gSharedMemRingBuffer.InitRingBuffer(MAX_RBUFFER_CAPACITY) )
    { 
        //Error!
        return 1; 
    }

    ElapsedTime elapsed;

    for ( gTestIndex=0; gTestIndex < MAX_TEST; gTestIndex++)
    {
        TestFunc(nCustomerId);
    }

    long long nElapsedMicro= elapsed.SetEndTime(MICRO_SEC_RESOLUTION);
    std::cout << "**** consumer test " << gTestIndex << " / count:"<< SUM_TILL_THIS << " -> elapsed : "<< nElapsedMicro << "(micro sec) /"
        << (long long) (10000L*1000000L)/nElapsedMicro <<" TPS\n";

    return 0;
}

