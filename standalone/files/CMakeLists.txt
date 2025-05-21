include_directories(${Boost_INCLUDE_DIR})

add_executable(
        mp_os_stdlone_files
        main.cpp
)

target_link_libraries(
        mp_os_stdlone_files
        PUBLIC
        mp_os_cmmn)
