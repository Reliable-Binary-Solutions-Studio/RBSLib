cmake_minimum_required(VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("rbslib")

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 提供启用 CUDA 的选项，默认为关闭
option(ENABLE_CUDA "Enable CUDA support" OFF)

if (ENABLE_CUDA)
    find_package(CUDA REQUIRED)
    message(STATUS "CUDA enabled")
    add_definitions(-DENABLE_CUDA) # 定义预处理宏
    #set(CUDA_NVCC_FLAGS -G;-g;-allow-unsupported-compiler)
    set(CUDA_NVCC_FLAGS -O3 -allow-unsupported-compiler -arch=compute_86 -code=sm_86,compute_86)
    add_definitions(-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)
endif()

# 收集头文件和源文件
file(GLOB RBS_LIB_HEADERS "rbslib/*.h" "rbslib/Windows/*.h" "rbslib/Linux/*.h")
file(GLOB RBS_LIB_SRC "rbslib/*.cpp" "rbslib/Windows/*.cpp" "rbslib/Linux/*.cpp")


# 添加可执行文件
if (ENABLE_CUDA)
    file(GLOB CUDA_SRC "rbslib/CUDA/*.cu" "rbslib/CUDA/*.cuh")
    cuda_add_executable(rbslib_test "test.cpp" ${RBS_LIB_SRC} ${RBS_LIB_HEADERS} ${CUDA_SRC})
    target_include_directories(rbslib_test PRIVATE ${CUDA_INCLUDE_DIRS})
    target_link_libraries(rbslib_test ${CUDA_LIBRARIES}) # 普通签名方式
else()
    add_executable(rbslib_test "test.cpp" ${RBS_LIB_SRC} ${RBS_LIB_HEADERS})
endif()


# 设置 C++ 标准版本
set_property(TARGET rbslib_test PROPERTY CXX_STANDARD 20)

# 显示 CUDA 编译的状态
if (ENABLE_CUDA)
    message(STATUS "CUDA support enabled for rbslib_test")
else()
    message(STATUS "CUDA support disabled for rbslib_test")
endif()
