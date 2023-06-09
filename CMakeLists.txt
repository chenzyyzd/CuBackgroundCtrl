cmake_minimum_required (VERSION 3.13)
project (CuBackgroundCtrl)

set(CMAKE_C_STANDARD_LIBRARY "glibc")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_LIBRARY "libc++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.c"
)
set(INCS
    "${CMAKE_CURRENT_LIST_DIR}/src"
)

add_executable(CuBackgroundCtrl ${SRC})
target_include_directories(CuBackgroundCtrl PRIVATE ${INCS})
target_link_libraries(CuBackgroundCtrl PRIVATE c++_static dl)

set(THIS_COMPILE_FLAGS
    -fdata-sections -ffunction-sections -fno-threadsafe-statics -fno-omit-frame-pointer -fno-rtti
    -fvisibility=hidden -fvisibility-inlines-hidden -O3 -D_GNU_SOURCE
    -fno-strict-aliasing -fomit-frame-pointer -finline-functions 
    -march=armv8-a
)
set(THIS_LINK_FLAGS
    -ffixed-x18 -Wl,--hash-style=both -fPIE -Wl,-exclude-libs,ALL -Wl,--gc-sections -static -O3
    -fvisibility=hidden -fvisibility-inlines-hidden -Wl,--icf=all,--strip-all -march=armv8-a
)

target_compile_options(CuBackgroundCtrl PRIVATE ${THIS_COMPILE_FLAGS})
target_link_options(CuBackgroundCtrl PRIVATE ${THIS_LINK_FLAGS})
