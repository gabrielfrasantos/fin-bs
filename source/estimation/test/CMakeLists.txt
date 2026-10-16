add_executable(fin.estimation_test)
emil_build_for(fin.estimation_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.estimation_test)

target_link_libraries(fin.estimation_test PUBLIC
    gmock_main
    fin.estimation
    fin.estimation_test_doubles
)

target_sources(fin.estimation_test PRIVATE
    TestDriftEstimator.cpp
    TestEmaVolatilityEstimator.cpp
    TestGbmSeries.cpp
    TestHistoricalVolatility.cpp
    TestLineFit.cpp
    TestLogReturns.cpp
    TestMarketParameters.cpp
    TestPriceSeries.cpp
    TestRlsVolatilityEstimator.cpp
)
