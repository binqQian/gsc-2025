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