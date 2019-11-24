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

#include "ring_buffer_on_shmem.hpp" 
#include "atomic_print.hpp"
#include "common_def.hpp" 

std::mutex AtomicPrint::lock_mutex_ ;

///////////////////////////////////////////////////////////////////////////////
SharedMemRingBuffer::SharedMemRingBuffer(ENUM_WAIT_STRATEGY wait_strategy)
{
    ring_buffer_status_on_shared_mem_ = NULL;
    wait_strategy_type_ = wait_strategy ;
    wait_strategy_ = NULL;
}

///////////////////////////////////////////////////////////////////////////////
SharedMemRingBuffer::~SharedMemRingBuffer()
{
    if(wait_strategy_) {
        delete wait_strategy_;
        wait_strategy_ = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::TerminateRingBuffer()
{
    if(! shared_mem_mgr_.DetachShMem()) {
        DEBUG_ELOG("Error"); 
        return false;
    }
    if(! shared_mem_mgr_.RemoveShMem()) {
        DEBUG_ELOG("Error"); 
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::RegisterConsumer (int id, int64_t* index_for_customer)
{
    if(ring_buffer_status_on_shared_mem_->array_of_consumer_indexes[id] == -1 ) {
        //처음 등록 
        ring_buffer_status_on_shared_mem_->registered_consumer_count++;
        if( ring_buffer_status_on_shared_mem_->registered_consumer_count >= MAX_CONSUMER) {
            DEBUG_ELOG("Error: Exceeds MAX_CONSUMER : " << MAX_CONSUMER); 
            return false;
        }
        if(ring_buffer_status_on_shared_mem_->cursor >= 0 ) {
            DEBUG_LOG("cursor >= 0"); 
            //데이터 전달 중이데 새로운 소비자가 추가
            ring_buffer_status_on_shared_mem_->array_of_consumer_indexes[id] = 
                        ring_buffer_status_on_shared_mem_->cursor.load() ; 
        } else {
            DEBUG_LOG("set 0 "); 
            ring_buffer_status_on_shared_mem_->array_of_consumer_indexes[id] = 0; 
        }
        *index_for_customer = ring_buffer_status_on_shared_mem_->array_of_consumer_indexes[id];
    } else {
        //last read message index 
        //기존 최종 업데이트 했던 인덱스 + 1 돌려준다. consumer가 호출할 인덱스 이므로 ..
        *index_for_customer = ring_buffer_status_on_shared_mem_->array_of_consumer_indexes[id] + 1;
    }
    DEBUG_LOG("USAGE_CONSUMER ID : " << id<< " / index : "<< *index_for_customer); 
    return true;
}

///////////////////////////////////////////////////////////////////////////////
void SharedMemRingBuffer::ResetRingBufferState() 
{
    if(ring_buffer_status_on_shared_mem_ == NULL ) {
        DEBUG_LOG("call InitRingBuffer first !"); 
        return;
    }
    DEBUG_LOG("---"); 
    ring_buffer_status_on_shared_mem_->cursor.store(-1);
    ring_buffer_status_on_shared_mem_->next.store(-1);
    ring_buffer_status_on_shared_mem_->registered_producer_count.store(0);
    ring_buffer_status_on_shared_mem_->registered_consumer_count.store(0);
    total_mem_size_ = 0;
    for(int i = 0; i < MAX_CONSUMER; i++) {
        ring_buffer_status_on_shared_mem_->array_of_consumer_indexes[i] = -1;
    }
    //for blocking wait strategy : shared mutex, shared cond var
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init( & ring_buffer_status_on_shared_mem_->mtx_lock, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init( & ring_buffer_status_on_shared_mem_->cond_var, &condAttr);
}


///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::InitRingBuffer(int size /*= DEFAULT_RING_BUFFER_SIZE*/)
{
    if(size<= 0) {
        DEBUG_ELOG("Error: Invalid size : " << size );
        return false;
    }
    buffer_size_ = size;
    if(!ring_buffer_.SetCapacity(size) ) {
        DEBUG_ELOG("Error: Invalid size : " << size );
        return false;
    }
    //shared memory consists of : RingBufferStatusOnSharedMem + actual data
    total_mem_size_ = sizeof(_RingBufferStatusOnSharedMem_)  + (sizeof(OneBufferData) * size) ;
    bool bSharedMemFirstCreated = false;
    if(! shared_mem_mgr_.CreateShMem(123456, total_mem_size_, &bSharedMemFirstCreated )) {
        DEBUG_ELOG("Error: CreateShMem failed :" <<shared_mem_mgr_.GetLastErrMsg());
        return false;
    }
    if(! shared_mem_mgr_.AttachShMem()) {
        DEBUG_ELOG("Error: AttachShMem failed :"<<shared_mem_mgr_.GetLastErrMsg());
        return false;
    }
    ring_buffer_status_on_shared_mem_ = (RingBufferStatusOnSharedMem*)shared_mem_mgr_. GetShMemStartAddr(); 
    if(bSharedMemFirstCreated) {
        ResetRingBufferState();
    }
    char* pBufferStart = (char*)shared_mem_mgr_.GetShMemStartAddr() + sizeof(_RingBufferStatusOnSharedMem_) ; 

    for(int i = 0; i < size; i++) {
        ring_buffer_[i] = (OneBufferData*) ( (char*)pBufferStart + (sizeof(OneBufferData)*i) ) ;
    }
    ring_buffer_status_on_shared_mem_->buffer_size = size;
    ring_buffer_status_on_shared_mem_->total_mem_size = total_mem_size_;
    //---------------------------------------------
    //wait strategy
    if(wait_strategy_type_ == BLOCKING_WAIT ) {
        DEBUG_LOG("Wait Strategy :BLOCKING_WAIT" ); 
        wait_strategy_ = new BlockingWaitStrategy(ring_buffer_status_on_shared_mem_);
    } else if(wait_strategy_type_ == YIELDING_WAIT ) {
        DEBUG_LOG("Wait Strategy :YIELDING_WAIT" ); 
        wait_strategy_ = new YieldingWaitStrategy(ring_buffer_status_on_shared_mem_);
    } else if(wait_strategy_type_ == SLEEPING_WAIT ) {
        DEBUG_LOG("Wait Strategy :SLEEPING_WAIT" ); 
        wait_strategy_ = new SleepingWaitStrategy(ring_buffer_status_on_shared_mem_);
    } else {
        DEBUG_ELOG("Error: Invalid Wait Strategy :" << wait_strategy_type_);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
OneBufferData*  SharedMemRingBuffer::GetData(int64_t index)
{
    return ring_buffer_[index];
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::GetTranslatedIndex( int64_t sequence)
{
    return ring_buffer_.GetTranslatedIndex(sequence);
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::SetData( int64_t index, OneBufferData* data)
{
    memcpy( ring_buffer_[index], data, LEN_ONE_BUFFER_DATA ); 
    return true;
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::GetMinIndexOfConsumers()
{
    int64_t min_index = INT64_MAX ;
    bool is_found = false;

    for(size_t i = 0; i < ring_buffer_status_on_shared_mem_->registered_consumer_count; i++) {
        int64_t index = ring_buffer_status_on_shared_mem_->array_of_consumer_indexes[i];
        if( index < min_index ) {
            min_index = index;
            is_found = true;
        }
    }
    if(! is_found) {
        return 0;
    }
    return min_index ;
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::GetNextSequenceForClaim()
{
    return ring_buffer_status_on_shared_mem_->next.fetch_add(1) + 1;
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::ClaimIndex(int caller_id )
{
    int64_t nNextSeqForClaim = GetNextSequenceForClaim() ;
    //다음 쓰기 위치 - 링버퍼 크기 = 링버퍼 한번 순회 이전위치, 
    //데이터 쓰기 작업이 링버퍼를 완전히 한바퀴 순회한 경우
    //여기서 더 진행하면 데이터를 이전 덮어쓰게 된다.
    //만약 아직 데이터를 read하지 못한 상태(min customer index가 wrap position보다 작거나 같은 경우)
    //라면 쓰기 작업은 대기해야 한다
    int64_t wrapPoint = nNextSeqForClaim - buffer_size_;
    do {
        int64_t gatingSequence = GetMinIndexOfConsumers();
        if (wrapPoint >=  gatingSequence  ) {
            std::this_thread::yield();
            continue;
        } else {
            break;
        }
    }
    while (true);
    return nNextSeqForClaim;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::Commit(int user_id, int64_t index)
{
    //cursor 가 index 바로 앞인 경우만 성공한다.
    int64_t expected = index -1 ;
    while (true) {
        if ( ring_buffer_status_on_shared_mem_->cursor == expected ) {
            ring_buffer_status_on_shared_mem_->cursor = index;
            break;
        }
        std::this_thread::yield();
    }
    wait_strategy_->SignalAllWhenBlocking(); //blocking wait strategy only.
    return true;
}

///////////////////////////////////////////////////////////////////////////////
void SharedMemRingBuffer::SignalAll()
{
    wait_strategy_->SignalAllWhenBlocking(); //blocking wait strategy only.
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::WaitFor(int user_id, int64_t index)
{
    int64_t nCurrentCursor = ring_buffer_status_on_shared_mem_->cursor.load() ;

    if( index > nCurrentCursor ) {
#if DEBUG_PRINTF
        char msg_buffer[1024];
        snprintf(msg_buffer, sizeof(msg_buffer), 
            "[id:%d]    \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d] index "
            "[%" PRId64 " - trans : %" PRId64 "] no data, wait for :nCurrentCursor[%" PRId64 "]", 
            user_id, __func__, __LINE__, index, GetTranslatedIndex(index),nCurrentCursor  );
        {AtomicPrint atomicPrint(msg_buffer);}
#endif
        //wait strategy
        return wait_strategy_->Wait(index);
    } else {
#if DEBUG_PRINTF
        char msg_buffer[1024];
        snprintf(msg_buffer, sizeof(msg_buffer), 
            "[id:%d]    \t\t\t\t\t\t\t\t\t\t\t\t [%s-%d] index[%" PRId64 "] "
            "returns [%" PRId64 "] ",
            user_id, __func__, __LINE__, index, 
            ring_buffer_status_on_shared_mem_->cursor.load() );
        {AtomicPrint atomicPrint(msg_buffer);}
#endif
        return nCurrentCursor ;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
bool  SharedMemRingBuffer::CommitRead(int user_id, int64_t index)
{
    ring_buffer_status_on_shared_mem_->array_of_consumer_indexes[user_id] = index ; //update
#if DEBUG_PRINTF
    char msg_buffer[1024];
    snprintf(msg_buffer, sizeof(msg_buffer), 
        "[id:%d]     \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d] index[%" PRId64 "] ", 
        user_id, __func__, __LINE__, index );
    {AtomicPrint atomicPrint(msg_buffer);}
#endif
    return true;
}


