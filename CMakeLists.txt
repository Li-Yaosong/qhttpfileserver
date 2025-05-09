cmake_minimum_required(VERSION 3.16)

project(httpserver LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ENABLE_GUI "Enable GUI" ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
list(APPEND QT_MODULES Core HttpServer)
if(ENABLE_GUI)
  message(STATUS "GUI enabled")
  list(APPEND QT_MODULES Gui Widgets)
  add_compile_definitions(ENABLE_GUI)
  set(APP_ICON ":/httpfileserver.ico")
  add_compile_definitions(APP_ICON=\"${APP_ICON}\")
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_MODULES})

add_executable(httpserver WIN32
  main.cpp
  httpfileserver.h httpfileserver.cpp
  resource.qrc
  app_icon.rc
)
target_link_libraries(httpserver Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::HttpServer)
if(ENABLE_GUI)
  target_link_libraries(httpserver Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)
endif()

include(GNUInstallDirs)
install(TARGETS httpserver
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
