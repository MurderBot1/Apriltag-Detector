# Camera detector executable

set(DETECTOR_SOURCES
    main.cpp
    AprilTagDetector.cpp
    DetectorConfig.hpp
)

add_executable(camera_detector ${DETECTOR_SOURCES})

target_include_directories(camera_detector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(camera_detector PRIVATE
    common
)

# Platform-specific linking
if(PLATFORM_LINUX)
    target_link_libraries(camera_detector PRIVATE
        v4l2
    )
endif()

if(PLATFORM_WINDOWS)
    target_link_libraries(camera_detector PRIVATE
        mfplat
        mfreadwrite
        mfuuid
        propsys
        setupapi
        ws2_32
    )
endif()

# Install
install(TARGETS camera_detector DESTINATION bin)
