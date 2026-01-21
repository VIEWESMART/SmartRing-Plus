set(SRC_DIR src)

file(GLOB_RECURSE SRC_SOURCES ${SRC_DIR}/*.c)

idf_component_register(

    SRCS ${SRC_SOURCES}

    INCLUDE_DIRS "include"
    
    REQUIRES "driver")