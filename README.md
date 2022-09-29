[![Build](https://github.com/Notallthatevil/TnTThreadPool/actions/workflows/build.yml/badge.svg)](https://github.com/Notallthatevil/TnTThreadPool/actions/workflows/build.yml)
___

# TnTThreadPool
___
 
Very simple, yet efficient and easy to use c++20 thread pool header only library. 

#### Using
To submit any task to the thread pool, include the single header and call TnTThreadPool\::submit, TnTThreadPool\::submitWaitable, or TnTThreadPool\::submitForReturn. 

#### Features
Allows the caller to submit a simple task without worrying about the lifetime of the thread. Allows the caller to wait for a specific submit call to finish, or even allows submission of tasks for a return value.

#### Notes
Due to the way variadic templates are forwarded to lambdas, any function taking an lvalue reference will be taking a copy of that value. For example:

```cpp
#include <TnTThreadPool.h>

void task(int& arg1) {
    arg1 = 25;
}

int main() {
    int my_int = 10;

    TnT::TnTThreadPool tp;
   
    tp.submit(task_with_args, my_int);

    std::cout << my_int; // Prints 10 due to forwarding only copies. 
}
```

To workaround this limitation, pass my_int as a int* instead of an int&.



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
    const char *my_string = "This is string.";
    int my_int = 10;
    
    TnT::TnTThreadPool tp;
    
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
    const char *my_string = "This is string.";
    int my_int = 10;

    TnT::TnTThreadPool tp;

    auto wait_1 = tp.submitWaitable(task);
    auto wait_2 = tp.submitWaitable(task_with_args, my_string, my_int);

    wait_1.wait(); // Pauses this threads execution until wait_1 has finished execution.
    wait_2.wait_for(5000ms); // Pauses this threads execution until wait_2 has finished or the timeout period of 5000ms has elapsed.
}
```

- Creating a task for a return value.  
This works by returning an std::future with the specified return type. To check whether the return value exists or not, call wait() (or one of its alternate forms) on the future and then validity of the return value can
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
    const char *my_string = "This is string.";
    int my_int = 10;
    
    TnT::TnTThreadPool tp;

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

