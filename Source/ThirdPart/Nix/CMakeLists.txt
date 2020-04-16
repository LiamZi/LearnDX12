CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

include_directories( 
	${CMAKE_CURRENT_SOURCE_DIR}
)

project( Nix )

set( NIX_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/archive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/archive.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/String/encoding.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/String/path.cpp
	# ${CMAKE_CURRENT_SOURCE_DIR}/Memory/BuddySystemAllocator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Memory/BuddySystemAllocator.cpp
    )

add_library( Nix STATIC 
	${NIX_SOURCE}
)