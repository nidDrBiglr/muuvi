# Include header files
target_include_directories(app PRIVATE .)

# Include source files
FILE(GLOB app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
target_sources(app PRIVATE ${app_sources})
