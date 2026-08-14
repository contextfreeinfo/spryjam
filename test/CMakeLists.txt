cmake_minimum_required(VERSION 3.13)

project(spryjam-test)

include_directories(../src)

# add_compile_options(
#     -fsanitize=address
#     -fno-omit-frame-pointer
#     -g
# )

# add_link_options(
#     -fsanitize=address
# )

add_executable(test
    test.c
    ../src/draw.c
)
