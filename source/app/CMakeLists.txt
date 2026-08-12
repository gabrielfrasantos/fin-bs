add_executable(fin.app)

target_link_libraries(fin.app PRIVATE
    fin.gui
)

target_compile_definitions(fin.app PRIVATE
    FIN_BS_CONFIG_DIRECTORY="${PROJECT_SOURCE_DIR}/config"
)

target_sources(fin.app PRIVATE
    Main.cpp
)
