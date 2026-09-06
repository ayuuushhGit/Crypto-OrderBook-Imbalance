# CMakeList.txt : CMake project for CoinBaseDataFetch, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (CoinBaseDataFetch "CoinBaseDataFetch.cpp" "CoinBaseDataFetch.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CoinBaseDataFetch PROPERTY CXX_STANDARD 20)
endif()

# Link the nlohmann_json library to the executable
# Link BOTH json and cpr to the executable
target_link_libraries(CoinBaseDataFetch PRIVATE nlohmann_json::nlohmann_json cpr::cpr)

# TODO: Add tests and install targets if needed.
