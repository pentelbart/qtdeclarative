# Generated from qmllint.pro.

#####################################################################
## qmllint Tool:
#####################################################################

qt_add_tool(qmllint
    TOOLS_TARGET Qml # special case
    SOURCES
        ../shared/componentversion.cpp ../shared/componentversion.h
        ../shared/metatypes.h
        ../shared/scopetree.cpp ../shared/scopetree.h
        ../shared/typedescriptionreader.cpp ../shared/typedescriptionreader.h
        checkidentifiers.cpp checkidentifiers.h
        findwarnings.cpp findwarnings.h
        importedmembersvisitor.cpp importedmembersvisitor.h
        main.cpp
        qcoloroutput.cpp qcoloroutput.h
    INCLUDE_DIRECTORIES
        ../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmllint.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Syntax" "Verifier"
# _OPTION = "host_build"
