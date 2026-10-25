cmake_minimum_required(VERSION 3.16)
project(Rate24_GUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic Qt tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

# Accept new Qt policies to suppress warnings
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

# Create the executable and link the source files
qt_add_executable(Rate24_GUI
    src/main.cpp
    src/Rate_24_Backend.cpp
    src/Activity.cpp
)

# Integrate QML into the C++ code
qt_add_qml_module(Rate24_GUI
    URI App
    VERSION 1.0
    QML_FILES qml/main.qml
)

# Link Qt libraries
target_link_libraries(Rate24_GUI PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)
