file(GLOB FILE_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB FILE_HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

add_library(NIFToolset.Native ${FILE_SOURCES} ${FILE_HEADERS})

set_target_properties(NIFToolset.Native PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	OUTPUT_NAME "NIFToolset.Native"
)

target_include_directories(NIFToolset.Native
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)

target_compile_definitions(NIFToolset.Native
	PRIVATE
		NIFTOOLSET_NATIVE_EXPORT
		$<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:EE_EFD_NO_IMPORT>
	INTERFACE
		NIFTOOLSET_NATIVE_IMPORT
)

target_link_libraries(NIFToolset.Native
	PUBLIC
		efd
		NiMain
		NiCollision
		NiMesh
		NiAnimation
		NiParticle
		NiPortal
		ecrD3D11Renderer
)

# Keep install behavior explicit for the bridge module.
ni_install_target(NIFToolset.Native)
