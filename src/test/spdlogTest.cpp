#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/async.h"
#include "spdlog/fmt/ostr.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <sstream>

void test_basic_logging() {
    std::cout << "\n=== 1. 基础日志测试 ===\n";
    
    // 设置全局日志格式
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    
    // 基本日志级别测试
    spdlog::trace("这是一条trace日志");
    spdlog::debug("这是一条debug日志");
    spdlog::info("这是一条info日志");
    spdlog::warn("这是一条warning日志");
    spdlog::error("这是一条error日志");
    spdlog::critical("这是一条critical日志");
    
    // 格式化日志测试
    spdlog::info("用户ID: {}, 姓名: {}, 年龄: {}", 12345, "张三", 28);
    spdlog::info("浮点数: {:.2f}, 百分比: {:.1%}", 3.14159, 0.85);
    spdlog::info("十六进制: 0x{:x}, 二进制: {:b}", 255, 15);
}

void test_console_logger() {
    std::cout << "\n=== 2. 控制台日志器测试 ===\n";
    
    // 创建彩色控制台日志器
    auto console = spdlog::stdout_color_mt("console");
    console->set_pattern("[%^%l%$] %v");
    
    console->trace("控制台trace日志");
    console->debug("控制台debug日志");
    console->info("控制台info日志");
    console->warn("控制台warn日志");
    console->error("控制台error日志");
    console->critical("控制台critical日志");
    
    // 测试日志级别设置
    console->set_level(spdlog::level::warn);
    console->info("这条info日志不会显示");
    console->warn("这条warn日志会显示");
    
    // 恢复级别
    console->set_level(spdlog::level::trace);
}

void test_file_logger() {
    std::cout << "\n=== 3. 文件日志器测试 ===\n";
    
    try {
        // 基础文件日志器
        auto file_logger = spdlog::basic_logger_mt("file", "logs/basic.log");
        file_logger->info("这是写入基础文件的日志");
        file_logger->warn("文件日志警告信息");
        file_logger->error("文件日志错误信息");
        
        std::cout << "基础文件日志已写入: logs/basic.log\n";
        
        // 轮转文件日志器
        auto rotating_logger = spdlog::rotating_logger_mt("rotating", "logs/rotating.log", 1024*1024*5, 3);
        for(int i = 0; i < 100; ++i) {
            rotating_logger->info("轮转日志测试 #{}: 这是一条很长的日志信息用来测试文件轮转功能", i);
        }
        
        std::cout << "轮转文件日志已写入: logs/rotating.log\n";
        
        // 每日文件日志器
        auto daily_logger = spdlog::daily_logger_mt("daily", "logs/daily.log", 2, 30);
        daily_logger->info("每日日志文件测试");
        daily_logger->warn("每日日志警告");
        
        std::cout << "每日文件日志已写入: logs/daily.log\n";
        
    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "文件日志初始化失败: " << ex.what() << std::endl;
    }
}

void test_multi_sink() {
    std::cout << "\n=== 4. 多输出目标测试 ===\n";
    
    try {
        // 创建多个sink
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multi.log"));
        
        // 创建多sink日志器
        auto multi_logger = std::make_shared<spdlog::logger>("multi", begin(sinks), end(sinks));
        multi_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%n] [%l] %v");
        
        // 注册日志器
        spdlog::register_logger(multi_logger);
        
        multi_logger->info("这条日志同时输出到控制台和文件");
        multi_logger->warn("多输出测试警告");
        multi_logger->error("多输出测试错误");
        
        std::cout << "多sink日志已同时输出到控制台和文件\n";
        
    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "多sink日志器初始化失败: " << ex.what() << std::endl;
    }
}

void test_async_logging() {
    std::cout << "\n=== 5. 异步日志测试 ===\n";
    
    try {
        // 初始化异步日志
        spdlog::init_thread_pool(8192, 1);
        
        // 创建异步文件日志器
        auto async_file = spdlog::basic_logger_mt<spdlog::async_factory>("async_file", "logs/async.log");
        
        // 创建异步控制台日志器
        auto async_console = spdlog::stdout_color_mt<spdlog::async_factory>("async_console");
        
        // 性能测试
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 10000; ++i) {
            async_file->info("异步日志性能测试 #{}: 这是一条测试异步日志性能的消息", i);
        }
        
        // 刷新所有日志
        spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) {
            l->flush();
        });
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        async_console->info("异步日志性能测试完成，写入10000条日志耗时: {}ms", duration.count());
        
        std::cout << "异步日志测试完成\n";
        
    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "异步日志初始化失败: " << ex.what() << std::endl;
    }
}

void test_pattern_formatting() {
    std::cout << "\n=== 6. 格式化模式测试 ===\n";
    
    auto formatter_logger = spdlog::stdout_color_mt("formatter");
    
    // 测试不同的格式模式
    std::vector<std::string> patterns = {
        "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v",                    // 带颜色的标准格式
        "[%Y-%m-%d %H:%M:%S] [%t] [%n] [%l] %v",                // 包含线程ID和logger名称
        "%^[%L] %v%$",                                          // 简单彩色格式
        "[%Y-%m-%d %H:%M:%S.%e] [%@] %v",                       // 包含文件名和行号
        "%+"                                                     // 默认格式
    };
    
    std::vector<std::string> descriptions = {
        "标准彩色格式",
        "详细信息格式",
        "简单彩色格式", 
        "文件位置格式",
        "默认格式"
    };
    
    for (size_t i = 0; i < patterns.size(); ++i) {
        std::cout << "\n" << descriptions[i] << ":\n";
        formatter_logger->set_pattern(patterns[i]);
        formatter_logger->info("这是{}的示例", descriptions[i]);
        formatter_logger->warn("这是警告信息");
        formatter_logger->error("这是错误信息");
    }
}

void test_custom_types() {
    std::cout << "\n=== 7. 自定义类型日志测试 ===\n";
    
    // 测试各种数据类型
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}, {"Charlie", 92}};
    
    // 手动格式化数组（因为fmt::join可能不可用）
    std::ostringstream oss;
    for (size_t i = 0; i < numbers.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << numbers[i];
    }
    spdlog::info("整数数组: {}", oss.str());
    
    // 格式化map
    std::ostringstream oss2;
    for (const auto& pair : scores) {
        oss2 << pair.first << ": " << pair.second << " ";
    }
    spdlog::info("分数映射: {}", oss2.str());
    
    // 自定义结构体
    struct Person {
        std::string name;
        int age;
        std::string city;
    };
    
    Person person{"李四", 30, "北京"};
    spdlog::info("用户信息 - 姓名: {}, 年龄: {}, 城市: {}", person.name, person.age, person.city);
    
    // 性能数据
    spdlog::info("内存使用: {:.2f}MB, CPU使用率: {:.1f}%, 磁盘使用: {:.1f}%", 
                1024.5, 85.3, 67.8);
}

void test_performance() {
    std::cout << "\n=== 8. 性能基准测试 ===\n";
    
    try {
        auto perf_logger = spdlog::basic_logger_mt("performance", "logs/performance.log");
        
        const int iterations = 100000;
        
        // 同步日志性能测试
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            perf_logger->info("性能测试日志 #{}: 当前时间戳 {}", i, 
                            std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count());
        }
        
        perf_logger->flush();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "同步日志性能: " << iterations << " 条日志, 耗时: " << duration.count() 
                  << "ms, 平均: " << (double)duration.count() / iterations << "ms/条\n";
        
        std::cout << "日志写入速度: " << (iterations * 1000.0 / duration.count()) << " 条/秒\n";
        
    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "性能测试失败: " << ex.what() << std::endl;
    }
}

void test_error_handling() {
    std::cout << "\n=== 9. 错误处理测试 ===\n";
    
    // 测试无效路径
    try {
        auto invalid_logger = spdlog::basic_logger_mt("invalid", "/invalid/path/test.log");
        invalid_logger->info("这条日志不应该成功写入");
    } catch (const spdlog::spdlog_ex &ex) {
        std::cout << "预期的错误被正确捕获: " << ex.what() << "\n";
    }
    
    // 测试重复注册
    try {
        auto logger1 = spdlog::stdout_color_mt("duplicate");
        auto logger2 = spdlog::stdout_color_mt("duplicate"); // 这会抛出异常
    } catch (const spdlog::spdlog_ex &ex) {
        std::cout << "重复注册错误被正确捕获: " << ex.what() << "\n";
    }
    
    // 设置错误处理器
    spdlog::set_error_handler([](const std::string &msg) {
        std::cerr << "spdlog错误: " << msg << std::endl;
    });
}

int main() {
    std::cout << "=== spdlog 全面功能测试开始 ===\n";
    
    // 设置全局日志级别为trace，显示所有日志
    spdlog::set_level(spdlog::level::trace);
    
    try {
        test_basic_logging();
        test_console_logger();
        test_file_logger();
        test_multi_sink();
        test_async_logging();
        test_pattern_formatting();
        test_custom_types();
        test_performance();
        test_error_handling();
        
        std::cout << "\n=== 所有测试完成 ===\n";
        std::cout << "请检查 logs/ 目录下的日志文件\n";
        
        // 最终清理
        spdlog::shutdown();
        
    } catch (const std::exception &ex) {
        std::cerr << "测试过程中发生异常: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}