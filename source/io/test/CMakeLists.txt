add_executable(fin.io_test)
emil_build_for(fin.io_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.io_test)

target_link_libraries(fin.io_test PUBLIC
    gmock_main
    fin.io
)

target_compile_definitions(fin.io_test PRIVATE
    FIN_BS_CONFIG_DIRECTORY="${PROJECT_SOURCE_DIR}/config"
    FIN_BS_DATA_DIRECTORY="${PROJECT_SOURCE_DIR}/data"
)

target_sources(fin.io_test PRIVATE
    TestBrokerConfig.cpp
    TestPriceCsvLoader.cpp
    TestReportWriter.cpp
    TestScenarioLoader.cpp
    TestTaxConfig.cpp
)
