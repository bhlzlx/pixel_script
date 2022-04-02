add_executable(token_parser_test)

target_sources(token_parser_test
PRIVATE
    token_parser_test.cpp
)

target_link_libraries(token_parser_test
PRIVATE
    stone
)