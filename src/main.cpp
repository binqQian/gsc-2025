#include <iostream>
#include <string>
#include "brotli.hpp"
#include "xxhash/xxh3.h"
#include <fstream>

uint64_t calculateFileHash(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    XXH64_state_t* state = XXH64_createState();
    if (state == nullptr) {
        throw std::runtime_error("Failed to create XXH64 state");
    }

    XXH64_reset(state, 0);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        XXH64_update(state, buffer, file.gcount());
    }

    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    return hash;
}


int main(int argc, char* argv[]) {
    try {
        std::string inputFile;
        std::string outputFile;
        bool compressMode = true;
        bool checkMode = false;
        std::string checkFile1, checkFile2;

        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "-i" && i + 1 < argc) {
                inputFile = argv[++i];
            } else if (std::string(argv[i]) == "-o" && i + 1 < argc) {
                outputFile = argv[++i];
            } else if (std::string(argv[i]) == "-d") {
                compressMode = false;
            } else if (std::string(argv[i]) == "-c" && i + 2 < argc) {
                checkMode = true;
                checkFile1 = argv[++i];
                checkFile2 = argv[++i];
            }
        }

        if (checkMode) {
            // 校验模式：比较两个文件哈希
            try {
                uint64_t hash1 = calculateFileHash(checkFile1);
                uint64_t hash2 = calculateFileHash(checkFile2);
                if (hash1 == hash2) {
                    std::cout << "Hash verification succeeded: Files are identical" << std::endl;
                    return 0;
                } else {
                    std::cerr << "Hash verification failed: Files are different" << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Hash calculation error: " << e.what() << std::endl;
                return 1;
            }
        }

        if (inputFile.empty() || outputFile.empty()) {
            std::cerr << "Usage: " << argv[0] << " -i <input_file> -o <output_file> [-d] | -c <file1> <file2>" << std::endl;
            return 1;
        }

        if (compressMode) {
            BrotliCompressor compressor(11, 24);
            if (compressor.compressFile(inputFile, outputFile)) {
                std::cout << "Compression completed successfully" << std::endl;
            } else {
                std::cerr << "Compression failed" << std::endl;
                return 1;
            }
        } else {
            BrotliDecompressor decompressor;
            if (decompressor.decompressFile(inputFile, outputFile)) {
                std::cout << "Decompression completed successfully" << std::endl;
            } else {
                std::cerr << "Decompression failed" << std::endl;
                return 1;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
