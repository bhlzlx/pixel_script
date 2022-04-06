add_executable(ast_builder_test)

target_sources(ast_builder_test
PRIVATE
    ast_builder_test.cpp
)

target_link_libraries(ast_builder_test
PRIVATE
    stone
)