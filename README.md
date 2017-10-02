# disruptorCpp-IPC

## basic disruptor c++ implementation for IPC
[http://jeremyko.blogspot.kr/2015/09/disruptor.html](http://jeremyko.blogspot.kr/2015/09/disruptor.html)

### inter thread test 

    cd tests/inter_thread 
    make
    ./inter_thread_test 

 

### inter process test 
    cd tests/inter_process 
    make -f make-producer.mk 
    make -f make-consumer.mk
    //run 2 consumer, then 1 producer
    ./consumer 0
    ./consumer 1
    ./producer
    //make sure reset shared memory running 'ipcrm -M your_shmkey' if you have changed buffer size or number of producers/consumers.
