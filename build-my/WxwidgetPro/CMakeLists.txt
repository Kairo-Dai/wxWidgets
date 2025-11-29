cmake_minimum_required(VERSION 3.16)

project(WxwidgetPro LANGUAGES CXX)

# Use a reasonably modern C++ standard for all demos.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

################################################################################
# wxWidgets dependency (from local build-my/install_win32)
################################################################################

include(GNUInstallDirs)

set(wxWidgets_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../install_win32"
    CACHE PATH "Root of local wxWidgets install used by WxwidgetPro demos"
)

set(wxWidgets_DIR
    "${wxWidgets_ROOT}/lib/cmake/wxWidgets"
    CACHE PATH "Path to wxWidgetsConfig.cmake"
)

find_package(OpenGL REQUIRED)
find_package(wxWidgets CONFIG REQUIRED)

add_subdirectory(BorderlessRoundedWindowBase)
add_subdirectory(SemiTransparentWindowExample)
add_subdirectory(NonRectShapedWindowExample)
add_subdirectory(AnimatedControlExample)
add_subdirectory(XmlBorderlessVideoWindowExample)
