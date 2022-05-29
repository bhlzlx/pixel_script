add_library(game_script STATIC)

target_sources(game_script
PRIVATE
    token.cpp
    token_parser.cpp
    ast_builder.cpp
    ast_node.cpp
    vm/vm_env.cpp
    vm/vm_register.cpp
    vm/internal_types/vm_object.cpp
    vm/internal_types/vm_types.cpp
    vm/internal_types/vm_symbol.cpp
    vm/internal_types/vm_module.cpp
    vm/internal_types/vm_primitive_types.cpp
    vm/internal_types/vm_userdata.cpp
    vm/stdlib/std_io.cpp
    vm/stdlib/std_string.cpp
    vm/stdlib/std_vec.cpp
    vm/stdlib/std_map.cpp
    vm/utils/vm_package_util.cpp
)

target_include_directories(game_script
PUBLIC
    .
)