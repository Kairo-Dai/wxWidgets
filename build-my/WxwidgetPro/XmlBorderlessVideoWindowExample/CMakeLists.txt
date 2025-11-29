add_executable(XmlBorderlessVideoWindowExample WIN32
    XmlBorderlessVideoWindowExample.cpp
)

target_link_libraries(XmlBorderlessVideoWindowExample
    PRIVATE
        BorderlessRoundedWindowBase
        wxWidgets::wxWidgets
)

# Copy XRC resources next to the built executable so the demo can run directly
# from the build tree.
add_custom_command(TARGET XmlBorderlessVideoWindowExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_LIST_DIR}/rc"
            "$<TARGET_FILE_DIR:XmlBorderlessVideoWindowExample>/rc"
)

install(TARGETS XmlBorderlessVideoWindowExample
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# Install the XRC resources alongside the executable for the installed tree.
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/rc"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

