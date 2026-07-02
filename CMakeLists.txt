cmake_minimum_required(VERSION 3.25)

project(esupath CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
  add_compile_options(-D_UNICODE -D_CRT_SECURE_NO_WARNINGS -W4)

  add_compile_options(-Zc:wchar_t -Zc:forScope)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.0.24215.1")
    add_compile_options(-utf-8)
    add_compile_options(-Zc:rvalueCast)
  endif()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.14.26428.1")
    add_compile_options(-Zc:__cplusplus)
  endif()
endif()

add_executable(${PROJECT_NAME}
  "${CMAKE_SOURCE_DIR}/src/esupath.cpp"
  "${CMAKE_SOURCE_DIR}/src/cmd_line_args.hpp"
)

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION "."
)
install(FILES "${CMAKE_SOURCE_DIR}/README.md"
  DESTINATION "."
  RENAME "esupath.md"
)
install(FILES
  "${CMAKE_SOURCE_DIR}/src/batch/besupath.bat"
  "${CMAKE_SOURCE_DIR}/src/batch/coreutilsPrio.bat"
  DESTINATION "."
)

include(CTest)

if(BUILD_TESTING AND WIN32)
  add_test(
    NAME esupath-env
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -Command
      "& '${CMAKE_SOURCE_DIR}/test/test.bat' env '$<TARGET_FILE:${PROJECT_NAME}>'; exit $LASTEXITCODE"
  )
  set_tests_properties(esupath-env PROPERTIES
    LABELS "env"
    TIMEOUT 60
  )

  add_test(
    NAME esupath-user
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -Command
      "& '${CMAKE_SOURCE_DIR}/test/test.bat' user '$<TARGET_FILE:${PROJECT_NAME}>'; exit $LASTEXITCODE"
  )
  set_tests_properties(esupath-user PROPERTIES
    LABELS "user;registry"
    RUN_SERIAL TRUE
    TIMEOUT 60
  )
endif()

set(ESUPATH_PACKAGE_TAG "" CACHE STRING "Suffix used for the package file name")
if(ESUPATH_PACKAGE_TAG)
  set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${ESUPATH_PACKAGE_TAG}")
else()
  set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}")
endif()
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/dist")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
