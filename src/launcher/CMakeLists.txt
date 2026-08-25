# Launcher executable

set(LAUNCHER_SOURCES
    main.cpp
)

add_executable(launcher ${LAUNCHER_SOURCES})

target_include_directories(launcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(launcher PRIVATE
    common
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(launcher PRIVATE
        ws2_32
    )
endif()

# Install
install(TARGETS launcher DESTINATION bin)
