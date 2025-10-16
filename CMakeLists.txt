cmake_minimum_required(VERSION 3.20)
project(spectral_main)

add_library(cmpl_flags INTERFACE)
target_compile_features(cmpl_flags INTERFACE cxx_std_23)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_executable(${PROJECT_NAME} main.cpp
        inc/SpctCalculationEngine.h
        inc/SpctProcessingFunctions.h
        inc/SpctOscillator.h
        inc/SpctDomainSpecific.h
        inc/SpctOscillatorStack.h
        inc/SpctBufferManager.h
        inc/SpctCircularBuffer.h
        inc/SpctInstanceController.h
        inc/SpctWavetables.h
        inc/SpctFxParameters.h
        inc/SpctExponentLUT.h
        inc/SpctTriggerManager.h
        src/SpctInstanceController.cpp
        test/SpctWTTest.h
        test/SpctBufferManagerTest.h
        test/VoiceSampleArray.h
        test/SpctRealVoiceTest.h)
target_link_libraries(${PROJECT_NAME} PUBLIC cmpl_flags)
target_include_directories(${PROJECT_NAME} PUBLIC inc)
