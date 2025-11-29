add_executable(AnimatedControlExample WIN32
    AnimatedControlExample.cpp
    SimpleAnimatedBanner.cpp
    SimpleAnimatedBanner.h
)

target_include_directories(AnimatedControlExample
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
)

target_link_libraries(AnimatedControlExample
    PRIVATE
        wxWidgets::wxWidgets
)

install(TARGETS AnimatedControlExample
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

