cmake_minimum_required(VERSION 4.1)
project(gooSQL C)

set(CMAKE_C_STANDARD 23)

add_executable(gooSQL main.c
        lang/Lexer.c)
