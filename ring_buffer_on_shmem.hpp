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

#ifndef DISRUPTORCPP_RING_BUFFER_ON_SHM_HPP
#define DISRUPTORCPP_RING_BUFFER_ON_SHM_HPP

#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <inttypes.h>
#include "common_def.hpp"
#include "ring_buffer.hpp"
#include "shared_mem_manager.hpp" 
#include "wait_strategy.hpp"

///////////////////////////////////////////////////////////////////////////////
class SharedMemRingBuffer
{
    public:
        SharedMemRingBuffer(ENUM_WAIT_STRATEGY wait_strategy);
        ~SharedMemRingBuffer();

        bool     InitRingBuffer(int size=DEFAULT_RING_BUFFER_SIZE);
        void     ResetRingBufferState();
        bool     TerminateRingBuffer();
        bool     SetData( int64_t index, OneBufferData* data);
        OneBufferData*  GetData(int64_t index);

        bool     RegisterConsumer (int id, int64_t* index_for_customer);
        int64_t  GetTranslatedIndex( int64_t sequence);
        void     SignalAll(); 
 
        //producer
        int64_t  ClaimIndex(int caller_id);
        bool     Commit(int user_id, int64_t index);
        
        //consumer
        int64_t  WaitFor(int user_id, int64_t index);
        bool     CommitRead(int user_id, int64_t index);

    private:
        int64_t GetMinIndexOfConsumers();
        int64_t GetNextSequenceForClaim();
        //no copy allowed
        SharedMemRingBuffer(SharedMemRingBuffer&) = delete;   
        void operator=(SharedMemRingBuffer) = delete;

    private:
        size_t  buffer_size_    ;
        size_t  total_mem_size_ ;
        RingBuffer<OneBufferData*>      ring_buffer_ ; 
        WaitStrategyInterface*          wait_strategy_ ;
        SharedMemoryManager             shared_mem_mgr_;
        ENUM_WAIT_STRATEGY              wait_strategy_type_;
        RingBufferStatusOnSharedMem*    ring_buffer_status_on_shared_mem_; 
    
};

#endif //DISRUPTORCPP_RING_BUFFER_HPP

