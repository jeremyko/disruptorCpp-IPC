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

#ifndef __DISRUPTORCPP_SHMANAGER_HPP__
#define __DISRUPTORCPP_SHMANAGER_HPP__
//20150721 kojh create

#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <string.h> 
#include <unistd.h> 
#include <errno.h> 
#include <stdio.h> 

class SharedMemoryManager
{
    public:
        SharedMemoryManager();

        bool CreateShMem(key_t nKey, size_t nSize,bool* pbFirstCreated);
        bool GetShMem(key_t nKey, size_t nSize);
        bool AttachShMem();
        bool DetachShMem();
        bool RemoveShMem();
        void* GetShMemStartAddr()
        {
            return pShMemStartAddr_ ;
        }

    private:
        key_t   nShMemKey_ ;
        int     nShMemId_ ;
        int     nShMemSize_ ;
        void*   pShMemStartAddr_ ;
        int     nTotalAttached_ ;
};

#endif

