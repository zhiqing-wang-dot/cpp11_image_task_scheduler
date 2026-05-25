#pragma once

#include <memory> // for std::unique_ptr
#include <utility> // for std::forward

// typename... Args：表示一组参数类型，例如 int、double、std::string
// Args&&... args：表示一组实际传入的参数，使用转发引用接收左值和右值
// std::forward<Args>(args)...：将参数包逐个展开，并尽量保持原来的左值/右值属性
// new T(...)：使用这些参数调用 T 的构造函数，在堆区创建对象
// std::unique_ptr<T>：接管 new 出来的对象，离开作用域后自动释放
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    // std::forward<Args>(args) ：别人传什么类型的参数进来，我就尽量保持原样传给构造函数。
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}