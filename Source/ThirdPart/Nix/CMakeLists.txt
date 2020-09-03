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
	${CMAKE_CURRENT_SOURCE_DIR}/Memory/BuddySystemAllocator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Timer/Timer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Timer/Timer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/Utils.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/Utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utils/UploadBuffer.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Math/MathHelper.h
	${CMAKE_CURRENT_SOURCE_DIR}/Math/MathHelper.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/ConstantObject.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/ConstantObject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/FrameResource.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/FrameResource.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/GeometryGenerator.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/GeometryGenerator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/MeshGeometry.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/MeshGeometry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/PassConstants.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/PassConstants.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/RenderItem.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/RenderItem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Utility/Vertex.hpp
    )

add_library( Nix STATIC 
	${NIX_SOURCE}
)