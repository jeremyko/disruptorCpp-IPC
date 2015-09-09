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
#ifndef __ELAPSED_TIME_HPP__
#define __ELAPSED_TIME_HPP__

#include <chrono>
#include <string>

typedef std::chrono::duration<int, std::milli> millisecs_t;
typedef std::chrono::duration<long long, std::micro> microsecs_t;

typedef enum _ENUM_TIME_RESOLUTION_
{
    MILLI_SEC_RESOLUTION,
    MICRO_SEC_RESOLUTION,
    NO_RESOLUTION

} ENUM_TIME_RESOLUTION;

class ElapsedTime
{
    public:
        ElapsedTime()
        {
            time_resolution_ = NO_RESOLUTION;
            SetStartTime();
        }

        ElapsedTime(std::string strDescription, ENUM_TIME_RESOLUTION time_resolution)
        {
            strDescription_ = strDescription;
            time_resolution_ = time_resolution;
            SetStartTime();
        }

        ~ElapsedTime()
        {
            SetEndTime(time_resolution_);
        }

        void SetStartTime()
        {
            startT_ = std::chrono::steady_clock::now(); //c++11
            //startT_ = std::chrono::monotonic_clock::now();
        }

        long long SetEndTime( ENUM_TIME_RESOLUTION resolution)
        {
            endT_ = std::chrono::steady_clock::now(); //c++11
            //endT_ = std::chrono::monotonic_clock::now();

            if(resolution == MILLI_SEC_RESOLUTION)
            {
                millisecs_t duration(std::chrono::duration_cast<millisecs_t>(endT_ - startT_));

                if(time_resolution_ != NO_RESOLUTION)
                {
                    std::cout << strDescription_ <<" elapsed : " << duration.count() << "(milli seconds)\n";
                }
                return duration.count();
            }
            else if (resolution == MICRO_SEC_RESOLUTION)
            {
                microsecs_t duration(std::chrono::duration_cast<microsecs_t>(endT_ - startT_));
                if(time_resolution_ != NO_RESOLUTION)
                {
                    std::cout <<strDescription_ << " elapsed : " << duration.count() << "(micro seconds)\n";
                }
                return duration.count();
            }

            return -1; //error
        }

    protected:
        //std::chrono::monotonic_clock::time_point startT_; //c++0x
        //std::chrono::monotonic_clock::time_point endT_;   //c++0x
        std::chrono::steady_clock::time_point startT_; //c++11
        std::chrono::steady_clock::time_point endT_;   //c++11

        ENUM_TIME_RESOLUTION time_resolution_;
        std::string               strDescription_;
};

#endif

