#include "mmap.hpp"
#include <mio/mmap.hpp>
#include <iostream>

int testMmap()
{
    // 1️⃣ 打开并映射文件
    mio::mmap_source mmap("input.txt");

    if (!mmap.is_open())
    {
        std::cerr << "无法打开文件 input.txt\n";
        return 1;
    }

    // 2️⃣ 获取数据指针和长度
    const char *data = mmap.data();
    size_t size = mmap.size();

    std::cout << "文件大小: " << size << " 字节\n\n";

    // 3️⃣ 像字符串一样访问
    // 打印前 200 字符
    std::cout.write(data, std::min<size_t>(size, 200));
    std::cout << "\n\n--- 文件已映射成功 ---\n";

    return 0;
}