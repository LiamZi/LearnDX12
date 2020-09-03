project(Shapes)


SET(PROJECT_SOURCE ${PLATFORM_SOURCE} 
	${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Shapes.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/GeometryGenerator.cpp
    )

SET(PROJECT_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/Device.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Shapes.hpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/GeometryGenerator.hpp
    )

add_executable(Shapes WIN32 ${PROJECT_SOURCE} ${PROJECT_HEADER})

target_link_libraries(
    Shapes
	d3d12
	dxgi
	d3dcompiler
    Nix
)