# Generated from qml.pro.

add_subdirectory(referenceexamples)
add_subdirectory(tutorials)
add_subdirectory(shell)
if(TARGET Qt::Quick)
    add_subdirectory(qmlextensionplugins)
    add_subdirectory(xmlhttprequest)
endif()
if(QT_FEATURE_qml_network AND TARGET Qt::Quick)
    add_subdirectory(networkaccessmanagerfactory)
endif()
