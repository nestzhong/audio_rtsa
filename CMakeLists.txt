cmake_minimum_required(VERSION 2.4)
project(audio_rtsa)

include_directories(
    ${THIRD_PARTY}/file_parser/include
    ${UTILITY})

aux_source_directory(${UTILITY} COMMON_FILES)
add_executable(audio_rtsa audio_rtsa.c ${COMMON_FILES})
target_link_libraries(audio_rtsa agora-rtc-sdk file_parser ${LIBS} asound)

