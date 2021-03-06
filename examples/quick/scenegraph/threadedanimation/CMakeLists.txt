# Generated from threadedanimation.pro.

cmake_minimum_required(VERSION 3.14)
project(threadedanimation LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick/scenegraph/threadedanimation")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(threadedanimation
    main.cpp
    spinner.cpp spinner.h
)
target_link_libraries(threadedanimation PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(threadedanimation_resource_files
    "main.qml"
    "spinner.png"
)

qt6_add_resources(threadedanimation "threadedanimation"
    PREFIX
        "/scenegraph/threadedanimation"
    FILES
        ${threadedanimation_resource_files}
)

install(TARGETS threadedanimation
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(threadedanimation PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI Spinner
)

qt6_qml_type_registration(threadedanimation)
