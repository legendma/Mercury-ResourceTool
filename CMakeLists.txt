
# FSBank
add_library( FSBank SHARED IMPORTED )
if( WIN32 )
    set_target_properties( FSBank PROPERTIES
                           IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/ots/fmod/lib/x64/fsbank.dll
                           IMPORTED_IMPLIB ${PROJECT_SOURCE_DIR}/ots/fmod/lib/x64/fsbank_vc.lib
    )
elseif( APPLE )
    set_target_properties( FSBank PROPERTIES
                           IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/ots/fmod/lib/MacOS/libfsbank.dylib
    )
endif()
target_include_directories( FSBank INTERFACE ${PROJECT_SOURCE_DIR}/ots/fmod/include )

# stb
set( STB_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/ots/stb/include )

# cjson
file( GLOB CJSON_FILES
      cjson/cjson.c
      cjson/cjson.h
)
source_group( cjson FILES ${CJSON_FILES} )

# ResourcePackager
if( WIN32 )
    file( GLOB PLATFORM_FILES
          ResourceUtilitiesMSW.cpp
    )
elseif( APPLE )
    file( GLOB PLATFORM_FILES
            ResourceUtilitiesMacOS.mm
    )
endif()
file( GLOB SRC_FILES
      ${PROJECT_SOURCE_DIR}/assets/assets.json
      AssetFile.cpp
      AssetFile.hpp
      assets.json
      ExportFont.cpp
      ExportFont.hpp
      ExportModel.cpp
      ExportModel.hpp
      ExportSounds.cpp
      ExportSounds.hpp
      ExportTexture.cpp
      ExportTexture.hpp
      ResourcePackager.cpp
      ResourcePackager.hpp
      ResourceUtilities.hpp
)
source_group( src FILES ${SRC_FILES} ${PLATFORM_FILES} )

list( JOIN RESOURCE_PACKAGE_ARGS " " RESOURCE_PACKAGE_ARGS_JOINED )

add_executable( ResourcePackager EXCLUDE_FROM_ALL
                CMakeLists.txt
                ${CJSON_FILES}
                ${SRC_FILES}
                ${PLATFORM_FILES}
)
target_link_libraries( ResourcePackager PUBLIC
                       Assimp
                       FSBank
)
if( APPLE )
    target_link_libraries( ResourcePackager PUBLIC
                           ${COCOA_LIBRARY}
    )
endif()

target_include_directories( ResourcePackager PRIVATE
                            cjson
                            ${PROJECT_SOURCE_DIR}/src
                            ${STB_INCLUDE_PATH}
)
target_compile_definitions( ResourcePackager PRIVATE
                            _CRT_SECURE_NO_WARNINGS
                            MERCURY_NO_VULKAN_INCLUDE
                            MERCURY_NO_PTHREAD_INCLUDE
                            DEFAULT_PROG_ARGS="${RESOURCE_PACKAGE_ARGS_JOINED}"
)
set_target_properties( ResourcePackager PROPERTIES
                       RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEPLOY_BASE_PATH}/tools
                       RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DEPLOY_BASE_PATH}/tools
                       VS_DEBUGGER_COMMAND_ARGUMENTS ${RESOURCE_PACKAGE_ARGS_JOINED}
)
add_custom_command( TARGET ResourcePackager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:Assimp,IMPORTED_LOCATION> $<TARGET_FILE_DIR:ResourcePackager>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:Fmod,IMPORTED_LOCATION> $<TARGET_FILE_DIR:ResourcePackager>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PROPERTY:FSBank,IMPORTED_LOCATION> $<TARGET_FILE_DIR:ResourcePackager>
)
if( WIN32 )
    target_compile_definitions( ResourcePackager PRIVATE
                                DEFAULT_PROG_ARGS=""
    )
elseif( APPLE )
    target_compile_definitions( ResourcePackager PRIVATE
                                DEFAULT_PROG_ARGS="${RESOURCE_PACKAGE_ARGS_JOINED}"
    )
endif()
