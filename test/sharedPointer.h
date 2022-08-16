#include <memory>
#include <cstdio>

enum SharedErrorSolver {
    ERROR,
    WeakPtr,
    ENABLE_FROM_THIS
};

// 使用智能指针管理实例
class MySharedInstanceManager {
public:
    std::shared_ptr<MySharedInstanceManager> getPointer() {
        // 返回一个管理this指针的智能指针
        return std::shared_ptr<MySharedInstanceManager>(this);
    }
    ~MySharedInstanceManager() {
        printf("instance was released\n");
    }
};

class MySharedInstanceManagerAdvanced : public std::enable_shared_from_this<MySharedInstanceManagerAdvanced> {
public:
    std::shared_ptr<MySharedInstanceManagerAdvanced> getPointer() {
        // 返回一个管理this指针的智能指针
        return shared_from_this();
    }
    ~MySharedInstanceManagerAdvanced() {
        printf("instance was released\n");
    }
};

// 出错
class WeakSharedInstanceManagerAdvanced {
public:
    std::weak_ptr<WeakSharedInstanceManagerAdvanced> getPointer() {
        // 返回一个管理this指针的智能指针
        return std::weak_ptr<WeakSharedInstanceManagerAdvanced>(instance);
    }
    ~WeakSharedInstanceManagerAdvanced() {
        printf("instance was released\n");
    }
private:
    std::shared_ptr<WeakSharedInstanceManagerAdvanced> instance = std::shared_ptr<WeakSharedInstanceManagerAdvanced>(this);
};

class TestSharedError {
public:
    void run(SharedErrorSolver solver) {
        switch (solver)
        {
        case SharedErrorSolver::ERROR: {
            std::shared_ptr<MySharedInstanceManager> countPointer1(new MySharedInstanceManager);
            std::shared_ptr<MySharedInstanceManager> countPointer2 = countPointer1->getPointer();
            printf("countPointer1 considered that reference count of the pointer memory it points to is %ld\n", countPointer1.use_count());
            printf("countPointer2 considered that reference count of the pointer memory it points to is %ld\n", countPointer2.use_count());
            break;
        }
        // enable from this用于解决计数不正常增加的情况
        case SharedErrorSolver::ENABLE_FROM_THIS: {
            std::shared_ptr<MySharedInstanceManagerAdvanced> countPointer1(new MySharedInstanceManagerAdvanced);
            std::shared_ptr<MySharedInstanceManagerAdvanced> countPointer2 = countPointer1->getPointer();
            printf("countPointer1 considered that reference count of the pointer memory it points to is %ld\n", countPointer1.use_count());
            printf("countPointer2 considered that reference count of the pointer memory it points to is %ld\n", countPointer2.use_count());
            break;
        }
        default:
            // 类中自带的shared_ptr和外部创建的都以为自己独自拥有shared_ptr->未增加计数(weak_ptr只是作为循环引用的解决方案) 
            std::shared_ptr<WeakSharedInstanceManagerAdvanced> countPointer1(new WeakSharedInstanceManagerAdvanced);
            std::weak_ptr<WeakSharedInstanceManagerAdvanced> countPointer2 = countPointer1->getPointer();
            printf("countPointer1 considered that reference count of the pointer memory it points to is %ld\n", countPointer1.use_count());
            printf("countPointer2 considered that reference count of the pointer memory it points to is %ld\n", countPointer2.use_count());
            break;
        }
      
    }
};