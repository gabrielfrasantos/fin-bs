add_executable(fin.present_test)
emil_build_for(fin.present_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.present_test)

target_link_libraries(fin.present_test PUBLIC
    gmock_main
    fin.present
)

target_compile_definitions(fin.present_test PRIVATE
    FIN_BS_CONFIG_DIRECTORY="${PROJECT_SOURCE_DIR}/config"
    FIN_BS_DATA_DIRECTORY="${PROJECT_SOURCE_DIR}/data"
)

target_sources(fin.present_test PRIVATE
    RunScenario.hpp
    TestFanChartSeries.cpp
    TestPriceHistory.cpp
    TestResultTable.cpp
    TestRunController.cpp
    TestScenarioDocument.cpp
)
