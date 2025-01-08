#pragma once

/*
    如果一个类可以拷贝与赋值那么我们让他继承至copyable
*/

class copyable
{
protected:
    copyable() = default;
    ~copyable() = default;
};