#include <iostream>
#include <endian.h>
#include <functional>
#include <stdio.h>
#include "test/sharedPointer.h"
#include "test/pool.h"
#include <queue>

union quint16 {
    // 2 bit
    uint16_t u16;
    uint8_t arr[2];
};

// 大小端测试
void TestEndian() {
    quint16 x;
    x.arr[0] = 0x11;
    x.arr[1] = 0x22;

    // 大端模式
    uint16_t be_x = htobe16(x.u16);
    printf("big edian: x.u16 = %#x\n", be_x);
    // 小端数据(硬件上的数据)
    uint32_t le_x = htole32(x.u16);
    printf("little edian: x.u16 = %#x\n", le_x);
}

// 测试任务队列
class TestQueue {
    typedef std::function<void()> TestCallBack;
public:
    void addTask(TestCallBack &&func) {
        tasks.push(func);
    }
    void run() {
        while (!tasks.empty())
        {
            TestCallBack callback = tasks.front(); 
            tasks.pop();
            callback();
        }
    }
private:
    std::queue<TestCallBack> tasks;
};

int main(int argc,char** argv) {
    // test 1
    TestQueue taskPool;
    taskPool.addTask(TestEndian);
    // test 2
    TestSharedError testObj1;
    taskPool.addTask(std::bind(&TestSharedError::run,testObj1,SharedErrorSolver::ENABLE_FROM_THIS));
    // test 3
    PoolTest testObj2;
    taskPool.addTask(std::bind(&PoolTest::testStop,testObj2));
    // start process
    taskPool.run();
    return 0;
}