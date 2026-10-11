cmake_minimum_required(VERSION 3.16)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Добавляем подпроекты
# add_subdirectory(Utils)
# add_subdirectory(src)

#
#
#

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "form")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/**.cpp" "src/**.ui" "src/**.h" "src/**.hpp")

# Статическая сборка
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${GUI_RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

