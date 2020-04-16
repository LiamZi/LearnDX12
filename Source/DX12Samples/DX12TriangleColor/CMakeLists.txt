project(HelloTriangleColor)

SET(PROJECT_SOURCE ${PLATFORM_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/HelloTriangleColor.cpp)
SET(PROJECT_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/HelloTriangleColor.h)


add_executable(HelloTriangleColor WIN32 ${PROJECT_SOURCE} ${PROJECT_HEADER})

target_link_libraries(
    HelloTriangleColor
	d3d12
	dxgi
	d3dcompiler
    Nix
)