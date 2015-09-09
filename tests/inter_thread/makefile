CXX = g++
CXXFLAGS += -Wall -std=c++11 -g -O2

CXXFLAGS+= -D_DEBUG_RSLT_
#CXXFLAGS+= -D_DEBUG_READ
#CXXFLAGS+= -D_DEBUG_WRITE

LIBS += -lpthread 

SRCS = inter_thread_test.cpp ../../shared_mem_manager.cpp ../../ring_buffer_on_shmem.cpp
OBJECTS = inter_thread_test.o ../../shared_mem_manager.o ../../ring_buffer_on_shmem.o
TARGET = inter_thread_test

$(TARGET) : $(OBJECTS) $(LINK)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LIBS)

inter_thread_test.o : ../../shared_mem_manager.hpp ../../ring_buffer.hpp ../../shared_mem_manager.cpp ../../ring_buffer_on_shmem.cpp

all: $(TARGET)

clean :
	rm -f $(OBJECTS) $(TARGET)

