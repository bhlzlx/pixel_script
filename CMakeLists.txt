cmake_minimum_required(VERSION 3.21)

project(make_compiler)

add_executable( token_parser )

target_sources( token_parser 
PRIVATE
    src/token.cpp
    src/token_parser.cpp
)