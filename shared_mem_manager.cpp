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
//20150721 kojh create

#include "shared_mem_manager.hpp" 

///////////////////////////////////////////////////////////////////////////////
SharedMemoryManager::SharedMemoryManager(): 
    nShMemId_(-1), nShMemSize_(-1), pShMemStartAddr_(NULL) , nTotalAttached_(0)
{

}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::CreateShMem(key_t nKey, size_t nSize, bool* pbFirstCreated)
{            
    nShMemKey_  = nKey ;
    nShMemSize_ = nSize;

    nShMemId_ = shmget((key_t)nKey, nSize, IPC_CREAT | IPC_EXCL | 0666 ); //IPC_CREAT | IPC_EXCL 

    *pbFirstCreated = false;
    if (nShMemId_ < 0)
    {
        if(errno == EINVAL )
        {
            printf("[%s-%d] shmget error: key[%d] EINVAL\n", __func__, __LINE__, nKey);
        }
        else if(errno == EEXIST || errno == EACCES )
        {
            //printf("[%s-%d] shmget error: key[%d] [%d]EEXIST/EACCES\n", __func__, __LINE__, nKey, errno);
            return GetShMem(nKey,nSize);
        }
        else
        {
            printf("[%s-%d] shmget error: key[%d] %s\n", __func__, __LINE__, nKey, strerror(errno));
        }
        return false;
    }

    //printf("[%s-%d] shmget key[%d] size[%d] nShMemId_ [%d]\n", 
    //    __func__, __LINE__, nShMemKey_, nShMemSize_, nShMemId_ );

    *pbFirstCreated = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::GetShMem(key_t nKey, size_t nSize)
{            
    nShMemKey_  = nKey ;
    nShMemSize_ = nSize;
    int nRtn = shmget((key_t)nKey, nSize,  0666 );
    //printf("[%s-%d] %d \n", __func__, __LINE__, nRtn);

    if( nRtn < 0  )
    {
        printf("[%s-%d] shmget error: %s\n", __func__, __LINE__,strerror(errno));
        return false;
    }

    nShMemId_ = shmget((key_t)nKey, nSize, 0666 );
    if (nShMemId_ < 0)
    {
        printf("[%s-%d] shmget error: %s\n", __func__, __LINE__,strerror(errno));
        return false;
    }

    //printf("[%s-%d] shmget key[%d] size[%d] nShMemId_ [%d]\n", 
    //    __func__, __LINE__, nShMemKey_, nShMemSize_, nShMemId_ );

    return true;
}
///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::AttachShMem()
{            
	pShMemStartAddr_ = shmat(nShMemId_ , NULL, 0);

	if (pShMemStartAddr_ == (void *) -1)
	{
        printf("[%s-%d] shmat error: %s nShMemId_ [%d]\n", __func__, __LINE__,strerror(errno), nShMemId_ );
		return false;
	}

    nTotalAttached_ ++ ;
    //printf("[%s-%d] nShMemId_ [%d]  nTotalAttached_ [%d] \n", __func__, __LINE__, nShMemId_, nTotalAttached_ );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::DetachShMem()
{            
	if (pShMemStartAddr_ == NULL)
    {
        printf("[%s-%d] Error : shared mem start addr is NULL ! \n", __func__, __LINE__);
		return false;
    }

	if (shmdt(pShMemStartAddr_) == -1)
	{
        printf("[%s-%d] shmdt error: %s\n", __func__, __LINE__,strerror(errno));
		return false;
	}

    nTotalAttached_ -- ;

    //printf("[%s-%d] nShMemId_ [%d]  nTotalAttached_ [%d] \n", __func__, __LINE__, nShMemId_, nTotalAttached_ );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::RemoveShMem()
{            
    if(nTotalAttached_ > 0 ) 
    {
        printf("[%s-%d] Error : attached exists [%d] \n", __func__, __LINE__, nTotalAttached_ );
		return false;
    }

    if (shmctl(nShMemId_, IPC_RMID, (struct shmid_ds *)NULL) == -1)
    {
        printf("[%s-%d] Remove shared memoey error: %s\n", __func__, __LINE__,strerror(errno));
		return false;
    }
    //printf("[%s-%d] nShMemId_ [%d] \n", __func__, __LINE__, nShMemId_ );
    return true;
}



