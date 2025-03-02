file(GLOB_RECURSE overengine_source_files "src/OverEngine/*.cpp")
file(GLOB_RECURSE overengine_header_files "src/OverEngine/*.h")

set(OE_CROSS_PLATFORM_FILES
	"src/OverEngine.h"
	"src/pcheader.cpp"
	"src/pcheader.h"
	
	${overengine_source_files}
	${overengine_header_files}

	"vendor/stb_image/stb_image.h"
	"vendor/stb_image/stb_image.cpp"

	"vendor/stb_rectpack/stb_rectpack.h"
	"vendor/stb_rectpack/stb_rectpack.cpp"

	"vendor/tinyfiledialogs/tinyfiledialogs.h"
	"vendor/tinyfiledialogs/tinyfiledialogsBuild.cpp"
)

file(GLOB_RECURSE opengl_platform_source_files "src/Platform/OpenGL/*.cpp")
file(GLOB_RECURSE opengl_platform_header_files "src/Platform/OpenGL/*.h")

file(GLOB_RECURSE win32_platform_source_files "src/Platform/Windows/*.cpp")
file(GLOB_RECURSE win32_platform_header_files "src/Platform/Windows/*.h")

file(GLOB_RECURSE linux_platform_source_files "src/Platform/Linux/*.cpp")
file(GLOB_RECURSE linux_platform_header_files "src/Platform/Linux/*.h")

if(WIN32)
	set(OE_OS_FILES ${win32_platform_source_files} ${win32_platform_header_files})
elseif("${CMAKE_SYSTEM}" MATCHES "Linux")
	set(OE_OS_FILES ${linux_platform_source_files} ${linux_platform_header_files})
endif()

add_library(OverEngine STATIC ${OE_CROSS_PLATFORM_FILES} ${OE_OS_FILES}
	${opengl_platform_source_files}
	${opengl_platform_header_files}
)

if (OE_ENABLE_UNITY_BUILD)
	# enable unity build and precompiled header for ultra fast build
	set_target_properties(OverEngine PROPERTIES UNITY_BUILD ON)
	set_target_properties(OverEngine PROPERTIES UNITY_BUILD_BATCH_SIZE 20)
endif()

target_precompile_headers(OverEngine PUBLIC "src/pcheader.h")

target_include_directories(OverEngine PRIVATE
	"res"
	"vendor/GLFW/include"
	"vendor/glad/include"
	"vendor/stb_image"
	"vendor/stb_rectpack"
)

target_include_directories(OverEngine PUBLIC
	"src"
	"vendor"
	"vendor/spdlog/include"
	"vendor/imgui"
	"vendor/glm"
	"vendor/lua/src"
	"vendor/entt/include"
	"vendor/box2d/include"
	"vendor/json/include"
	"vendor/fmt/include"
	"vendor/yaml-cpp/include"
)

target_compile_definitions(OverEngine PRIVATE
	"_CRT_SECURE_NO_WARNINGS"
	"GLFW_INCLUDE_NONE"
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions("OE_DEBUG")
endif()
	
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_definitions("OE_RELEASE")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	add_compile_definitions("OE_DEBUG")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
	add_compile_definitions("OE_RELEASE")
endif()

target_link_libraries(OverEngine PRIVATE glfw glad)
target_link_libraries(OverEngine PUBLIC imgui box2d yaml-cpp)
