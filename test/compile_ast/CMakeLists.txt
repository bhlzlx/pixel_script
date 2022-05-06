add_executable(compile_ast_test)

target_sources(compile_ast_test
PRIVATE
    compile_ast_test.cpp
)

target_link_libraries(compile_ast_test
PRIVATE
    stone
)