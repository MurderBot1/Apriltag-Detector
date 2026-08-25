# Web UI executable

set(WEB_UI_SOURCES
    main.cpp
    WebServer.cpp
    StreamHandler.cpp
    ConfigAPI.cpp
)

add_executable(web_ui ${WEB_UI_SOURCES})

target_include_directories(web_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(web_ui PRIVATE
    common
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(web_ui PRIVATE
        ws2_32
    )
endif()

# Install
install(TARGETS web_ui DESTINATION bin)

# Copy web assets
if(BUILD_WEB_UI)
    add_custom_command(TARGET web_ui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/web
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/web
        COMMENT "Copying web assets to build directory"
    )
endif()
