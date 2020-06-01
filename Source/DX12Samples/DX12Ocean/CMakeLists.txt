project(OceanSimulator)


SET(PROJECT_SOURCE ${PLATFORM_SOURCE} 
	${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SimluatorApp.cpp
	)


SET(PROJECT_HEADER 
	# ${CMAKE_CURRENT_SOURCE_DIR}/Common.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Device.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/SimluatorApp.hpp
	)

add_executable(OceanSimulator WIN32 ${PROJECT_SOURCE} ${PROJECT_HEADER})

target_link_libraries(
    OceanSimulator
	d3d12
	dxgi
	d3dcompiler
    Nix
)