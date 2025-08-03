# Copyright (C) BGI-Reasearch - All Rights Reserved
# Unauthorized copying of this file, via any medium is strictly prohibited
# Proprietary and confidential
# Written by STomics development team <P_stomics_dev@genomics.cn>, May 2017
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (gsc)

set(CMAKE_CXX_STANDARD 17)
# exports compile_commands.json - used by clang tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(UNIX)
    message("current platform: Linux ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare -g -O0 -mavx2 -fopenmp -fno-omit-frame-pointer")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

else()
    message("current platform: Windows ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3 -mavx2 -fopenmp")
endif()


set(MAIN_SOURCE_FILES
    src/main.cpp
)

if(UNIX)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include") # 添加 Brotli 头文件路径
    link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../lib")
    link_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib") # 添加 Brotli 库路径

    add_library(objlib OBJECT ${MAIN_SOURCE_FILES})
    add_executable(gsc $<TARGET_OBJECTS:objlib>)

    #target_link_libraries(gsc z pthread dl hts bsc)
    target_link_libraries(gsc z pthread dl bsc brotlicommon brotlienc brotlidec)
else()
    # include_directories("classify"  )
    # link_directories("classify" "zlib")
    # add_executable (seqarc ${MAIN_SOURCE_FILES})
    # target_link_libraries(seqarc zlibwapi)
endif()

