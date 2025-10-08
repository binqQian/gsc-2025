#include <gtest/gtest.h>

#include "../src/mmap.hpp"

int main()
{
    int rc = testMmap();
    if (rc != 0)
    {
        std::cerr << "testMmap failed with code " << rc << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "testMmap OK\n";
    return EXIT_SUCCESS;
}

// // 基本示例测试
// TEST(BasicTest, SanityCheck)
// {
//     EXPECT_EQ(1 + 1, 2);
//     EXPECT_TRUE(true);
// }

// // 字符串测试示例
// TEST(StringTest, BasicOperations)
// {
//     std::string str = "Hello";
//     EXPECT_EQ(str.length(), 5);
//     EXPECT_EQ(str + " World", "Hello World");
// }

// // 测试夹具示例
// class MyTestFixture : public ::testing::Test
// {
// protected:
//     void SetUp() override
//     {
//         // 测试前初始化
//         value = 42;
//     }

//     void TearDown() override
//     {
//         // 测试后清理
//     }

//     int value;
// };

// TEST_F(MyTestFixture, ValueTest)
// {
//     EXPECT_EQ(value, 42);
//     value = 100;
//     EXPECT_EQ(value, 100);
// }

// // 如果你有实际的函数要测试，例如：
// // int add(int a, int b) { return a + b; }
// //
// // TEST(MathTest, Addition) {
// //     EXPECT_EQ(add(2, 3), 5);
// //     EXPECT_EQ(add(-1, 1), 0);
// // }
