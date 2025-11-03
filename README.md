# vcfComp
Extension of gsc compression tool.
test, qb-dev
```
# 新建构建目录（推荐 out-of-source build）
mkdir build
cd build

# 生成构建系统文件
cmake ..

# 编译
make

```

```
# 项目依赖(版本号)：
    - brotli   --
    - zstd -- 提高压缩速度，但降低压缩率
    - xxhash  -- 
    - oneTBB  --
    - mio  -- mmap工具
```

```
./build/gsc -i input.txt -o compressed_output.br
./build/gsc -i compressed_output.br -o decompressed_output.txt -d
./build/gsc -c input.txt decompressed_output.txt
```

## Todo List
- [x] 改仓库名、更新文件名
- [ ] cc/codex 解读genozip处理方式 同时阅读bgi_gsc，
### phase 1: I/O 处理 与 错误处理与验证、用户界面改进（小规模数据测试后再在大规模数据上测试）
- [x] 生产者-消费者框架：块处理
- [x] 内存控制，限速
- [ ] vcf.gz/gvcf.gz 解压到 vcf/gvcf 的同时，并行解析每行变体，分开存储每个字段的数据，从而变为一个待压缩的中间态
- [ ] 简单通过brotli压缩中间态生成gsc后缀格式的文件，然后将gsc格式文件解压并还原成vcf/gvcf（是否可以一步直接到vcf.gz/gvcf.gz），验证流程是否能跑通
- [x] 添加压缩过程中的错误恢复、完善日志记录系统
- [ ] 添加进度条显示、提供压缩统计报告
### phase 2: 压缩算法与文件架构
- [ ] 想想怎么定义文件压缩后的gsc格式文件码流，组织结构等
- [ ] 边界怎么处理？
- [ ] 能正常输入输出后，针对已有的数据处理方法（已经写好了相应的压缩算法代码），压缩各个字段（按行列分块）
### phase 3: 性能指标收集 与 格式转换（plink和之前华大讨论想要的格式，这个也有性能指标）
- [ ] 压缩率对比、压缩和解压速度测试、内存使用量监控（压缩和解压）、格式转换的指标
- [ ] 检索速率、检索来增加随机访问 

---
- GPU 处理 比特矩阵
