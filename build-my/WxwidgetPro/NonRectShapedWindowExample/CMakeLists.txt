add_executable(NonRectShapedWindowExample WIN32
    NonRectShapedWindowExample.cpp
)

target_link_libraries(NonRectShapedWindowExample
    PRIVATE
        wxWidgets::wxWidgets
)

install(TARGETS NonRectShapedWindowExample
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

