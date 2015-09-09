CXX = g++
CXXFLAGS += -Wall -std=c++11 -g -O2

CXXFLAGS+= -D_DEBUG_RSLT_
#CXXFLAGS+= -D_DEBUG_READ

LIBS += -lpthread 

SRCS = consumer.cpp ../../shared_mem_manager.cpp ../../ring_buffer_on_shmem.cpp
OBJECTS = consumer.o ../../shared_mem_manager.o ../../ring_buffer_on_shmem.o
TARGET = consumer

$(TARGET) : $(OBJECTS) $(LINK) 
	$(CXX) -o $(TARGET) $(OBJECTS) $(LIBS)

producer.o : ../../shared_mem_manager.hpp ../../ring_buffer.hpp ../../shared_mem_manager.cpp ../../ring_buffer_on_shmem.cpp

all: $(TARGET)

clean :
	rm -f $(OBJECTS) $(TARGET)

