cmake_minimum_required(VERSION 3.16)
project(sndfile-example)

SET (CMAKE_BUILD_TYPE "Release")
#SET (CMAKE_BUILD_TYPE "Debug")

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++20")
SET (CMAKE_CXX_FLAGS_RELEASE "-O3")
SET (CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address")

SET (BASE_DIR ${CMAKE_SOURCE_DIR} )
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BASE_DIR}/../bin)

add_subdirectory(${BASE_DIR}/../../bit_stream/src ${CMAKE_BINARY_DIR}/bit_stream_build)

add_executable (wav_cp wav_cp.cpp)
target_link_libraries (wav_cp sndfile)

add_executable (wav_hist wav_hist.cpp)
target_link_libraries (wav_hist sndfile)

add_executable (wav_dct wav_dct.cpp)
target_link_libraries (wav_dct sndfile fftw3)

add_executable (wav_quant wav_quant.cpp)
target_link_libraries (wav_quant sndfile)

add_executable (wav_cmp wav_cmp.cpp)
target_link_libraries (wav_cmp sndfile)

add_executable (wav_effects wav_effects.cpp)
target_link_libraries (wav_effects sndfile)

add_executable (wav_quant_enc wav_quant_enc.cpp)
target_include_directories(wav_quant_enc PRIVATE ../../bit_stream/src)
target_link_libraries (wav_quant_enc bit_stream sndfile)

add_executable (wav_quant_dec wav_quant_dec.cpp)
target_include_directories(wav_quant_dec PRIVATE ../../bit_stream/src)
target_link_libraries(wav_quant_dec bit_stream sndfile) 

add_executable (dct_enc dct_enc.cpp)
target_include_directories(dct_enc PRIVATE ../../bit_stream/src)
target_link_libraries(dct_enc bit_stream sndfile fftw3)

add_executable (dct_dec dct_dec.cpp)
target_include_directories(dct_dec PRIVATE ../../bit_stream/src)
target_link_libraries(dct_dec bit_stream sndfile fftw3)

add_executable (wav_to_mono wav_to_mono.cpp)
target_link_libraries (wav_to_mono sndfile)