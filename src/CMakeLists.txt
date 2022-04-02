add_library( stone STATIC)

target_sources(stone
PRIVATE
    # astree.cpp
    token.cpp
    token_parser.cpp
    # lexer.cpp
)

target_include_directories(stone
PUBLIC
    .
)