cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "../shared_components" "components")

# Build only main and its transitive dependencies (skips the screen-only jpeg
# component and the host-test components scout_hal/unity/cmock).
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(scout_cam)
