# TODO: exclude copy_shaders.cmake
FILE(GLOB_RECURSE shader_files RELATIVE "${SRCDIR}/" "${SRCDIR}/*")

MESSAGE(STATUS ${shader_files})

FOREACH(file IN LISTS shader_files)
    CONFIGURE_FILE(${SRCDIR}/${file} ${DSTDIR}/${file} COPYONLY)
ENDFOREACH()
