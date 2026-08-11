add_executable(fin.pricing_test)
emil_build_for(fin.pricing_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.pricing_test)

target_link_libraries(fin.pricing_test PUBLIC
    gmock_main
    fin.pricing
)

target_sources(fin.pricing_test PRIVATE
    OptionScenario.hpp
    TestAssignmentModel.cpp
    TestBlackScholes.cpp
    TestGreeks.cpp
    TestImpliedVolatility.cpp
    TestNormalDistribution.cpp
)
