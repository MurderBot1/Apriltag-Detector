# Pose finder executable

set(POSE_FINDER_SOURCES
    main.cpp
    PoseSolver.cpp
)

add_executable(pose_finder ${POSE_FINDER_SOURCES})

target_include_directories(pose_finder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/camera_detector
)

target_link_libraries(pose_finder PRIVATE
    common
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(pose_finder PRIVATE
        ws2_32
    )
endif()

# Install
install(TARGETS pose_finder DESTINATION bin)
