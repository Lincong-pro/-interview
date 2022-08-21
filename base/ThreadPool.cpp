#include "ThreadPool.h"
// 分配静态变量的堆内存
bool Task::operator()()  {
    taskBusy_ = false;
    func_();
    taskBusy_ = true;
    TaskPool::getInstance()->notify();
    return true;
}

std::shared_ptr<TaskPool> TaskPool::instance_(new TaskPool); 
std::shared_ptr<ThreadPool> ThreadPool::instance_(new ThreadPool);

std::shared_ptr<TaskPool> TaskPool::getInstance() {
    return instance_;
}
std::shared_ptr<ThreadPool> ThreadPool::globalInstance() {
    return instance_;
}
