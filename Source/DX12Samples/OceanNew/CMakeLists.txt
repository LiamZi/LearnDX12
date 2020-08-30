project(OceanNew)


SET(PROJECT_SOURCE ${PLATFORM_SOURCE} 
	# ${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/OceanNew.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Waves.cpp
    )

SET(PROJECT_HEADER
    # ${CMAKE_CURRENT_SOURCE_DIR}/Device.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OceanNew.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Waves.hpp
    )

add_executable(OceanNew WIN32 ${PROJECT_SOURCE} ${PROJECT_HEADER})

target_link_libraries(
    OceanNew
	d3d12
	dxgi
	d3dcompiler
    Nix
)