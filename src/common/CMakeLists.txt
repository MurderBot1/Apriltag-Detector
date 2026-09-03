# Common library - shared by all components
set(COMMON_SOURCES
    # Logging
    logging/Logger.cpp
    logging/FileSink.cpp
    
    # Config
    config/GlobalConfig.cpp
    config/ConfigManager.cpp
    config/ConfigWatcher.cpp
    config/LimelightFMapLoader.cpp
    
    # IPC
    ipc/ProcessManager.cpp
    ipc/RestartPolicy.cpp
    ipc/HealthCheck.cpp
    ipc/SharedMemoryFrameBuffer.cpp
    
    # Camera
    camera/CameraFactory.cpp
    
    # NT4
    nt4/NT4Client.cpp
    nt4/NT4Publisher.cpp
    nt4/NT4Subscriber.cpp
    nt4/NT4Types.cpp
    
    # Utils
    utils/FileSystem.cpp
    utils/TimeUtils.cpp
    utils/Platform.cpp
)

# Platform-specific sources
if(PLATFORM_LINUX)
    list(APPEND COMMON_SOURCES
        camera/V4L2Camera.cpp
    )
endif()

if(PLATFORM_WINDOWS)
    list(APPEND COMMON_SOURCES
        camera/WMFCamera.cpp
    )
endif()

if(PLATFORM_MACOS)
    list(APPEND COMMON_SOURCES
        camera/AVCaptureCamera.cpp
    )
endif()

# Create common library
add_library(common STATIC ${COMMON_SOURCES})

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/third_party
)

# Link dependencies
target_link_libraries(common PRIVATE
    std::c++fs
)

# Platform-specific linking
if(PLATFORM_LINUX)
    target_link_libraries(common PRIVATE
        v4l2
    )
endif()

if(PLATFORM_WINDOWS)
    target_link_libraries(common PRIVATE
        mfplat
        mfreadwrite
        mfuuid
        propsys
        setupapi
        ws2_32
    )
endif()

if(PLATFORM_MACOS)
    find_library(AVFOUNDATION_FRAMEWORK AVFoundation)
    find_library(COREMEDIA_FRAMEWORK CoreMedia)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    
    target_link_libraries(common PRIVATE
        ${AVFOUNDATION_FRAMEWORK}
        ${COREMEDIA_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
endif()
