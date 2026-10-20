add_executable(fin.sim_test)
emil_build_for(fin.sim_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.sim_test)

target_link_libraries(fin.sim_test PUBLIC
    gmock_main
    fin.sim
)

target_sources(fin.sim_test PRIVATE
    TestBookMonteCarlo.cpp
    TestBookScript.cpp
    TestCalibration.cpp
    TestConvergence.cpp
    TestGbmPathGenerator.cpp
    TestHistoricalPath.cpp
    TestMonteCarlo.cpp
    TestOutcomeDistribution.cpp
    TestRng.cpp
    TestWheelScript.cpp
)
