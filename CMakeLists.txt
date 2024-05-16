cmake_minimum_required(VERSION 3.14)

project(QMQTT2SQL LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

configure_file(res/qmqtt2sql.service.in ${CMAKE_CURRENT_BINARY_DIR}/qmqtt2sql.service @ONLY)

# https://stackoverflow.com/a/43355735
function(IndicateExternalFile _target)
    foreach(_file IN ITEMS ${ARGN})
        if ((IS_ABSOLUTE "${_file}" AND EXISTS "${_file}") OR
            (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_file}"))
            target_sources(${_target} PRIVATE "${_file}")
        endif()
    endforeach()
endfunction()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Mqtt Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Mqtt Sql)

add_executable(QMQTT2SQL
  src/main.cpp
  src/mqttsubscriber.h src/mqttsubscriber.cpp
  src/mqtt2sqlconfig.h src/mqtt2sqlconfig.cpp
)
target_link_libraries(QMQTT2SQL Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Mqtt Qt${QT_VERSION_MAJOR}::Sql)

IndicateExternalFile(${PROJECT_NAME} "README.md" "res/qmqtt2sql.ini" "res/qmqtt2sql.service.in")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT application)
install(FILES res/qmqtt2sql.ini DESTINATION ${CMAKE_INSTALL_SYSCONFDIR} COMPONENT data)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmqtt2sql.service DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system/ COMPONENT data)

