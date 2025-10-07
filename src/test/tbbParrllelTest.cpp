#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <random>
#include <thread>
#include <fstream>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <condition_variable>

// 启用TBB预览功能
#define TBB_PREVIEW_MEMORY_POOL 1

// oneTBB 头文件
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_pipeline.h>
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/task_scheduler_observer.h>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/scalable_allocator.h>
#include <oneapi/tbb/cache_aligned_allocator.h>
#include <oneapi/tbb/memory_pool.h>
#include <oneapi/tbb/concurrent_vector.h>

using namespace std;
using namespace tbb;

// ==================== 详细的线程监控器 ====================
class DetailedThreadMonitor : public task_scheduler_observer
{
private:
    atomic<int> active_threads{0};
    atomic<int> max_concurrent_threads{0};
    atomic<int> worker_entries{0};
    atomic<int> master_entries{0};
    mutable mutex stats_mutex;
    vector<thread::id> thread_ids;
    chrono::high_resolution_clock::time_point start_time;

public:
    DetailedThreadMonitor() : task_scheduler_observer()
    {
        start_time = chrono::high_resolution_clock::now();
        observe(true);
    }

    void on_scheduler_entry(bool is_worker) override
    {
        int current = ++active_threads;

        // 更新最大并发线程数
        int expected = max_concurrent_threads.load();
        while (current > expected &&
               !max_concurrent_threads.compare_exchange_weak(expected, current))
        {
            expected = max_concurrent_threads.load();
        }

        // 统计线程类型
        if (is_worker)
        {
            worker_entries++;
        }
        else
        {
            master_entries++;
        }

        // 记录线程ID
        {
            lock_guard<mutex> lock(stats_mutex);
            thread::id tid = this_thread::get_id();
            if (find(thread_ids.begin(), thread_ids.end(), tid) == thread_ids.end())
            {
                thread_ids.push_back(tid);
            }
        }
    }

    void on_scheduler_exit(bool is_worker) override
    {
        --active_threads;
    }

    void print_detailed_stats() const
    {
        auto now = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration<double>(now - start_time).count();

        cout << "\n=== 线程使用详细统计 ===" << endl;
        cout << "最大并发线程数: " << max_concurrent_threads.load() << endl;
        cout << "当前活跃线程: " << active_threads.load() << endl;
        cout << "工作线程进入次数: " << worker_entries.load() << endl;
        cout << "主线程进入次数: " << master_entries.load() << endl;
        cout << "总的唯一线程数: " << thread_ids.size() << endl;
        cout << "运行时间: " << fixed << setprecision(3) << elapsed << " 秒" << endl;
        cout << "平均线程利用率: " << (worker_entries.load() / elapsed) << " 进入/秒" << endl;
    }

    int get_max_threads() const { return max_concurrent_threads.load(); }
    int get_unique_thread_count() const
    {
        lock_guard<mutex> lock(stats_mutex);
        return thread_ids.size();
    }
};

// ==================== 内存使用监控器 ====================
class MemoryUsageMonitor
{
private:
    size_t initial_memory = 0;
    size_t peak_memory = 0;
    atomic<size_t> current_allocated{0};
    atomic<size_t> total_allocated{0};
    atomic<size_t> total_freed{0};

public:
    void start_monitoring()
    {
        initial_memory = get_system_memory();
        peak_memory = initial_memory;
        current_allocated = 0;
        total_allocated = 0;
        total_freed = 0;
    }

    void record_allocation(size_t size)
    {
        current_allocated += size;
        total_allocated += size;
        update_peak();
    }

    void record_deallocation(size_t size)
    {
        current_allocated -= size;
        total_freed += size;
    }

    void update_peak()
    {
        size_t current = get_system_memory();
        if (current > peak_memory)
        {
            peak_memory = current;
        }
    }

    void print_memory_stats() const
    {
        cout << "\n=== 内存使用统计 ===" << endl;
        cout << "初始内存: " << (initial_memory / 1024 / 1024) << " MB" << endl;
        cout << "峰值内存: " << (peak_memory / 1024 / 1024) << " MB" << endl;
        cout << "内存增长: " << ((peak_memory - initial_memory) / 1024 / 1024) << " MB" << endl;
        cout << "当前分配: " << (current_allocated.load() / 1024 / 1024) << " MB" << endl;
        cout << "总分配量: " << (total_allocated.load() / 1024 / 1024) << " MB" << endl;
        cout << "总释放量: " << (total_freed.load() / 1024 / 1024) << " MB" << endl;
        cout << "内存效率: " << (total_freed.load() * 100.0 / max(total_allocated.load(), size_t(1))) << "%" << endl;
    }

private:
    size_t get_system_memory()
    {
        ifstream status_file("/proc/self/status");
        string line;
        while (getline(status_file, line))
        {
            if (line.find("VmRSS:") == 0)
            {
                istringstream iss(line);
                string label;
                size_t memory_kb;
                iss >> label >> memory_kb;
                return memory_kb * 1024;
            }
        }
        return 0;
    }
};

// ==================== 测试数据结构 ====================
struct LargeDataBlock
{
    vector<double> data;
    string metadata;
    int id;

    LargeDataBlock(int block_id, size_t size = 1000) : id(block_id)
    { // 默认大小改为1000
        data.reserve(size);
        random_device rd;
        mt19937 gen(rd() + block_id);
        uniform_real_distribution<> dis(0.0, 1000.0);

        for (size_t i = 0; i < size; ++i)
        {
            data.push_back(dis(gen));
        }

        metadata = "Block_" + to_string(block_id) + "_" + string(50, 'X'); // 减少字符串大小
    }

    double compute_result() const
    {
        // 模拟复杂计算
        double sum = accumulate(data.begin(), data.end(), 0.0);
        double avg = sum / data.size();

        // 减少计算延迟
        this_thread::sleep_for(chrono::microseconds(100)); // 从1ms改为100微秒

        return avg;
    }
};

// ==================== TBB控制功能测试类 ====================
class TBBControlTests
{
private:
    DetailedThreadMonitor *monitor;
    MemoryUsageMonitor mem_monitor;

public:
    TBBControlTests() : monitor(new DetailedThreadMonitor()) {}

    ~TBBControlTests()
    {
        delete monitor;
    }

    // 测试1: 线程数量控制
    void test_thread_control()
    {
        cout << "\n"
             << string(60, '=') << endl;
        cout << "测试1: TBB线程数量控制" << endl;
        cout << string(60, '=') << endl;

        const int data_size = 1000000;
        vector<int> data(data_size);
        iota(data.begin(), data.end(), 1);

        // 测试不同的线程数限制
        vector<int> thread_limits = {1, 2, 4, 8};

        for (int limit : thread_limits)
        {
            cout << "\n--- 限制为 " << limit << " 个线程 ---" << endl;

            // 创建新的监控器
            delete monitor;
            monitor = new DetailedThreadMonitor();

            // 使用global_control限制线程数
            global_control gc(global_control::max_allowed_parallelism, limit);

            auto start = chrono::high_resolution_clock::now();

            atomic<long long> sum{0};
            parallel_for(blocked_range<size_t>(0, data.size()),
                         [&](const blocked_range<size_t> &range)
                         {
                             long long local_sum = 0;
                             for (size_t i = range.begin(); i != range.end(); ++i)
                             {
                                 local_sum += data[i] * data[i]; // 模拟计算
                             }
                             sum += local_sum;

                             // 添加一些延迟来观察线程行为
                             this_thread::sleep_for(chrono::microseconds(1));
                         });

            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration<double>(end - start).count();

            cout << "计算结果: " << sum.load() << endl;
            cout << "执行时间: " << fixed << setprecision(3) << duration << " 秒" << endl;
            cout << "实际使用的最大线程数: " << monitor->get_max_threads() << endl;
            cout << "唯一线程总数: " << monitor->get_unique_thread_count() << endl;
        }
    }

    // 测试2: Task Arena控制
    void test_task_arena_control()
    {
        cout << "\n"
             << string(60, '=') << endl;
        cout << "测试2: Task Arena并发控制" << endl;
        cout << string(60, '=') << endl;

        const int num_tasks = 100;
        vector<shared_ptr<LargeDataBlock>> blocks;

        // 生成测试数据
        for (int i = 0; i < num_tasks; ++i)
        {
            blocks.push_back(make_shared<LargeDataBlock>(i, 5000));
        }

        vector<int> arena_sizes = {1, 4, 8};

        for (int arena_size : arena_sizes)
        {
            cout << "\n--- Arena大小: " << arena_size << " ---" << endl;

            delete monitor;
            monitor = new DetailedThreadMonitor();
            mem_monitor.start_monitoring();

            vector<double> results(num_tasks);

            // 创建task_arena
            task_arena arena(arena_size);

            auto start = chrono::high_resolution_clock::now();

            arena.execute([&]()
                          { parallel_for(blocked_range<int>(0, num_tasks),
                                         [&](const blocked_range<int> &range)
                                         {
                                             for (int i = range.begin(); i != range.end(); ++i)
                                             {
                                                 results[i] = blocks[i]->compute_result();
                                                 mem_monitor.update_peak();
                                             }
                                         }); });

            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration<double>(end - start).count();

            cout << "处理了 " << num_tasks << " 个数据块" << endl;
            cout << "执行时间: " << fixed << setprecision(3) << duration << " 秒" << endl;
            cout << "吞吐量: " << (num_tasks / duration) << " 块/秒" << endl;

            monitor->print_detailed_stats();

            // 验证结果
            double avg_result = accumulate(results.begin(), results.end(), 0.0) / results.size();
            cout << "平均计算结果: " << fixed << setprecision(2) << avg_result << endl;
        }
    }

    // 测试3: 内存分配器控制
    void test_memory_allocator_control()
    {
        cout << "\n"
             << string(60, '=') << endl;
        cout << "测试3: TBB内存分配器控制" << endl;
        cout << string(60, '=') << endl;

        const int num_blocks = 1000;
        const size_t block_size = 10000;

        // 测试不同的内存分配策略
        cout << "\n--- 标准分配器 ---" << endl;
        test_allocator_performance<std::allocator<double>>(num_blocks, block_size, "标准分配器");

        cout << "\n--- TBB可扩展分配器 ---" << endl;
        test_allocator_performance<scalable_allocator<double>>(num_blocks, block_size, "TBB可扩展分配器");

        cout << "\n--- TBB缓存对齐分配器 ---" << endl;
        test_allocator_performance<cache_aligned_allocator<double>>(num_blocks, block_size, "TBB缓存对齐分配器");
    }

    // 测试4: Pipeline内存控制
    void test_pipeline_memory_control()
    {
        cout << "\n"
             << string(60, '=') << endl;
        cout << "测试4: Pipeline内存流控制" << endl;
        cout << string(60, '=') << endl;

        delete monitor;
        monitor = new DetailedThreadMonitor();
        mem_monitor.start_monitoring();

        const int total_items = 10000;
        atomic<int> produced{0};
        atomic<int> processed{0};
        atomic<int> consumed{0};

        // 控制内存使用的Pipeline
        concurrent_vector<double> results;

        // 限制Pipeline的并发度来控制内存使用
        const int max_tokens = 4; // 限制同时处理的数据块数量

        auto start = chrono::high_resolution_clock::now();

        parallel_pipeline(max_tokens,
                          // 生产阶段
                          make_filter<void, shared_ptr<LargeDataBlock>>(
                              filter_mode::serial_in_order,
                              [&](flow_control &fc) -> shared_ptr<LargeDataBlock>
                              {
                                  int item = produced++;
                                  if (item >= total_items)
                                  {
                                      fc.stop();
                                      return nullptr;
                                  }

                                  auto block = make_shared<LargeDataBlock>(item, 1000);
                                  mem_monitor.record_allocation(block->data.size() * sizeof(double) + block->metadata.size());

                                  if (item % 1000 == 0)
                                  {
                                      cout << "已生产: " << item << " 项" << endl;
                                      mem_monitor.update_peak();
                                  }

                                  return block;
                              }) &

                              // 处理阶段
                              make_filter<shared_ptr<LargeDataBlock>, double>(
                                  filter_mode::parallel,
                                  [&](shared_ptr<LargeDataBlock> block) -> double
                                  {
                                      if (!block)
                                          return 0.0;

                                      processed++;
                                      double result = block->compute_result();
                                      mem_monitor.update_peak();

                                      return result;
                                  }) &

                              // 消费阶段
                              make_filter<double, void>(
                                  filter_mode::serial_in_order,
                                  [&](double result)
                                  {
                                      results.push_back(result);
                                      consumed++;

                                      // 模拟释放内存
                                      mem_monitor.record_deallocation(1000 * sizeof(double) + 100);

                                      if (consumed % 1000 == 0)
                                      {
                                          cout << "已消费: " << consumed << " 项" << endl;
                                      }
                                  }));

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration<double>(end - start).count();

        cout << "\nPipeline执行完成!" << endl;
        cout << "生产项目: " << produced.load() << endl;
        cout << "处理项目: " << processed.load() << endl;
        cout << "消费项目: " << consumed.load() << endl;
        cout << "执行时间: " << fixed << setprecision(3) << duration << " 秒" << endl;
        cout << "吞吐量: " << (total_items / duration) << " 项/秒" << endl;

        monitor->print_detailed_stats();
        mem_monitor.print_memory_stats();

        cout << "结果统计: 平均值 = " << (accumulate(results.begin(), results.end(), 0.0) / results.size()) << endl;
    }

private:
    template <typename Allocator>
    void test_allocator_performance(int num_blocks, size_t block_size, const string &name)
    {
        mem_monitor.start_monitoring();
        delete monitor;
        monitor = new DetailedThreadMonitor();

        auto start = chrono::high_resolution_clock::now();

        vector<vector<double, Allocator>> blocks(num_blocks);

        parallel_for(blocked_range<int>(0, num_blocks),
                     [&](const blocked_range<int> &range)
                     {
                         random_device rd;
                         mt19937 gen(rd());
                         uniform_real_distribution<> dis(0.0, 100.0);

                         for (int i = range.begin(); i != range.end(); ++i)
                         {
                             blocks[i].reserve(block_size);
                             for (size_t j = 0; j < block_size; ++j)
                             {
                                 blocks[i].push_back(dis(gen));
                             }

                             // 模拟一些计算
                             double sum = accumulate(blocks[i].begin(), blocks[i].end(), 0.0);
                             blocks[i][0] = sum / block_size; // 避免优化掉计算
                         }

                         mem_monitor.update_peak();
                     });

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration<double>(end - start).count();

        cout << name << " 性能:" << endl;
        cout << "  执行时间: " << fixed << setprecision(3) << duration << " 秒" << endl;
        cout << "  吞吐量: " << (num_blocks / duration) << " 块/秒" << endl;

        monitor->print_detailed_stats();
        mem_monitor.print_memory_stats();
    }
};

// ==================== 主函数 ====================
int main()
{
    cout << "TBB内存控制和并发控制功能测试程序" << endl;
    cout << "系统CPU核心数: " << thread::hardware_concurrency() << endl;

    TBBControlTests tests;

    try
    {
        // 运行各项测试
        tests.test_thread_control();

        this_thread::sleep_for(chrono::milliseconds(1000));
        tests.test_task_arena_control();

        this_thread::sleep_for(chrono::milliseconds(1000));
        tests.test_memory_allocator_control();

        this_thread::sleep_for(chrono::milliseconds(1000));
        tests.test_pipeline_memory_control();

        cout << "\n"
             << string(60, '=') << endl;
        cout << "所有测试完成!" << endl;
        cout << "\nTBB控制功能总结:" << endl;
        cout << "1. global_control: 全局线程数量控制" << endl;
        cout << "2. task_arena: 局部并发度控制" << endl;
        cout << "3. scalable_allocator: 可扩展内存分配" << endl;
        cout << "4. cache_aligned_allocator: 缓存对齐分配" << endl;
        cout << "5. pipeline token控制: 内存流量控制" << endl;
        cout << "6. task_scheduler_observer: 实时监控" << endl;
    }
    catch (const exception &e)
    {
        cerr << "测试执行错误: " << e.what() << endl;
        return 1;
    }

    return 0;
}