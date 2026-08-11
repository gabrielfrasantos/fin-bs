add_executable(fin.optimize_test)
emil_build_for(fin.optimize_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.optimize_test)

target_link_libraries(fin.optimize_test PUBLIC
    gmock_main
    fin.optimize
)

target_sources(fin.optimize_test PRIVATE
    StrategyScenario.hpp
    TestObjective.cpp
    TestRecommendation.cpp
    TestRiskLimits.cpp
    TestStrategyObjective.cpp
    TestStrategySetting.cpp
    TestStrikeSizingOptimizer.cpp
)
