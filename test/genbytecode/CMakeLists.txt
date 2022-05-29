add_executable(genbytecode)

target_sources(genbytecode
PRIVATE
    genbytecode.cpp
)

target_link_libraries(genbytecode
PRIVATE
    game_script
)