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

#ifndef __DISRUPTORCPP_RING_BUFFER_ON_SHM_HPP__
#define __DISRUPTORCPP_RING_BUFFER_ON_SHM_HPP__
///////////////////////////////////////////////////////////////////////////////
//20150721 kojh create
///////////////////////////////////////////////////////////////////////////////


#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <inttypes.h>
#include "common_def.hpp"
#include "ring_buffer.hpp"
#include "shared_mem_manager.hpp" 
#include "wait_strategy.hpp"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
class SharedMemRingBuffer
{
    public:
        SharedMemRingBuffer(ENUM_WAIT_STRATEGY waitStrategyType);
        ~SharedMemRingBuffer();

        bool     InitRingBuffer(int nSize=DEFAULT_RING_BUFFER_SIZE);
        void     ResetRingBufferState();
        bool     TerminateRingBuffer();
        bool     SetData( int64_t nIndex, OneBufferData* pData);
        OneBufferData*  GetData(int64_t nIndex);

        bool     RegisterConsumer (int nId, int64_t* nIndexforCustomerUse);
        int64_t  GetTranslatedIndex( int64_t sequence);
        void     SignalAll(); 
 
        //producer
        int64_t  ClaimIndex(int nCallerId);
        bool     Commit(int nUserId, int64_t index);
        
        //consumer
        int64_t  WaitFor(int nUserId, int64_t index);
        bool     CommitRead(int nUserId, int64_t index);

    private:

        SharedMemoryManager sharedMemoryManager_;
        RingBufferStatusOnSharedMem* pRingBufferStatusOnSharedMem_; 
        int  nBufferSize_;
        int  nTotalMemSize_ ;
        int64_t GetMinIndexOfConsumers();
        int64_t GetNextSequenceForClaim();
        ENUM_WAIT_STRATEGY waitStrategyType_;
        WaitStrategyInterface* pWaitStrategy_ ;
        RingBuffer< OneBufferData* > ringBuffer_ ; 
    
        //no copy allowed
        SharedMemRingBuffer(SharedMemRingBuffer&) = delete;   
        void operator=(SharedMemRingBuffer) = delete;
};

#endif

