project(TriangleDelux)


SET(PROJECT_SOURCE ${PLATFORM_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/TriangleDelux.cpp)
SET(PROJECT_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/TriangleDelux.h)

add_executable(TriangleDelux WIN32 ${PROJECT_SOURCE} ${PROJECT_HEADER})

target_link_libraries(
    TriangleDelux
	d3d12
	dxgi
	d3dcompiler
    Nix
)