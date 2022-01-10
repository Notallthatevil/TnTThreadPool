# TnTThreadPool
___
[![BuildAndTest](https://github.com/Notallthatevil/TnTThreadPool/actions/workflows/build.yml/badge.svg)](https://github.com/Notallthatevil/TnTThreadPool/actions/workflows/build.yml)


Very simple, yet efficient and easy to use c++17 thread pool header only library. 

#### Using

To submit any task to the thread pool, include the header into your project and construct a TnTThreadPool::ThreadPool. This object can, (and should), be constructed anytime a task needs to be run in the pool. 
This object is reference counted. Each time it is constructed it increments the global reference counter. Then when it is destructed it decrements that counter. When there are no more references to the global 
thread pool. All tasks are completed, the threads are joined, and the pool is cleaned up. If another TnTThreadPool::ThreadPool object were to be constructed after this point, the thread pool is set back up.

#### Features
Allows the caller to submit a simple task without worrying about the lifetime of the thread. Allows the caller to wait for a specific submit call to finish, or even allows submission of tasks for a return value.

#### Examples

 - Simple void task with or without return arguments.

```cpp
#include <TnTThreadPool.h>

void task() {
    ...
}

void task_with_args(const char * arg1, int arg2) {
    ...
}

int main() {
    TnTThreadPool::ThreadPool tp;
    
    const char *my_string = "This is string.";
    int my_int = 10;

    tp.submit(task);
    tp.submit(task_with_args, my_string, my_int);
}
```

- Creating a task waiting for it to finish.

```cpp
#include <TnTThreadPool.h>
#include <chrono>

using namespace std::chrono_literals;

void task() {
    ...
}

void task_with_args(const char * arg1, int arg2) {
    ...
}

int main() {
    TnTThreadPool::ThreadPool tp;
    
    const char *my_string = "This is string.";
    int my_int = 10;

    auto wait_1 = tp.submitWaitable(task);
    auto wait_2 = tp.submitWaitable(task_with_args, my_string, my_int);

    wait_1.wait(); // Pauses this threads execution until wait_1 has finished execution.
    wait_2.wait_for(5000ms); // Pauses this threads execution until wait_2 has finished or the timeout period of 5000ms has elapsed.
}
```

- Creating a task for a return value.  
This works by returning an std::future with the specified return type. To check whether the return value exists or not, call wait() (or one of its alternate forms) on the future and then validatity of the return value can
be checked with valid(). See [std::future](https://en.cppreference.com/w/cpp/thread/future) for more details.
```cpp
#include <TnTThreadPool.h>
#include <chrono>

using namespace std::chrono_literals;

int task() {
    return 10;
}

int task_with_args(const char * arg1, int arg2) {
    return 1;
}

int main() {
    TnTThreadPool::ThreadPool tp;
    
    const char *my_string = "This is string.";
    int my_int = 10;

    auto wait_1 = tp.submitForReturn<int>(task);
    auto wait_2 = tp.submitForReturn<int>(task_with_args, my_string, my_int);

    wait_1.wait(); // Pauses this threads execution until wait_1 has finished execution.
    wait_2.wait_for(5000ms); // Pauses this threads execution until wait_2 has finished or the timeout period of 5000ms has elapsed.

    if(wait_2.valid()) {
        return wait_1.get() + wait_2.get() // Returns 11.
    }
    else{
        return wait_1.get(); // Returns 10.
    }
}
```

#### Limitations
All of the various submit calls can be called with function pointer, lambdas, or even classes/structs with an overloaded call operator. However, the job passed to submit cannot be taken in as a const reference. 
Doing so limits the ability to use the call operator of a class/struct unless that call operator is also const. Because of this, it was decided that the first parameter to submit "job" should not be const. As a result
a lambda cannot be constructed in the function call, and instead must be assigned to a variable and that variable then passed in.

```cpp
#include <TnTThreadPool.h>
#include <iostream>

struct S {
    int operator()(int x) { return x * x; }
};

int main() {
    TnTThreadPool::ThreadPool tp;
    
    int my_int = 10;
    S s;

    auto value_1 = tp.submitForReturn<int>(s, my_int);
    value_1.wait();
    
    std::cout << value_1.get() << std::endl; // Prints 100.

    auto lambda = [](int y) { return y * y; };
    auto value_2 = tp.submitForReturn<int>(lambda, 5);

    std::cout << value_2.get() << std::endl; // Prints 25.

    auto value_3 = tp.submitForReturn<int>([](int z) { return z * z; }, 2); // Compiler error: unable to convert lambda to Job&.
}
```
