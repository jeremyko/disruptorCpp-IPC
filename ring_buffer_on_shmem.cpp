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
///////////////////////////////////////////////////////////////////////////////
//20150721 kojh create
///////////////////////////////////////////////////////////////////////////////

#include "ring_buffer_on_shmem.hpp" 
#include "atomic_print.hpp"

int LEN_ONE_BUFFER_DATA = sizeof(OneBufferData);

#define _DEBUG_        0
#define _DEBUG_COMMIT  0

std::mutex AtomicPrint::lock_mutex_ ;

///////////////////////////////////////////////////////////////////////////////
SharedMemRingBuffer::SharedMemRingBuffer(ENUM_WAIT_STRATEGY waitStrategyType)
{
    pRingBufferStatusOnSharedMem_ = NULL;
    waitStrategyType_ = waitStrategyType ;
    pWaitStrategy_ = NULL;
}

///////////////////////////////////////////////////////////////////////////////
SharedMemRingBuffer::~SharedMemRingBuffer()
{
    if(pWaitStrategy_)
    {
        delete pWaitStrategy_;
        pWaitStrategy_ = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::TerminateRingBuffer()
{
    if(! sharedMemoryManager_.DetachShMem())
    {
        std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "Error " << '\n'; 
        return false;
    }

    if(! sharedMemoryManager_.RemoveShMem())
    {
        std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << 
            "Ln[" << __LINE__ << "] " << "Error " << '\n'; 
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::RegisterConsumer (int nId, int64_t* pIndexforCustomerUse)
{

    if(pRingBufferStatusOnSharedMem_->arrayOfConsumerIndexes[nId] == -1 )
    {
        //처음 등록 
        pRingBufferStatusOnSharedMem_->registered_consumer_count++;
        if( pRingBufferStatusOnSharedMem_->registered_consumer_count >= MAX_CONSUMER)
        {
            std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "Error: Exceeds MAX_CONSUMER : " << MAX_CONSUMER << '\n'; 
            return false;
        }

        if(pRingBufferStatusOnSharedMem_->cursor >= 0 )
        {
            std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "cursor >= 0 " << '\n'; 
            //데이터 전달 중이데 새로운 소비자가 추가
            pRingBufferStatusOnSharedMem_->arrayOfConsumerIndexes[nId] = pRingBufferStatusOnSharedMem_->cursor.load() ; 
        }
        else
        {
            std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "set  0 " << '\n'; 
            pRingBufferStatusOnSharedMem_->arrayOfConsumerIndexes[nId] = 0; 
        }

        *pIndexforCustomerUse = pRingBufferStatusOnSharedMem_->arrayOfConsumerIndexes[nId];
    }
    else
    {
        //last read message index 
        //기존 최종 업데이트 했던 인덱스 + 1 돌려준다. consumer가 호출할 인덱스 이므로 ..
        *pIndexforCustomerUse = pRingBufferStatusOnSharedMem_->arrayOfConsumerIndexes[nId] + 1;
    }

#if 1
    std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "USAGE_CONSUMER ID : " << nId<< " / index :"<< *pIndexforCustomerUse << '\n'; 
#endif

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void SharedMemRingBuffer::ResetRingBufferState() 
{
    if(pRingBufferStatusOnSharedMem_ == NULL )
    {
        std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "call InitRingBuffer first !" << '\n'; 
        return;
    }

    std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " <<  '\n'; 

    pRingBufferStatusOnSharedMem_->cursor.store(-1);
    pRingBufferStatusOnSharedMem_->next.store(-1);
    pRingBufferStatusOnSharedMem_->registered_producer_count.store(0);
    pRingBufferStatusOnSharedMem_->registered_consumer_count.store(0);

    nTotalMemSize_ = 0;

    for(int i = 0; i < MAX_CONSUMER; i++)
    {
        pRingBufferStatusOnSharedMem_->arrayOfConsumerIndexes[i] = -1;
    }

    //for blocking wait strategy : shared mutex, shared cond var
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init( & pRingBufferStatusOnSharedMem_->mtxLock, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init( & pRingBufferStatusOnSharedMem_->condVar, &condAttr);
}


///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::InitRingBuffer(int nSize /*= DEFAULT_RING_BUFFER_SIZE*/)
{
    if(nSize<= 0)
    {
        std::cout << "Ln[" << __LINE__ << "] " << "Error: Invalid size " << '\n'; 
        return false;
    }

    nBufferSize_ = nSize;

    if(!ringBuffer_.SetCapacity(nSize) )
    {
        std::cout << "Ln[" << __LINE__ << "] " << "Error: Invalid size " << '\n'; 
        return false;
    }

    //shared memory consists of : RingBufferStatusOnSharedMem + actual data
    nTotalMemSize_ = sizeof(_RingBufferStatusOnSharedMem_)  + (sizeof(OneBufferData) * nSize) ;

    bool bSharedMemFirstCreated = false;
    if(! sharedMemoryManager_.CreateShMem(123456, nTotalMemSize_, &bSharedMemFirstCreated )) 
    {
        std::cout << "Ln[" << __LINE__ << "] " << "Error: shared memory failed " << '\n'; 
        return false;
    }

    if(! sharedMemoryManager_.AttachShMem())
    {
        std::cout << "Ln[" << __LINE__ << "] " << "Error: shared memory failed " << '\n'; 
        return false;
    }

    pRingBufferStatusOnSharedMem_ = (RingBufferStatusOnSharedMem*) sharedMemoryManager_. GetShMemStartAddr(); 
    if(bSharedMemFirstCreated)
    {
        ResetRingBufferState();
    }

    char* pBufferStart = (char*)sharedMemoryManager_.GetShMemStartAddr() + sizeof(_RingBufferStatusOnSharedMem_) ; 

    for(int i = 0; i < nSize; i++)
    {
        //OneBufferData* pData = new( (char*)pBufferStart + (sizeof(OneBufferData)*i)) OneBufferData; 
        //ringBuffer_[i] = pData ;
        ringBuffer_[i] = (OneBufferData*) ( (char*)pBufferStart + (sizeof(OneBufferData)*i) ) ;
    }

    pRingBufferStatusOnSharedMem_->nBufferSize = nSize;
    pRingBufferStatusOnSharedMem_->nTotalMemSize = nTotalMemSize_;

    //---------------------------------------------
    //wait strategy
    if(waitStrategyType_ == BLOCKING_WAIT )
    {
        std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "Wait Strategy :BLOCKING_WAIT" << '\n'; 
        pWaitStrategy_ = new BlockingWaitStrategy(pRingBufferStatusOnSharedMem_);
    }
    else if(waitStrategyType_ == YIELDING_WAIT )
    {
        std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "Wait Strategy :YIELDING_WAIT" << '\n'; 
        pWaitStrategy_ = new YieldingWaitStrategy(pRingBufferStatusOnSharedMem_);
    }
    else if(waitStrategyType_ == SLEEPING_WAIT )
    {
        std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "Wait Strategy :SLEEPING_WAIT" << '\n'; 
        pWaitStrategy_ = new SleepingWaitStrategy(pRingBufferStatusOnSharedMem_);
    }
    else
    {
        std::cout << "["<< __func__ <<"-"<<  __LINE__ << "] " << "Invalid Wait Strategy :" << waitStrategyType_<< '\n'; 
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
OneBufferData*  SharedMemRingBuffer::GetData(int64_t nIndex)
{
    return ringBuffer_[nIndex];
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::GetTranslatedIndex( int64_t sequence)
{
    return ringBuffer_.GetTranslatedIndex(sequence);
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::SetData( int64_t nIndex, OneBufferData* pData)
{
    memcpy( ringBuffer_[nIndex], pData, LEN_ONE_BUFFER_DATA ); 

    return true;
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::GetMinIndexOfConsumers()
{
    int64_t nMinIndex = INT64_MAX ;
    bool bFound = false;

    for(int i = 0; i < pRingBufferStatusOnSharedMem_->registered_consumer_count; i++)
    {
        int64_t nIndex = pRingBufferStatusOnSharedMem_->arrayOfConsumerIndexes[i];
        if( nIndex < nMinIndex )
        {
            nMinIndex = nIndex;
            bFound = true;
        }
    }

    if(!bFound)
    {
        return 0;
    }

    return nMinIndex ;
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::GetNextSequenceForClaim()
{
    return pRingBufferStatusOnSharedMem_->next.fetch_add(1) + 1;
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::ClaimIndex(int nCallerId )
{
    int64_t nNextSeqForClaim = GetNextSequenceForClaim() ;

    //다음 쓰기 위치 - 링버퍼 크기 = 링버퍼 한번 순회 이전위치, 
    //데이터 쓰기 작업이 링버퍼를 완전히 한바퀴 순회한 경우
    //여기서 더 진행하면 데이터를 이전 덮어쓰게 된다.
    //만약 아직 데이터를 read하지 못한 상태(min customer index가 wrap position보다 작거나 같은 경우)
    //라면 쓰기 작업은 대기해야 한다
    int64_t wrapPoint = nNextSeqForClaim - nBufferSize_;

    do
    {
        int64_t gatingSequence = GetMinIndexOfConsumers();
        if (wrapPoint >=  gatingSequence  ) 
        {
            std::this_thread::yield();
            continue;
        }
        else
        {
            break;
        }
    }
    while (true);

    return nNextSeqForClaim;

}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemRingBuffer::Commit(int nUserId, int64_t index)
{
    //cursor 가 index 바로 앞인 경우만 성공한다.
    int64_t expected = index -1 ;

    while (true)
    {
        //if ( cursor_.compare_exchange_strong(expected , index ))
        if ( pRingBufferStatusOnSharedMem_->cursor == expected )
        {
            pRingBufferStatusOnSharedMem_->cursor = index;

            break;
        }
    
        std::this_thread::yield();
        //std::this_thread::sleep_for(std::chrono::nanoseconds(1)); 
    }
    pWaitStrategy_->SignalAllWhenBlocking(); //blocking wait strategy only.

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void SharedMemRingBuffer::SignalAll()
{
    pWaitStrategy_->SignalAllWhenBlocking(); //blocking wait strategy only.
}

///////////////////////////////////////////////////////////////////////////////
int64_t SharedMemRingBuffer::WaitFor(int nUserId, int64_t nIndex)
{
    int64_t nCurrentCursor = pRingBufferStatusOnSharedMem_->cursor.load() ;

    if( nIndex > nCurrentCursor )
    {
#if _DEBUG_
        snprintf(szMsg, sizeof(szMsg), 
                "[id:%d]    \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d] index [%" PRId64 " - trans : %" PRId64 "] no data, wait for :nCurrentCursor[%" PRId64 "]", 
                nUserId, __func__, __LINE__, nIndex, GetTranslatedIndex(nIndex),nCurrentCursor  );
        {AtomicPrint atomicPrint(szMsg);}
#endif
        //wait strategy
        return pWaitStrategy_ ->Wait(nIndex);
    }
    else
    {
#if _DEBUG_
        snprintf(szMsg, sizeof(szMsg), "[id:%d]    \t\t\t\t\t\t\t\t\t\t\t\t [%s-%d] index[%" PRId64 "] returns [%" PRId64 "] ",
                nUserId, __func__, __LINE__, nIndex, pRingBufferStatusOnSharedMem_->cursor.load() );
        {AtomicPrint atomicPrint(szMsg);}
#endif
        return nCurrentCursor ;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
bool  SharedMemRingBuffer::CommitRead(int nUserId, int64_t index)
{

    pRingBufferStatusOnSharedMem_->arrayOfConsumerIndexes[nUserId] = index ; //update

#if _DEBUG_
    snprintf(szMsg, sizeof(szMsg), 
            "[id:%d]     \t\t\t\t\t\t\t\t\t\t\t\t[%s-%d] index[%" PRId64 "] ", nUserId, __func__, __LINE__, index );
    {AtomicPrint atomicPrint(szMsg);}
#endif

    return true;
}


