add_executable(SemiTransparentWindowExample WIN32
    SemiTransparentWindowExample.cpp
)

target_link_libraries(SemiTransparentWindowExample
    PRIVATE
        BorderlessRoundedWindowBase
        wxWidgets::wxWidgets
)

install(TARGETS SemiTransparentWindowExample
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

