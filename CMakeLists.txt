cmake_minimum_required (VERSION 3.21)

project ("learn-alembic" VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_UNITY_BUILD ON)

# 3rdparty include
include_directories (
	${CMAKE_SOURCE_DIR}/3rdparty/glad-0.1.35/include
	${CMAKE_SOURCE_DIR}/3rdparty/glfw-3.3.6/include
	${CMAKE_SOURCE_DIR}/3rdparty/Alembic/include
	${CMAKE_SOURCE_DIR}/3rdparty/Imath/include/Imath
)

# 3rdparty libs
link_directories(
	${CMAKE_SOURCE_DIR}/3rdparty/glad-0.1.35/lib
	${CMAKE_SOURCE_DIR}/3rdparty/glfw-3.3.6/lib
	${CMAKE_SOURCE_DIR}/3rdparty/Alembic/lib
	${CMAKE_SOURCE_DIR}/3rdparty/Imath/lib
)

# 3rdparty link
link_libraries(glad glfw3 Alembic Imath-3_1_d)

# Projects
include_directories(
	 ${CMAKE_SOURCE_DIR}/include
)

set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (EXISTS ${CMAKE_SOURCE_DIR}/data)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
								COMMAND ${CMAKE_COMMAND} -E copy_directory
								${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/data
	)
endif()
