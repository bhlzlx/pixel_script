add_library( stone STATIC)

target_sources(stone
PRIVATE
    # astree.cpp
    token.cpp
    token_parser.cpp
    ast_builder.cpp
    # lexer.cpp
    vm/vm_object.cpp
    vm/vm_types.cpp
    vm/vm_env.cpp
    vm/vm_symbol.cpp
)

target_include_directories(stone
PUBLIC
    .
)