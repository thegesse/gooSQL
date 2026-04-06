cmake_minimum_required(VERSION 4.1)
project(gooSQL C)

set(CMAKE_C_STANDARD 23)

add_executable(gooSQL main.c
        lang/Lexer.c
        lang/Parser.c
        lang/Parser.h
        lang/Ast.h
        lang/Ast.c
        runtime/Databse.h
        runtime/Database.c
        storage/Storage.h
        storage/Storage.c)
