add_library( stone STATIC)

target_sources(stone
PRIVATE
    token.cpp
    token_parser.cpp
    ast_builder.cpp
    ast_node.cpp
    vm/vm_object.cpp
    vm/vm_types.cpp
    vm/vm_symbol.cpp
    vm/vm_module.cpp
    vm/vm_env.cpp
    vm/vm_eval.cpp
    vm/vm_userdata.cpp
    vm/internal_types/string_impl.cpp
)

target_include_directories(stone
PUBLIC
    .
)