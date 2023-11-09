cmake_minimum_required(VERSION 3.10)
project(gtktest C CXX)
set(PROJECT_TARGET weatherapp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED "gtk+-3.0")
pkg_check_modules(GTKMM REQUIRED "gtkmm-3.0")
pkg_check_modules(CURL REQUIRED "libcurl")
pkg_check_modules(PNG REQUIRED "libpng")

add_executable(${PROJECT_TARGET} weatherapp.cpp)
target_link_libraries(${PROJECT_TARGET} PUBLIC ${GTK_LIBRARIES} ${GTKMM_LIBRARIES} ${CURL_LIBRARIES} ${PNG_LIBRARIES})
target_include_directories(${PROJECT_TARGET} PUBLIC ${GTK_INCLUDE_DIRS} ${GTKMM_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
#target_link_libraries(curl-demo ${CURL_LIBRARIES})