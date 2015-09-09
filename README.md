# disruptorCpp-IPC

##basic disruptor c++ implementation for IPC
[http://jeremyko.blogspot.kr/2015/09/disruptor.html](http://jeremyko.blogspot.kr/2015/09/disruptor.html)
###inter thread test 

    cd tests/inter_thread 
    make
    ./inter_thread_test 

 

###inter process test 
    cd tests/inter_process 
    make -f make-producer.mk 
    make -f make-consumer.mk
    ./consumer
    ./producer
