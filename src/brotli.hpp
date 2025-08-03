#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include "brotli/encode.h"
#include "brotli/decode.h"

class BrotliCompressor {
private:
    BrotliEncoderState* encoder;
    const size_t kBufferSize = 65536;  // 64KB

public:
    // 构造函数，可以设置 quality 和 window 参数
    BrotliCompressor(int quality = BROTLI_DEFAULT_QUALITY, int window = BROTLI_DEFAULT_WINDOW) {
        encoder = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
        if (!encoder) {
            throw std::runtime_error("Failed to create Brotli encoder instance");
        }

        // 设置质量参数 (0-11)
        BrotliEncoderSetParameter(encoder, BROTLI_PARAM_QUALITY, quality);
        
        // 设置窗口大小 (10-24)
        BrotliEncoderSetParameter(encoder, BROTLI_PARAM_LGWIN, window);
    }

    // 析构函数
    ~BrotliCompressor() {
        if (encoder) {
            BrotliEncoderDestroyInstance(encoder);
        }
    }

    // 压缩文件
    bool compressFile(const std::string& inputFile, const std::string& outputFile) {
        std::ifstream inFile(inputFile, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Failed to open input file: " << inputFile << std::endl;
            return false;
        }

        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output file: " << outputFile << std::endl;
            return false;
        }

        std::vector<uint8_t> inputBuffer(kBufferSize);
        std::vector<uint8_t> outputBuffer(kBufferSize);
        
        BrotliEncoderOperation op = BROTLI_OPERATION_PROCESS;
        size_t availableIn = 0;
        const uint8_t* nextIn = nullptr;
        
        bool done = false;

        while (!done) {
            // 如果输入缓冲区为空，从文件读取更多数据
            if (availableIn == 0 && !inFile.eof()) {
                inFile.read(reinterpret_cast<char*>(inputBuffer.data()), kBufferSize);
                availableIn = inFile.gcount();
                nextIn = inputBuffer.data();
                
                // 如果已经到达文件末尾，设置操作为完成
                if (inFile.eof()) {
                    op = BROTLI_OPERATION_FINISH;
                }
            }

            size_t availableOut = kBufferSize;
            uint8_t* nextOut = outputBuffer.data();

            // 执行压缩
            BROTLI_BOOL result = BrotliEncoderCompressStream(
                encoder,
                op,
                &availableIn,
                &nextIn,
                &availableOut,
                &nextOut,
                nullptr);

            if (result == BROTLI_FALSE) {
                std::cerr << "Compression failed" << std::endl;
                return false;
            }

            // 计算输出大小并写入文件
            size_t outputSize = kBufferSize - availableOut;
            if (outputSize > 0) {
                outFile.write(reinterpret_cast<char*>(outputBuffer.data()), outputSize);
            }

            // 检查是否完成
            done = BrotliEncoderIsFinished(encoder);
        }

        return true;
    }
    
    // 压缩内存中的数据
    uint32_t compressData(const uint8_t* data, size_t dataSize, std::vector<uint8_t> &compressedData, uint8_t off = 0) {
        compressedData.clear();
        compressedData.reserve(dataSize);
        for(uint8_t i=0; i<off; i++)
        {
            compressedData.push_back(0);
        }
        std::vector<uint8_t> outputBuffer(kBufferSize);
        
        const uint8_t* nextIn = data;
        size_t availableIn = dataSize;
        BrotliEncoderOperation op = BROTLI_OPERATION_FINISH; // 一次性压缩所有数据
        
        bool done = false;
        
        while (!done) {
            size_t availableOut = kBufferSize;
            uint8_t* nextOut = outputBuffer.data();
            
            // 执行压缩
            BROTLI_BOOL result = BrotliEncoderCompressStream(
                encoder,
                op,
                &availableIn,
                &nextIn,
                &availableOut,
                &nextOut,
                nullptr);
                
            if (result == BROTLI_FALSE) {
                throw std::runtime_error("Compression failed");
            }
            
            // 计算输出大小并添加到结果向量
            size_t outputSize = kBufferSize - availableOut;
            if (outputSize > 0) {
                compressedData.insert(compressedData.end(), 
                                     outputBuffer.data(), 
                                     outputBuffer.data() + outputSize);
            }
            
            // 检查是否完成
            done = BrotliEncoderIsFinished(encoder);
        }
        
        return compressedData.size();
    }
    
    // 重置编码器状态（用于重复使用同一实例进行多次压缩）
    void reset() {
        if (encoder) {
            BrotliEncoderDestroyInstance(encoder);
        }
        encoder = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
        if (!encoder) {
            throw std::runtime_error("Failed to create Brotli encoder instance");
        }
    }
};

class BrotliDecompressor {
private:
    BrotliDecoderState* decoder;
    const size_t kBufferSize = 65536;  // 64KB

public:
    // 构造函数
    BrotliDecompressor() {
        decoder = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
        if (!decoder) {
            throw std::runtime_error("Failed to create Brotli decoder instance");
        }
    }

    // 析构函数
    ~BrotliDecompressor() {
        if (decoder) {
            BrotliDecoderDestroyInstance(decoder);
        }
    }

    // 解压文件
    bool decompressFile(const std::string& inputFile, const std::string& outputFile) {
        std::ifstream inFile(inputFile, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Failed to open input file: " << inputFile << std::endl;
            return false;
        }

        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output file: " << outputFile << std::endl;
            return false;
        }

        std::vector<uint8_t> inputBuffer(kBufferSize);
        std::vector<uint8_t> outputBuffer(kBufferSize);
        
        size_t availableIn = 0;
        const uint8_t* nextIn = nullptr;
        BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;

        while (result != BROTLI_DECODER_RESULT_SUCCESS) {
            // 如果需要更多输入且未到达文件末尾
            if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
                if (inFile.eof()) {
                    std::cerr << "Unexpected end of input file" << std::endl;
                    return false;
                }
                
                inFile.read(reinterpret_cast<char*>(inputBuffer.data()), kBufferSize);
                availableIn = inFile.gcount();
                nextIn = inputBuffer.data();
                
                if (availableIn == 0) {
                    std::cerr << "Failed to read from input file" << std::endl;
                    return false;
                }
            }

            size_t availableOut = kBufferSize;
            uint8_t* nextOut = outputBuffer.data();

            // 执行解压
            result = BrotliDecoderDecompressStream(
                decoder,
                &availableIn,
                &nextIn,
                &availableOut,
                &nextOut,
                nullptr);

            if (result == BROTLI_DECODER_RESULT_ERROR) {
                std::cerr << "Decompression failed: " 
                          << BrotliDecoderErrorString(BrotliDecoderGetErrorCode(decoder)) 
                          << std::endl;
                return false;
            }

            // 计算输出大小并写入文件
            size_t outputSize = kBufferSize - availableOut;
            if (outputSize > 0) {
                outFile.write(reinterpret_cast<char*>(outputBuffer.data()), outputSize);
            }
        }

        return true;
    }
    
    // 解压内存中的数据
    uint32_t decompressData(const uint8_t* compressedData, size_t compressedSize, std::vector<uint8_t> &decompressedData) {
        decompressedData.clear();
        decompressedData.reserve(compressedSize * 2);  // 估计解压后大小为压缩大小的两倍
        std::vector<uint8_t> outputBuffer(kBufferSize);
        size_t availableIn = compressedSize;
        const uint8_t* nextIn = compressedData;
        BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
        
        while (result != BROTLI_DECODER_RESULT_SUCCESS) {
            size_t availableOut = kBufferSize;
            uint8_t* nextOut = outputBuffer.data();
            
            // 执行解压
            result = BrotliDecoderDecompressStream(
                decoder,
                &availableIn,
                &nextIn,
                &availableOut,
                &nextOut,
                nullptr);
                
            if (result == BROTLI_DECODER_RESULT_ERROR) {
                std::string errorMsg = "Decompression failed: ";
                errorMsg += BrotliDecoderErrorString(BrotliDecoderGetErrorCode(decoder));
                throw std::runtime_error(errorMsg);
            }
            
            // 计算输出大小并添加到结果向量
            size_t outputSize = kBufferSize - availableOut;
            if (outputSize > 0) {
                decompressedData.insert(decompressedData.end(), 
                                       outputBuffer.data(), 
                                       outputBuffer.data() + outputSize);
            }
            
            // 检查是否需要更多输入但已经没有输入了
            if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT && availableIn == 0) {
                throw std::runtime_error("Unexpected end of compressed data");
            }
        }
        
        return decompressedData.size();
    }
    
    // 重置解码器状态（用于重复使用同一实例进行多次解压）
    void reset() {
        if (decoder) {
            BrotliDecoderDestroyInstance(decoder);
        }
        decoder = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
        if (!decoder) {
            throw std::runtime_error("Failed to create Brotli decoder instance");
        }
    }
};

// int main(int argc, char* argv[]) {
//     if (argc < 4) {
//         std::cout << "Usage: " << argv[0] << " [compress/decompress] <input_file> <output_file> [quality] [window]" << std::endl;
//         std::cout << "  quality: 0-11 (default: 11)" << std::endl;
//         std::cout << "  window: 10-24 (default: 22)" << std::endl;
        
//         // 演示内存压缩示例
//         std::cout << "\nMemory compression example:" << std::endl;
        
//         // 创建一些测试数据
//         const char* testData = "This is a test string that will be compressed in memory and then decompressed.";
//         size_t dataSize = strlen(testData);
        
//         try {
//             // 设置压缩参数（quality=9, window=18）
//             BrotliCompressor memCompressor(9, 18);
            
//             // 压缩内存中的数据
//             std::vector<uint8_t> compressedData = memCompressor.compressData(
//                 reinterpret_cast<const uint8_t*>(testData), dataSize);
            
//             std::cout << "Original size: " << dataSize << " bytes" << std::endl;
//             std::cout << "Compressed size: " << compressedData.size() << " bytes" << std::endl;
            
//             // 解压数据
//             BrotliDecompressor memDecompressor;
//             std::vector<uint8_t> decompressedData = memDecompressor.decompressData(
//                 compressedData.data(), compressedData.size(), dataSize);
            
//             // 验证解压结果
//             bool success = (decompressedData.size() == dataSize && 
//                            memcmp(decompressedData.data(), testData, dataSize) == 0);
            
//             std::cout << "Decompressed size: " << decompressedData.size() << " bytes" << std::endl;
//             std::cout << "Verification: " << (success ? "Success" : "Failed") << std::endl;
//         } catch (const std::exception& e) {
//             std::cerr << "Memory compression example failed: " << e.what() << std::endl;
//         }
        
//         return 1;
//     }

//     std::string mode = argv[1];
//     std::string inputFile = argv[2];
//     std::string outputFile = argv[3];
    
//     int quality = BROTLI_DEFAULT_QUALITY;  // 默认为11
//     int window = BROTLI_DEFAULT_WINDOW;    // 默认为22
    
//     if (argc > 4) {
//         quality = std::stoi(argv[4]);
//         if (quality < 0 || quality > 11) {
//             std::cerr << "Quality must be between 0 and 11" << std::endl;
//             return 1;
//         }
//     }
    
//     if (argc > 5) {
//         window = std::stoi(argv[5]);
//         if (window < 10 || window > 24) {
//             std::cerr << "Window size must be between 10 and 24" << std::endl;
//             return 1;
//         }
//     }

//     try {
//         if (mode == "compress") {
//             std::cout << "Compressing " << inputFile << " to " << outputFile 
//                       << " (quality=" << quality << ", window=" << window << ")" << std::endl;
            
//             BrotliCompressor compressor(quality, window);
//             if (compressor.compressFile(inputFile, outputFile)) {
//                 std::cout << "Compression completed successfully" << std::endl;
//             } else {
//                 std::cerr << "Compression failed" << std::endl;
//                 return 1;
//             }
//         } else if (mode == "decompress") {
//             std::cout << "Decompressing " << inputFile << " to " << outputFile << std::endl;
            
//             BrotliDecompressor decompressor;
//             if (decompressor.decompressFile(inputFile, outputFile)) {
//                 std::cout << "Decompression completed successfully" << std::endl;
//             } else {
//                 std::cerr << "Decompression failed" << std::endl;
//                 return 1;
//             }
//         } else {
//             std::cerr << "Invalid mode. Use 'compress' or 'decompress'" << std::endl;
//             return 1;
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//         return 1;
//     }

//     return 0;
// }