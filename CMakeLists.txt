
# FSBank
add_library( FSBank SHARED IMPORTED )
set_target_properties( FSBank PROPERTIES
                       IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/ots/fmod/lib/x64/fsbank.dll
                       IMPORTED_IMPLIB ${PROJECT_SOURCE_DIR}/ots/fmod/lib/x64/fsbank_vc.lib )
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
)
source_group( src FILES ${SRC_FILES} )

list( JOIN RESOURCE_PACKAGE_ARGS " " RESOURCE_PACKAGE_ARGS_JOINED )

add_executable( ResourcePackager EXCLUDE_FROM_ALL
                CMakeLists.txt
                ${CJSON_FILES}
                ${SRC_FILES}
)
target_link_libraries( ResourcePackager PUBLIC
                       Assimp
                       FSBank
)
target_include_directories( ResourcePackager PRIVATE
                            cjson
                            ${STB_INCLUDE_PATH}
)
target_compile_definitions( ResourcePackager PRIVATE
                            _CRT_SECURE_NO_WARNINGS
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