project(HelloTriangle)

SET(PROJECT_SOURCE ${PLATFORM_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/HelloTriangle.cpp)
SET(PROJECT_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/HelloTriangle.h)


add_executable(HelloTriangle WIN32 ${PROJECT_SOURCE} ${PROJECT_HEADER})


target_link_libraries(
    HelloTriangle
	d3d12
	dxgi
	d3dcompiler
    Nix
)