cmake_minimum_required(VERSION 3.22)
project(MoonMicCapsuleBench LANGUAGES CXX)

# Stage 5 test bench for the Physical Capsule Engine. Standalone: no JUCE, no plugin.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CapsuleBench bench.cpp ../../plugin/source/dsp/CapsuleEngine.h)
target_include_directories(CapsuleBench PRIVATE ../../plugin/source/dsp)
if(MSVC)
    target_compile_options(CapsuleBench PRIVATE /W4 /fp:precise)
else()
    target_compile_options(CapsuleBench PRIVATE -Wall -Wextra)
endif()
