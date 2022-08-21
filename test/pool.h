#include "../base/ThreadPool.h"
#include <chrono>
#include <iostream>

/**
 * 测试ThreadPool的相关函数 
 */
class PoolTest {
public:
    void testWait() {
        ThreadPool::globalInstance()->start(16);
        for(int i=0;i<49;++i) {
            ThreadPool::globalInstance()->push(std::bind(&PoolTest::threadRunningCase1,this));
        }
        ThreadPool::globalInstance()->push(std::bind(&PoolTest::threadRunningCase2,this));
        ThreadPool::globalInstance()->wait();
    }
    void testStop() {
        ThreadPool::globalInstance()->start(16);
        for(int i=0;i<49;++i) {
            ThreadPool::globalInstance()->push(std::bind(&PoolTest::threadRunningCase1,this));
        }
        ThreadPool::globalInstance()->push(std::bind(&PoolTest::threadRunningCase2,this));
        std::cout << "main thread starts to sleep\n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
        ThreadPool::globalInstance()->stop();
    }
private:
    void threadRunningCase1() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    void threadRunningCase2() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
};