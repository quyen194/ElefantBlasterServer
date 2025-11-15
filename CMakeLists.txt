# CMakeList.txt : CMake project for BomberMan, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# prevent installing to system directories.
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

# Declare the project
project ("ElefantBlasterServer" LANGUAGES C CXX)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/dist/${BUILD_CONFIGURATION_NAME}/${PROJECT_NAME}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/dist/${BUILD_CONFIGURATION_NAME}/${PROJECT_NAME}")

if(MSVC)
    if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
        add_definitions(/MP)                # parallelize each target, unless Ninja is the generator
    endif()
endif()

# Set the name of the executable
set(EXECUTABLE_NAME ${PROJECT_NAME})

# Create an executable
add_executable(${EXECUTABLE_NAME})

# Add include directories
target_include_directories(${EXECUTABLE_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB_RECURSE PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/src/network/protocols/*")

# Add your sources to the target
target_sources (${EXECUTABLE_NAME}
PRIVATE
    src/entrypoint/entrypoint.cpp
    ${PROTO_FILES}
)

# What is iosLaunchScreen.storyboard? This file describes what Apple's mobile platforms
# should show the user while the application is starting up. If you don't include one,
# then you get placed in a compatibility mode that does not allow HighDPI.
# This file is referenced inside Info.plist.in, where it is marked as the launch screen file.
# It is also ignored on non-Apple platforms.

# To get an app icon on Apple platforms, add it to your executable.
# Afterward, the image file in Info.plist.in.
if(APPLE)
    target_sources("${EXECUTABLE_NAME}" PRIVATE "resources/logo.png")
endif()

# Set C++ version
target_compile_features(${EXECUTABLE_NAME} PUBLIC cxx_std_20)

include(FetchContent)

# glm (C++ mathematics library for graphics software)
FetchContent_Declare(
    glm
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/libs/glm
)
FetchContent_MakeAvailable(glm)

# nlohmann_json (JSON for Modern C++)
FetchContent_Declare(
    nlohmann_json
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/libs/json
)
FetchContent_MakeAvailable(nlohmann_json)

# Protocol Buffers (protobuf)
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    protobuf
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/libs/protobuf
)
FetchContent_MakeAvailable(protobuf)

# Link SDL to our executable. This also makes its include directory available to us.
target_link_libraries(${EXECUTABLE_NAME} PUBLIC
    glm::glm-header-only
    nlohmann_json::nlohmann_json
    protobuf::libprotobuf
    websocketpp::websocketpp
)

# ===================================================================
# ASSET & RESOURCE HANDLING – FULLY AUTOMATIC FOR YOUR BOMBERMAN PROJECT
# Supports: Windows | Linux | macOS | iOS | Android | Web (Emscripten)
# Automatically copies:
#   - assets/      → all game data (audio, graphics, levels, etc.)
#   - resources/   → logo.png, iosLaunchScreen.storyboard, Info.plist.in
# ===================================================================

set(DIST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Game data folder
set(ASSETS_DIR "${CMAKE_CURRENT_LIST_DIR}/assets")
# Bundle resources (Apple-specific: icons, splash, plist, etc.)
set(RESOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/resources")

# 1. Recursively collect all files in assets/
file(GLOB_RECURSE ASSET_FILES "${ASSETS_DIR}/*")
list(FILTER ASSET_FILES EXCLUDE REGEX "[\\/]\\.git[\\/]|\\.gitkeep$|\\.DS_Store$|~$|\\.tmp$|\\.bak$")

# 2. Collect all files in resources/
file(GLOB RESOURCE_FILES "${RESOURCES_DIR}/*")
list(FILTER RESOURCE_FILES EXCLUDE REGEX "\\.gitkeep$|\\.DS_Store$|~$")

# Debug output
list(LENGTH ASSET_FILES ASSET_COUNT)
list(LENGTH RESOURCE_FILES RES_COUNT)
message(STATUS "[Assets] Found ${ASSET_COUNT} game asset(s) in '${ASSETS_DIR}'")
message(STATUS "[Resources] Found ${RES_COUNT} bundle resource(s) in '${RESOURCES_DIR}'")

# ===================================================================
# APPLE (macOS + iOS): Embed everything into the .app bundle under Resources/
# ===================================================================
if(APPLE)
    macro(add_to_bundle input_file base_dir)
        file(RELATIVE_PATH rel_path "${base_dir}" "${input_file}")
        get_filename_component(rel_dir "${rel_path}" DIRECTORY)
        target_sources(${EXECUTABLE_NAME} PRIVATE "${input_file}")
        set_property(SOURCE "${input_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${rel_dir}")
    endmacro()

    foreach(file ${ASSET_FILES})
        add_to_bundle("${file}" "${ASSETS_DIR}")
    endforeach()
    foreach(file ${RESOURCE_FILES})
        add_to_bundle("${file}" "${RESOURCES_DIR}")
    endforeach()

# ===================================================================
# DESKTOP (Windows/Linux): Copy both trees next to the executable
# ===================================================================
else()
    macro(copy_tree file_list base_dir prefix)
        foreach(file ${file_list})
            file(RELATIVE_PATH rel "${base_dir}" "${file}")
            get_filename_component(rel_dir "${rel}" DIRECTORY)
            if(ANDROID)
                set(dest "${MOBILE_ASSETS_DIR}/${prefix}/${rel}")
            else()
                set(dest "${DIST_DIR}/${prefix}/${rel}")
            endif()
            get_filename_component(dest_dir "${dest}" DIRECTORY)
            add_custom_command(TARGET ${EXECUTABLE_NAME} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${dest_dir}"
            )
            add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${file}" "${dest}"
                COMMENT "Copying ${prefix}: ${rel}"
            )
        endforeach()
    endmacro()

    # Copy game assets
    copy_tree("${ASSET_FILES}" "${ASSETS_DIR}" "assets")
    if(APPLE)
        # Copy bundle resources (logo, storyboard, etc.)
        copy_tree("${RESOURCE_FILES}" "${RESOURCES_DIR}" "resources")
    endif()
endif()

# ===================================================================
# APPLE BUNDLE SETTINGS – Auto-include logo.png, storyboard, Info.plist
# ===================================================================
if(APPLE)
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${RESOURCES_DIR}/Info.plist.in"
        XCODE_GENERATE_SCHEME TRUE
        XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER "com.elefantblasterserver.vn"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.elefantblasterserver.vn"
        XCODE_ATTRIBUTE_CURRENTYEAR "2025"
        RESOURCE "${RESOURCE_FILES}"
    )
endif()

# Visual Studio: set as startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${EXECUTABLE_NAME}")

# ===================================================================
# macOS: Create draggable .app with proper dependency fixup
# ===================================================================
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    install(TARGETS ${EXECUTABLE_NAME}
        BUNDLE DESTINATION ./install COMPONENT Runtime
        RUNTIME DESTINATION ./install/bin COMPONENT Runtime
    )
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"$<TARGET_BUNDLE_DIR:${EXECUTABLE_NAME}>\" \"\" \"${CMAKE_BINARY_DIR}\")
    ")
    set(CPACK_GENERATOR "DragNDrop")
    include(CPack)
endif()
