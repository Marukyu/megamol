get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(ROOT_DIR "${SELF_DIR}/../../" ABSOLUTE)

include(${SELF_DIR}/MegaMolCore-target.cmake)

set(MegaMolCore_LIBRARIES MegaMolCore)
set(MegaMolCore_INCLUDE_DIRS "${ROOT_DIR}/include")

set(MegaMolCore_FOUND 1)

get_property(self_target_location TARGET MegaMolCore PROPERTY LOCATION_${CMAKE_BUILD_TYPE})
message(STATUS "Found MegaMolCore: ${self_target_location}")

