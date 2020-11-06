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

#ifndef DISRUPTORCPP_SHMANAGER_HPP
#define DISRUPTORCPP_SHMANAGER_HPP
//20150721 kojh create

#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <string.h> 
#include <unistd.h> 
#include <errno.h> 
#include <stdio.h> 
#include <string> 

////////////////////////////////////////////////////////////////////////////////
class SharedMemoryManager
{
    public:
        SharedMemoryManager();

        bool AttachShMem();
        bool DetachShMem();
        bool RemoveShMem();
        bool GetShMem(key_t key, size_t size);
        bool CreateShMem(key_t key, size_t size,bool* first_created);
        void* GetShMemStartAddr() { return sh_mem_start_addr_ ; }
        const char* GetLastErrMsg() { return err_msg_.c_str(); }

    private:
        key_t   sh_mem_key_  ;
        int     sh_mem_id_   ;
        size_t  sh_mem_size_ ;
        void*   sh_mem_start_addr_ ;
        int     total_attached_ ;
        std::string err_msg_ ;  
};

#endif

