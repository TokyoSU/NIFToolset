file(GLOB_RECURSE FILE_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE FILE_HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

find_package(assimp CONFIG REQUIRED)
add_executable(AssimpExporter ${FILE_SOURCES} ${FILE_HEADERS})

target_include_directories(AssimpExporter
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(AssimpExporter
	PRIVATE
		NiMain
		NiMesh
		NiAnimation
		NiParticle
		NiPortal
		assimp::assimp
		windowscodecs
		ole32
)

set_target_properties(AssimpExporter PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
)

ni_install_target(AssimpExporter)
