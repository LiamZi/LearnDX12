project( DX12Samples )

##if(WIN32)
##    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
##    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
##    set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
##    set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
##endif()

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(
	${SOLUTION_DIR}/Source/Utility
	${SOLUTION_DIR}/Source/ThirdPart
)

set( EXECUTABLE_TYPE )

if( WIN32 )
    if( ${WINDOW_SYSTEM} STREQUAL SDL2 )
        set( PLATFORM_SOURCE
            ${SOLUTION_DIR}/Source/Platform/Windows/SDL2/SDL2Main.cpp
        )
    elseif( ${WINDOW_SYSTEM} STREQUAL Win32 )
        set( PLATFORM_SOURCE 
            ${SOLUTION_DIR}/Source/Platform/Windows/WinMain/WinMain.cpp
            # ${SOLUTION_DIR}/Source/Platform/Windows/WinMain/WinMain.rc
        )
    endif()
    set( EXECUTABLE_TYPE WIN32 )
endif()

if( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" )
    set( PLATFORM_SOURCE ${SOLUTION_DIR}/Source/Platform/Linux/LinuxMain.cpp )
endif()

if( ANDROID )
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -E")
    add_subdirectory( Android )
    SET_PROPERTY(TARGET AndroidSample PROPERTY FOLDER "DX12Samples")
else()
    if( WIN32 )
	
		add_definitions(-DUNICODE -D_UNICODE)
		# add_subdirectory( DX12Wrapper )
        # add_subdirectory( DX12Test )
		# add_subdirectory( DX12Triangle )
        # add_subdirectory( DX12TriangleColor )
        # add_subdirectory( DX12TriangleDelux )
        add_subdirectory( DX12WaterWave )
        # add_subdirectory( OceanFFT )
        add_subdirectory( Shapes )
		
		# SET_PROPERTY(TARGET DX12Wrapper PROPERTY FOLDER "Samples")
		# SET_PROPERTY(TARGET TriangleDelux PROPERTY FOLDER "DX12Samples")
        # SET_PROPERTY(TARGET HelloDX12 PROPERTY FOLDER "DX12Samples")
        # SET_PROPERTY(TARGET HelloTriangle PROPERTY FOLDER "DX12Samples")
        # SET_PROPERTY(TARGET HelloTriangleColor PROPERTY FOLDER "DX12Samples")
        SET_PROPERTY(TARGET WaterWave PROPERTY FOLDER "DX12WaterWave")
        # SET_PROPERTY(TARGET OceanFFT PROPERTY FOLDER "OceanFFT")
        SET_PROPERTY(TARGET Shapes PROPERTY FOLDER "Shapes")
        
    endif()
endif()

if(  CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" )
    add_subdirectory( ClearScreen )
endif()