cmake_minimum_required (VERSION 3.15)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (gsc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 定义spdlog编译库宏
add_definitions(-DSPDLOG_COMPILED_LIB)

if(UNIX)
    message("current platform: Linux ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-sign-compare -g -O3 -mavx2 -fopenmp -fno-omit-frame-pointer")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    message("current platform: Windows ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3 -mavx2 -fopenmp")
endif()

# 修改源文件路径（根据你的实际情况调整）
set(MAIN_SOURCE_FILES
    src/main.cpp
)

if(UNIX)
    # 添加头文件路径
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

    # 添加库文件路径
    link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../lib")
    link_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")

    add_library(objlib OBJECT ${MAIN_SOURCE_FILES})
    add_executable(gsc $<TARGET_OBJECTS:objlib>)

    target_link_libraries(gsc spdlog z pthread dl bsc brotlicommon brotlienc brotlidec tbb tbbmalloc)

    # 设置运行时库路径
    set_target_properties(gsc PROPERTIES
        INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

    set_target_properties(gsc PROPERTIES
        INSTALL_RPATH "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

else()
    # Windows平台配置
endif()

# ==================== Google Test 集成 ====================

# 添加选项来控制是否编译测试（可选）
option(BUILD_TESTS "Build the tests" ON)

if(BUILD_TESTS)
    message("Building tests enabled")
    
    # 引入 FetchContent 模块
    include(FetchContent)
    
    # 设置 FetchContent 使用全局缓存目录
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache" CACHE PATH "FetchContent base directory")
    
    # 下载并配置 Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    
    # 在 Windows 上防止覆盖父项目的编译器/链接器设置
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    # 使 googletest 可用
    FetchContent_MakeAvailable(googletest)
    
    # 启用测试
    enable_testing()
    
    # 如果你有需要测试的库代码，将其分离出来
    # 例如：将 main.cpp 以外的源文件编译成库
    set(LIB_SOURCE_FILES
        src/mmap.cpp
        # 如果将来有其它要测试的源文件，可以在这里添加（不包括 main.cpp）
    )
    
    # 如果有库代码，创建一个库
    if(LIB_SOURCE_FILES)
        add_library(gsc_lib ${LIB_SOURCE_FILES})
        target_include_directories(gsc_lib PUBLIC 
            "${CMAKE_CURRENT_SOURCE_DIR}/../include"
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        target_link_libraries(gsc_lib spdlog z pthread dl bsc brotlicommon brotlienc brotlidec tbb tbbmalloc)
        
        # 让主程序链接这个库
        target_link_libraries(gsc gsc_lib)
    endif()
    
    # 创建测试可执行文件
    file(GLOB TEST_SOURCES "tests/*.cpp")
    
    if(TEST_SOURCES)
        add_executable(gsc_tests ${TEST_SOURCES})
        
        # 链接 Google Test 和你的库
        target_link_libraries(gsc_tests 
            gtest_main
            gmock_main
        )
        
        # 如果有 gsc_lib，也链接它
        if(LIB_SOURCE_FILES)
            target_link_libraries(gsc_tests gsc_lib)
        endif()
        
        # 如果测试需要访问相同的头文件
        target_include_directories(gsc_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../include"
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        
    # 添加测试到 CTest
    # 使用 add_test 而不是 gtest_discover_tests 以避免在发现阶段运行测试可执行文件
    #（当前的测试可执行文件在运行时会访问项目根目录下的 input.txt）
    add_test(NAME gsc_tests COMMAND gsc_tests)
    # 确保测试在源码根目录下运行，这样相对路径 input.txt 能被找到
    set_tests_properties(gsc_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    message("Test executable: gsc_tests")
    else()
        message(WARNING "No test files found in tests/ directory")
    endif()
    
endif()