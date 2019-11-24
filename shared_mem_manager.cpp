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
#include "common_def.hpp" 

///////////////////////////////////////////////////////////////////////////////
SharedMemoryManager::SharedMemoryManager(): 
    sh_mem_id_(-1), sh_mem_size_(-1), sh_mem_start_addr_(NULL) , total_attached_(0)
{

}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::CreateShMem(key_t key, size_t size, bool* first_created)
{            
    sh_mem_key_  = key ;
    sh_mem_size_ = size;
    sh_mem_id_ = shmget((key_t)key, size, IPC_CREAT | IPC_EXCL | 0666 ); 
    *first_created = false;
    if (sh_mem_id_ < 0) {
        if(errno == EINVAL ) {
            err_msg_ = std::string("shmget error: key [ ") + 
                       std::to_string(key) + std::string(strerror(errno)) ;
            DEBUG_ELOG(err_msg_);
        } else if(errno == EEXIST || errno == EACCES ) {
            return GetShMem(key,size);
        } else {
            err_msg_ = std::string("shmget error: key [ ") + 
                       std::to_string(key) + std::string(strerror(errno)) ;
            DEBUG_ELOG(err_msg_);
        }
        return false;
    }
    *first_created = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::GetShMem(key_t key, size_t size)
{            
    sh_mem_key_  = key ;
    sh_mem_size_ = size;
    int nRtn = shmget((key_t)key, size,  0666 );
    if( nRtn < 0  ) {
        err_msg_ = "shmget error: " + std::string(strerror(errno)) ;
        DEBUG_ELOG(err_msg_);
        return false;
    }
    sh_mem_id_ = shmget((key_t)key, size, 0666 );
    if (sh_mem_id_ < 0) {
        err_msg_ = "shmget error: " + std::string(strerror(errno)) ;
        DEBUG_ELOG(err_msg_);
        return false;
    }
    return true;
}
///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::AttachShMem()
{            
	sh_mem_start_addr_ = shmat(sh_mem_id_ , NULL, 0);
	if (sh_mem_start_addr_ == (void *) -1) {
        DEBUG_ELOG("shmat error: "<< strerror(errno) <<",sh_mem_id_="<<sh_mem_id_);
		return false;
	}
    total_attached_ ++ ;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::DetachShMem()
{            
	if (sh_mem_start_addr_ == NULL) {
        DEBUG_ELOG("Error : shared mem start addr is NULL");
		return false;
    }
	if (shmdt(sh_mem_start_addr_) == -1) {
        DEBUG_ELOG("shmdt error: "<< strerror(errno) );
		return false;
	}
    total_attached_ -- ;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool SharedMemoryManager::RemoveShMem()
{            
    if(total_attached_ > 0 ) {
        DEBUG_ELOG("Error : attached exists :" <<total_attached_ );
		return false;
    }
    if (shmctl(sh_mem_id_, IPC_RMID, (struct shmid_ds *)NULL) == -1) {
        DEBUG_ELOG("Remove shared memoey error: "<< strerror(errno) );
		return false;
    }
    return true;
}



