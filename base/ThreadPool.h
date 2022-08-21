#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "common.h"
#include <thread>
#include <iostream>
#include <queue>

// 线程池中的基础函数类型
typedef std::function<void()> TaskFunction;
// 采用原子变量加快性能
typedef std::atomic<ushort> AtomicUShort;
typedef std::atomic<int> AtomicInt;
// 任务池中的最大的任务数量
#define MAX_TASK_POOL_COUNT 50
// 函数队列中最大的函数数量
#define MAX_FUNCTION_QUEUE_COUNT 1000 


/**
 * 任务执行类
 */
class Task {
public:
    Task():taskBusy_(false) {}
    /**
     * 给任务函数附着执行函数
     */
    void attach(TaskFunction &&func) {
        func_ = func;
    }
    /**
     * 重载operator函数->函数的调用可以利用对象名即可 
     */
    bool operator()();
    /**
     * 判断任务是否结束 
     */
    bool isBusy() {
        return taskBusy_;
    }

private:
    TaskFunction func_;
    bool taskBusy_;
};

/**
 * 任务池->用于像内存池一样同一管理空闲任务和非空闲任务 
 * 同时也可以限制任务的最大数量
 */
class TaskPool:public std::enable_shared_from_this<TaskPool> {
public:
    static std::shared_ptr<TaskPool> getInstance();
    /**
     * 初始化任务数量，避免二次初始化
     */
    void initTaskCount(ushort count) {
        if (!isTaskCountInit_) {
            isTaskCountInit_ = true;
            // 环形队列区分为空和为满的情况
            tasks.resize(count);
        }
        return;
    }

    /**
     * 从任务池中取出任务放入执行函数->从尾部拿任务
     */
    Task& push(TaskFunction &&func) { 
        std::unique_lock<std::mutex> lk(fullLock_);
        // the aim to use outer while loop is to avoid fake wakeup
        while (isFull()) {
            std::cout << "wait for the task poll releasing\n"; 
            fullCv_.wait(lk, [&](){ return !isFull();}); 
        }
        // 完美转发右值引用
        tasks[freeIndex].attach(std::forward<TaskFunction>(func)); 

        return tasks[freeIndex];
    }

    void notify() {
        fullCv_.notify_one();
    }

    /**
     * 判断任务槽池是否为满 
     */
    bool isFull() {
        bool flag = true;
        for(int i=0;i<tasks.size();++i) {
            if(!tasks[i].isBusy()) {
                flag = false;
                freeIndex = i;
            }
        }
        return flag;
    }

private:
    TaskPool():isTaskCountInit_(false) {
        initTaskCount(MAX_TASK_POOL_COUNT);
    }

private:
    static std::shared_ptr<TaskPool> instance_;
    // 任务池
    std::vector<Task> tasks;
    bool isTaskCountInit_;
    // lock and condition
    std::mutex fullLock_;
    std::condition_variable fullCv_;
    // free index
    int freeIndex;
};

/**
 * 线程池->用于全局线程数量的管理
 * 限定线程的数量
 */
class ThreadPool:public std::enable_shared_from_this<ThreadPool> {
public:
    /**
     * 模仿Qt对线程池的命名 
     */
    static std::shared_ptr<ThreadPool> globalInstance();
    /**
     * 线程池中启动的线程数目->线程池并不会卡顿 
     * 所有的线程执行相同的逻辑->从任务池中拿任务，等待任务
     */
    void start(ushort count) {
        isRunning = true;
        poolThreadCount_ = count;
        std::shared_ptr<TaskPool> taskPool = TaskPool::getInstance();
        threads.reserve(count);
        for (size_t i = 0; i < count; i++) {
            threads.push_back(std::thread(&ThreadPool::runInThread,this));
        }
    }
    /**
     * 将函数放入函数执行队列
     */
    void push(TaskFunction &&func) {
        std::unique_lock<std::mutex> lk(fullLock_);
        // 函数队列为满时
        while(functions.size() == MAX_FUNCTION_QUEUE_COUNT) {
            fullCv_.wait(lk, [&](){
                return functions.size() < MAX_FUNCTION_QUEUE_COUNT||!isRunning;
            });
        }
        if(!isRunning) {
            return;
        }
        functions.push(func);
        emptyCv_.notify_one();
    }

    /**
     * 退出线程池->模仿QThread的wait() quit() 
     */
    void quit() {
        isRunning = false;
        std::unique_lock<std::mutex> lk(childLock_);
        std::cout << "等待子线程退出\n";
        emptyCv_.notify_all();
        childCv_.wait(lk,[this](){
            return this->poolThreadCount_ == 0;
        });
        std::cout << "所有子线程退出\n";
        for (size_t i = 0; i < threads.size(); i++) {
            threads[i].join();
        }
        std::cout << "所有子线程合并到主线程\n";
    }
    /**
     * 执行线程池等待->模仿QThread的wait()
     */
    void wait() {
        // 第一步，等待正在运行的任务完成
        {
            std::unique_lock<std::mutex> lk(emptyLock_);
            std::cout << "wait for child threads exiting...\n";
            // 设置标志位->只有任务执行完毕(虚假唤醒)且函数拿完该wait函数才会继续执行
            emptyCv_.wait(lk, [this] { return functions.empty() && this->activeTask_ == 0; });
        }
        // 第二步，唤醒子线程并退出
        quit();
    }
    void stop() {
        isRunning = false;
        // 退出所有想要放任务的线程 ------- 任务槽已满
        fullCv_.notify_all();
        
        while (!functions.empty())
        {
            functions.pop();
        }

        for(std::thread &t:threads) {
            // linux平台特有的停止方法
            pthread_cancel(t.native_handle());
            if(t.joinable()) {
                t.join();
            }
        } 
    }
private:

    /** 
     * 线程入口函数 
     */
    void runInThread() {
        // In loop 
        while (isRunning) {
            std::shared_ptr<TaskPool> taskPool = TaskPool::getInstance();
            TaskFunction func = take();
            if (func != nullptr) {
                static std::atomic_int32_t count = 0;
                ++count;
                Task task = taskPool->push(std::move(func));
                // 执行任务函数
                std::string id = idString(std::this_thread::get_id()) + ":execute task function\n";
                std::cout << id;
                ++activeTask_;
                task();
                --activeTask_;
                // 当前任务执行完毕(name should be taskEmptyLock-)
                std::unique_lock<std::mutex> lock(emptyLock_);
                if (activeTask_ == 0 && functions.empty()) {
                    // 唤醒所有的线程
                    emptyCv_.notify_all();
                    std::string id = idString(std::this_thread::get_id()) + ":notify all sub-threads\n";
                    std::cout << id;
                }
            }else {
                --poolThreadCount_;
                if (poolThreadCount_ == 0) {
                    // 通知等待线程退出(主线程)
                    childCv_.notify_one();
                }
                break;
            }
        }
        // 线程退出
    }
    /**
     * 从函数队列中获取函数
     */
    TaskFunction take() {
        std::unique_lock<std::mutex> lk(emptyLock_);
        // 函数队列为空或者线程池还在运行
        while (functions.empty()&&isRunning){ emptyCv_.wait(lk, [&](){ return !functions.empty()||!isRunning; }); }
        if(!isRunning) {
            return nullptr;
        }
        TaskFunction func = functions.front();
        functions.pop();
        return func;
    }

    ThreadPool():isRunning(false) {}

private:
    // 函数队列为空
    std::mutex emptyLock_;
    std::condition_variable emptyCv_;
    // 函数队列为满
    std::mutex fullLock_;
    std::condition_variable fullCv_;
    // 子线程清理
    std::mutex childLock_;
    std::condition_variable childCv_;
    // 活跃任务标志
    AtomicInt activeTask_;
    // 活跃线程标志
    AtomicInt poolThreadCount_;
    bool isRunning;
    // 负责执行任务的线程 
    std::vector<std::thread> threads;
    // 等待加入线程池的函数队列
    std::queue<TaskFunction> functions;
    static std::shared_ptr<ThreadPool> instance_;
};