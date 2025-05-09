#include <coroutine>
#include <iostream>

class UserFacing {
public:
    struct promise_type {
        UserFacing         get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        void               return_void() { }
        void               unhandled_exception() { }
        std::suspend_never final_suspend() noexcept { return {}; }
    };
};

UserFacing demo_coroutine()
{
    std::cout << "hello, world" << std::endl;
    co_return;
}

int main(void)
{
    UserFacing demo_instance = demo_coroutine();
}
