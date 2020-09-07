project(OceanFFT)


SET(PROJECT_SOURCE ${PLATFORM_SOURCE} 
	${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/OceanFFT.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Waves.cpp
    )

SET(PROJECT_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/Device.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OceanFFT.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Waves.hpp
    )

add_executable(OceanFFT WIN32 ${PROJECT_SOURCE} ${PROJECT_HEADER})

target_link_libraries(
    OceanFFT
	d3d12
	dxgi
	d3dcompiler
    Nix
)