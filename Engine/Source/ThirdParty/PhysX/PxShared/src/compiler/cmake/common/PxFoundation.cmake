#
# Build PxFoundation common
#


ADD_LIBRARY(PxFoundation ${PXFOUNDATION_LIBTYPE} 
	${LL_SOURCE_DIR}/src/PsAllocator.cpp
	${LL_SOURCE_DIR}/src/PsAssert.cpp
	${LL_SOURCE_DIR}/src/PsFoundation.cpp
	${LL_SOURCE_DIR}/src/PsMathUtils.cpp
	${LL_SOURCE_DIR}/src/PsString.cpp
	${LL_SOURCE_DIR}/src/PsTempAllocator.cpp
	${LL_SOURCE_DIR}/src/PsUtilities.cpp
	
	${PXFOUNDATION_PLATFORM_FILES}
)

TARGET_INCLUDE_DIRECTORIES(PxFoundation 
	PRIVATE ${PXSHARED_SOURCE_DIR}/../include
	PRIVATE ${LL_SOURCE_DIR}/include
	
	PRIVATE ${PXFOUNDATION_PLATFORM_INCLUDES}
)

TARGET_COMPILE_DEFINITIONS(PxFoundation 
	PRIVATE ${PXFOUNDATION_COMPILE_DEFS}
)

SET_TARGET_PROPERTIES(PxFoundation PROPERTIES 
	COMPILE_PDB_NAME_DEBUG "PxFoundation${CMAKE_DEBUG_POSTFIX}"
	COMPILE_PDB_NAME_CHECKED "PxFoundation${CMAKE_CHECKED_POSTFIX}"
	COMPILE_PDB_NAME_PROFILE "PxFoundation${CMAKE_PROFILE_POSTFIX}"
	COMPILE_PDB_NAME_RELEASE "PxFoundation${CMAKE_RELEASE_POSTFIX}"
)