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

using namespace std;

int gTestIndex;
int MAX_PRODUCER_CNT ; 
int MAX_CONSUMER_CNT;
int SUM_TILL_THIS ;
int64_t SUM_ALL ; 

//SharedMemRingBuffer gSharedMemRingBuffer (YIELDING_WAIT); 
//SharedMemRingBuffer gSharedMemRingBuffer (SLEEPING_WAIT); 
SharedMemRingBuffer gSharedMemRingBuffer (BLOCKING_WAIT); 

///////////////////////////////////////////////////////////////////////////////
void ThreadWorkWrite(string tid, int nMyId) 
{
    int64_t nMyIndex = -1;

    for(int i=1; i <= SUM_TILL_THIS  ; i++) 
    {
        OneBufferData my_data;
        nMyIndex = gSharedMemRingBuffer.ClaimIndex(nMyId);
        my_data.producerId = nMyId;
        my_data.nData = i ;

        gSharedMemRingBuffer.SetData( nMyIndex, &my_data );

        gSharedMemRingBuffer.Commit(nMyId, nMyIndex); 
    }

#ifdef _DEBUG_WRITE
    char szMsg[1024];
    snprintf(szMsg, sizeof(szMsg), 
            "[id:%d]    [%s-%d] Write Done", nMyId, __func__, __LINE__ );
    {AtomicPrint atomicPrint(szMsg);}
#endif

}

///////////////////////////////////////////////////////////////////////////////
void ThreadWorkRead(string tid, int nMyId, int64_t nIndexforCustomerUse) 
{
    char szMsg[1024];
    int64_t nSum =  0;
    int64_t nTotalFetched = 0; 
    int64_t nMyIndex = nIndexforCustomerUse ; 

    for(int i=0; i < MAX_PRODUCER_CNT * SUM_TILL_THIS  ; i++)
    {
        if( nTotalFetched >= (MAX_PRODUCER_CNT*  SUM_TILL_THIS) ) 
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
void TestFunc()
{
    std::vector<std::thread> consumerThreads ;
    std::vector<std::thread> producerThreads ;

    //Consumer
    //1. register
    std::vector<int64_t> vecConsumerIndexes ;
    for(int i = 0; i < MAX_CONSUMER_CNT; i++)
    {
        int64_t nIndexforCustomerUse = -1;
        if(!gSharedMemRingBuffer.RegisterConsumer(i, &nIndexforCustomerUse ) )
        {
            return; //error
        }
        vecConsumerIndexes.push_back(nIndexforCustomerUse);
    }

    //2. run
    for(int i = 0; i < MAX_CONSUMER_CNT; i++)
    {
        consumerThreads.push_back (std::thread (ThreadWorkRead, "consumer", i, vecConsumerIndexes[i] ) );
    }

    //producer run
    for(int i = 0; i < MAX_PRODUCER_CNT; i++)
    {
        producerThreads.push_back (std::thread (ThreadWorkWrite, "procucer", i ) );
    }

    for(int i = 0; i < MAX_PRODUCER_CNT; i++)
    {
        producerThreads[i].join();
    }

    for(int i = 0; i < MAX_CONSUMER_CNT; i++)
    {
        consumerThreads[i].join();
    }
}


///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    int MAX_TEST = 10;
    SUM_TILL_THIS = 10000;
    int MAX_RBUFFER_CAPACITY = 4096*4; 
    MAX_PRODUCER_CNT = 2; 
    MAX_CONSUMER_CNT = 10; 
    SUM_ALL =  ( ( SUM_TILL_THIS * ( SUM_TILL_THIS + 1 ) ) /2 ) ; 
    SUM_ALL *= MAX_PRODUCER_CNT; 

    std::cout << "********* SUM_ALL : " << SUM_ALL << '\n';

    if(! gSharedMemRingBuffer.InitRingBuffer(MAX_RBUFFER_CAPACITY) )
    { 
        //Error!
        return 1; 
    }

    ElapsedTime elapsed;

    for ( gTestIndex=0; gTestIndex < MAX_TEST; gTestIndex++)
    {
        TestFunc();
    }

    long long nElapsedMicro= elapsed.SetEndTime(MICRO_SEC_RESOLUTION);
    std::cout << "**** test " << gTestIndex << " / count:"<< SUM_TILL_THIS << " -> elapsed : "<< nElapsedMicro << "(micro sec) /"
        << (long long) (10000L*1000000L)/nElapsedMicro <<" TPS\n";

    return 0;
}

