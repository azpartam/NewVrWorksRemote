#
# Build LowLevelAABB
#

SET(GW_DEPS_ROOT $ENV{GW_DEPS_ROOT})
FIND_PACKAGE(PxShared REQUIRED)

SET(PHYSX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../)

SET(LL_SOURCE_DIR ${PHYSX_SOURCE_DIR}/LowLevelAABB/src)

FIND_PACKAGE(nvToolsExt REQUIRED)

SET(LOWLEVELAABB_PLATFORM_INCLUDES
	${NVTOOLSEXT_INCLUDE_DIRS}
	${PHYSX_SOURCE_DIR}/Common/src/tvos
	${PHYSX_SOURCE_DIR}/LowLevelAABB/tvos/include
	${PHYSX_SOURCE_DIR}/GpuBroadPhase/include
	${PHYSX_SOURCE_DIR}/GpuBroadPhase/src
)


SET(LOWLEVELAABB_COMPILE_DEFS

	# Common to all configurations
	${PHYSX_TVOS_COMPILE_DEFS};PX_PHYSX_STATIC_LIB

	$<$<CONFIG:debug>:${PHYSX_TVOS_DEBUG_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=DEBUG;>
	$<$<CONFIG:checked>:${PHYSX_TVOS_CHECKED_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=CHECKED;>
	$<$<CONFIG:profile>:${PHYSX_TVOS_PROFILE_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=PROFILE;>
	$<$<CONFIG:release>:${PHYSX_TVOS_RELEASE_COMPILE_DEFS};>
)

# include common low level AABB settings
INCLUDE(../common/LowLevelAABB.cmake)

# enable -fPIC so we can link static libs with the editor
SET_TARGET_PROPERTIES(LowLevelAABB PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
