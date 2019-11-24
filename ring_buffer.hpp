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

#ifndef DISRUPTORCPP_RING_BUFFER_HPP
#define DISRUPTORCPP_RING_BUFFER_HPP

#include <iostream>
#include <memory>
#include <vector>
#include "common_def.hpp" 
        
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class RingBuffer
{
    public:
        RingBuffer() {
            capacity_ = DEFAULT_RING_BUFFER_SIZE;
            buffer_.reserve(DEFAULT_RING_BUFFER_SIZE);
        }
        RingBuffer(const std::vector<T>& buffer) : buffer_(buffer) {}
        
        T& operator[](const int64_t & sequence) { 
            return buffer_[sequence & (capacity_ - 1)]; //only when multiple of 2
        }
        
        int64_t GetTranslatedIndex( int64_t sequence) {
            int64_t translated_index = (sequence & (capacity_ - 1)) ; 
            return translated_index ;
        }

        bool SetCapacity(size_t capacity) {
            bool is_power2 =  capacity && !( (capacity-1) & capacity ) ; 
            if( is_power2 ==0 ) {
                DEBUG_ELOG("Buffer capacity error: power of 2 required!");
                return false;
            }

            try {
                buffer_.reserve(capacity);
            } catch (const std::length_error& le) {
                DEBUG_ELOG("Length error: " << le.what() );
                return false;
            }
            capacity_ = capacity;
            return true;
        }

    private:
        size_t          capacity_ ;
        std::vector<T>  buffer_;
        RingBuffer(const RingBuffer&);
        void operator=(const RingBuffer&);
        RingBuffer(RingBuffer&&);
        void operator=(const RingBuffer&&);
};

#endif //DISRUPTORCPP_RING_BUFFER_HPP

