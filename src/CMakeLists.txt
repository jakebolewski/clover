include_directories (${Coal_SOURCE_DIR}/include
                    ${Coal_SOURCE_DIR}/src
                    ${LLVM_INCLUDE_DIR}
                    ${GALLIUM_INCLUDE_DIRS}
                    ${CLANG_INCLUDE_DIRS}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

configure_file(core/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/core/config.h)

set(COAL_SRC_FILES
    api/api_command.cpp
    api/api_device.cpp
    api/api_event.cpp
    api/api_kernel.cpp
    api/api_platform.cpp
    api/api_program.cpp
    api/api_context.cpp
    api/api_enqueue.cpp
    api/api_flush.cpp
    api/api_memory.cpp
    api/api_profiling.cpp
    api/api_sampler.cpp
    api/api_gl.cpp

    core/context.cpp
    core/commandqueue.cpp
    core/memobject.cpp
    core/events.cpp
    core/program.cpp
    core/compiler.cpp
    core/kernel.cpp
    core/sampler.cpp
    core/object.cpp

    core/cpu/buffer.cpp
    core/cpu/device.cpp
    core/cpu/kernel.cpp
    core/cpu/program.cpp
    core/cpu/worker.cpp
    core/cpu/builtins.cpp
    core/cpu/sampler.cpp

    ${CMAKE_CURRENT_BINARY_DIR}/runtime/stdlib.h.embed.h
    ${CMAKE_CURRENT_BINARY_DIR}/runtime/stdlib.c.bc.embed.h
)

add_subdirectory(runtime)

add_library(OpenCL SHARED ${COAL_SRC_FILES})

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/runtime/stdlib.h.embed.h
                            PROPERTIES GENERATED 1)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/runtime/stdlib.c.bc.embed.h
                            PROPERTIES GENERATED 1)
add_dependencies(OpenCL generate_stdlib_h)
add_dependencies(OpenCL generate_stdlib_c)

SET(LIBRARY_OUTPUT_PATH ${Coal_BINARY_DIR}/lib)

SET_TARGET_PROPERTIES(OpenCL PROPERTIES
  VERSION ${${PROJECT_NAME}_VERSION}
  SOVERSION ${${PROJECT_NAME}_SOVERSION}
)

set_source_files_properties(${COAL_SRC_FILES}
        PROPERTIES COMPILE_FLAGS ${LLVM_COMPILE_FLAGS})

set_target_properties(OpenCL PROPERTIES
  LINK_FLAGS ${LLVM_LDFLAGS}
  LINK_INTERFACE_LIBRARIES "")

TARGET_LINK_LIBRARIES(OpenCL
        ${CLANG_LIBS}
        ${LLVM_LIBS_CORE}
        ${LLVM_LIBS_JIT}
        pthread
        rt
#        ${GALLIUM_LIBS}
)

install(TARGETS OpenCL LIBRARY DESTINATION /usr/lib)
