project(OceanSimulator)


SET(PROJECT_SOURCE ${PLATFORM_SOURCE} 
	${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SimluatorApp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Waves.cpp
	# ${CMAKE_CURRENT_SOURCE_DIR}/PassConstants.cpp
	)


SET(PROJECT_HEADER 
	# ${CMAKE_CURRENT_SOURCE_DIR}/Common.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Device.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/SimluatorApp.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Waves.hpp
	# ${CMAKE_CURRENT_SOURCE_DIR}/PassConstants.hpp
	# ${CMAKE_CURRENT_SOURCE_DIR}/Object
	)

add_executable(OceanSimulator WIN32 ${PROJECT_SOURCE} ${PROJECT_HEADER})

target_link_libraries(
    OceanSimulator
	d3d12
	dxgi
	d3dcompiler
    Nix
)