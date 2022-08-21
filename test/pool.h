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
        // 此处考虑不能无休止放入执行函数->导致服务器崩溃
        for(int i=0;i<149;++i) {
            ThreadPool::globalInstance()->push(std::bind(&PoolTest::threadRunningCase1,this));
        }
        std::cout << "全部任务加载进入线程池\n";
        ThreadPool::globalInstance()->push(std::bind(&PoolTest::threadRunningCase2,this));
        // wait函数放在这个始终会等待所有的任务执行完毕
        ThreadPool::globalInstance()->wait();
    }
    void testStop() {
        ThreadPool::globalInstance()->start(16);
        for(int i=0;i<149;++i) {
            ThreadPool::globalInstance()->push(std::bind(&PoolTest::threadRunningCase1,this));
        }
        ThreadPool::globalInstance()->push(std::bind(&PoolTest::threadRunningCase2,this));
        std::cout << "main thread starts to sleep\n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
        ThreadPool::globalInstance()->stop();
    }
private:
    void threadRunningCase1() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    void threadRunningCase2() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
};