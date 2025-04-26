# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(DynamicImageViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required Qt packages
# CMakeLists.txt - MODIFY to include required modules
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# If still encountering issues, explicitly add:
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# Define source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    imageviewer.cpp
    ImageLoader.cpp
)

# Define header files
set(HEADERS
    mainwindow.h
    imageviewer.h
    ImageLoader.h
)

# Create executable
add_executable(DynamicImageViewer
    ${SOURCES}
    ${HEADERS}
)

# Link Qt libraries
target_link_libraries(DynamicImageViewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Install configuration
install(TARGETS DynamicImageViewer
    RUNTIME DESTINATION bin
)
