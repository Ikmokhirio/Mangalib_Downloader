cmake_minimum_required(VERSION 3.0)

project(MangalibDownloader)

set(CMAKE_CXX_STANDARD 23)

if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
set(OPENSSL_PATH "${CMAKE_SOURCE_DIR}/openssl-3/x64")
else()
set(OPENSSL_PATH "${CMAKE_SOURCE_DIR}/openssl-3/x86")
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)

# JSON
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory("${PROJECT_SOURCE_DIR}/Modules/cppjson")

#add_compile_definitions(NO_SPD_LOGGER)
#add_compile_definitions(IMGUI_APP_LOGGER)
add_compile_definitions(DAEDALUS_PLATFORM_WINDOWS)
add_compile_definitions(DAEDALUS_STATIC_BUILD)
add_compile_definitions(_WINSOCKAPI_)

# YOU CAN INSTALL DIRECTX SDK FROM URL :
set(DXSDK_DIR "C:/Program files (x86)/Microsoft DirectX SDK (June 2010)")
set(DAEDALUS_PATH "C:/Users/dryav/CLionProjects/Daedalus")

# SPDLOG
set(SPDLOG_BUILD_WARNINGS 1)
set(SPDLOG_NO_THREAD_ID ON)

add_executable(${PROJECT_NAME} WIN32 main.cpp MangalibAuthorizer.h MangalibAuthorizer.cpp Downloader.h Downloader.cpp Combiner.h HtmlCombiner.h HtmlCombiner.cpp RawCombiner.h RawCombiner.cpp UriParser.h DarkTheme.h DarkTheme.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC Modules/cppHttplib)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/Modules/cppjson/single_include")
target_include_directories(${PROJECT_NAME} PUBLIC "${OPENSSL_PATH}/include/")
target_include_directories(${PROJECT_NAME} PUBLIC Modules/cppBase64)
target_include_directories(${PROJECT_NAME} PUBLIC ${DAEDALUS_PATH}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${DAEDALUS_PATH}/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${DXSDK_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${DAEDALUS_PATH}/ThirdParty/imgui)
target_include_directories(${PROJECT_NAME} PUBLIC ${DAEDALUS_PATH}/ThirdParty/spdlog/include)

target_link_directories(${PROJECT_NAME} PRIVATE "${OPENSSL_PATH}/lib/")
if (build_type STREQUAL debug)
  target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${DAEDALUS_PATH}/Debug/static/Daedalus)
  target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${DAEDALUS_PATH}/Debug/static/imgui)
  target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${DAEDALUS_PATH}/Debug/static/spdlog)
elseif (build_type STREQUAL release)
  target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${DAEDALUS_PATH}/Release/static/Daedalus)
  target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${DAEDALUS_PATH}/Release/static/imgui)
  target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${DAEDALUS_PATH}/Release/static/spdlog)
endif()

target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR} ${DXSDK_DIR}/Lib/x64)
if (build_type STREQUAL debug)
message("Debug build")
target_link_libraries(${PROJECT_NAME} Daedalus imgui spdlogd d3d9 d3dx9 libssl nlohmann_json::nlohmann_json libcrypto)
elseif (build_type STREQUAL release)
message("Release build")
target_link_libraries(${PROJECT_NAME} Daedalus imgui spdlog d3d9 d3dx9 libssl nlohmann_json::nlohmann_json libcrypto)
endif()
