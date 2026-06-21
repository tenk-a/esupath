cmake_minimum_required(VERSION 3.24)

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
)

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin"
)
