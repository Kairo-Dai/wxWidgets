
################################################################################
# wxWidgets dependency (from local build-my/install_win32)
#
# Assumes you configured/installed wxWidgets using build-my\build.bat and the
# install prefix was: build-my\install_win32.
################################################################################

# Root of the local wxWidgets install used by this demo.
set(wxWidgets_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../install_win32"
    CACHE PATH "Root of local wxWidgets install used by WxwidgetPro demos"
)

# Point directly at the wxWidgets *config* package under that install tree.
set(wxWidgets_DIR
    "${wxWidgets_ROOT}/lib/cmake/wxWidgets"
    CACHE PATH "Path to wxWidgetsConfig.cmake"
)

# Use wxWidgets CMake *config* package (modern, imported targets interface).
# For a monolithic wxWidgets build (wxBUILD_MONOLITHIC=ON), the config exports
# a single "mono" component, but the aggregate target wxWidgets::wxWidgets is
# always provided. We only need that aggregate target here, so we don't
# request specific components to stay compatible with both mono and multi-lib
# installs.
find_package(wxWidgets CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Use a reasonably modern C++ standard.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Library: BorderlessRoundedWindowBase
# ---------------------------------------------------------------------------
add_library(BorderlessRoundedWindowBase
    BorderlessRoundedWindowBase.cpp
)

# Public include dir so other targets can `#include "BorderlessRoundedWindowBase.h"`.
target_include_directories(BorderlessRoundedWindowBase
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
)

# Link against wxWidgets aggregate target.
target_link_libraries(BorderlessRoundedWindowBase
    PUBLIC
        wxWidgets::wxWidgets
)

# ---------------------------------------------------------------------------
# Example executable using the base class
# ---------------------------------------------------------------------------
add_executable(BorderlessRoundedWindowExample
    ExampleBorderlessRoundedWindow.cpp
)

# Example depends on both wxWidgets and our base library.
target_link_libraries(BorderlessRoundedWindowExample
    PRIVATE
        BorderlessRoundedWindowBase
)
