# Sn Compiler - CMake Build System
# Cross-platform build for Linux (GCC/Clang) and Windows (Clang/MinGW)

cmake_minimum_required(VERSION 3.16)
project(sn C)

# Use C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Options
option(SN_DEBUG "Build with debug symbols and sanitizers" OFF)
option(SN_ASAN "Enable AddressSanitizer (GCC/Clang only)" OFF)

# Compiler flags (GCC/Clang on all platforms)
add_compile_options(-Wall -Wextra)
# Suppress warnings for valid C99 patterns that trigger C11 warnings
add_compile_options(-Wno-typedef-redefinition)
# Suppress unknown pragma warnings (Windows-specific pragmas)
add_compile_options(-Wno-unknown-pragmas)
add_compile_definitions(_GNU_SOURCE)

if(SN_DEBUG)
    add_compile_options(-g -O0)
    if(SN_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
else()
    add_compile_options(-O3 -flto)
    add_link_options(-flto -O3)
endif()

# Collect source files
set(SN_CORE_SOURCES
    src/arena.c
    src/file.c
    src/token.c
    src/debug.c
    src/diagnostic.c
    src/compiler.c
)

set(SN_LEXER_SOURCES
    src/lexer.c
    src/lexer/lexer_util.c
    src/lexer/lexer_scan.c
)

set(SN_AST_SOURCES
    src/ast.c
    src/ast/ast_type.c
    src/ast/ast_expr.c
    src/ast/ast_stmt.c
    src/ast/ast_print.c
)

set(SN_PARSER_SOURCES
    src/parser.c
    src/parser/parser_util.c
    src/parser/parser_expr.c
    src/parser/parser_stmt.c
    src/parser/parser_stmt_decl.c
    src/parser/parser_stmt_control.c
)

set(SN_SYMBOL_TABLE_SOURCES
    src/symbol_table.c
    src/symbol_table/symbol_table_core.c
    src/symbol_table/symbol_table_namespace.c
    src/symbol_table/symbol_table_thread.c
)

set(SN_TYPE_CHECKER_SOURCES
    src/type_checker.c
    src/type_checker/type_checker_util.c
    src/type_checker/type_checker_expr.c
    src/type_checker/type_checker_expr_call.c
    src/type_checker/type_checker_expr_call_core.c
    src/type_checker/type_checker_expr_call_array.c
    src/type_checker/type_checker_expr_call_string.c
    src/type_checker/type_checker_expr_call_file.c
    src/type_checker/type_checker_expr_call_time.c
    src/type_checker/type_checker_expr_call_net.c
    src/type_checker/type_checker_expr_call_random.c
    src/type_checker/type_checker_expr_array.c
    src/type_checker/type_checker_expr_lambda.c
    src/type_checker/type_checker_stmt.c
)

set(SN_OPTIMIZER_SOURCES
    src/optimizer.c
    src/optimizer/optimizer_util.c
    src/optimizer/optimizer_tail_call.c
    src/optimizer/optimizer_string.c
)

set(SN_CODEGEN_SOURCES
    src/code_gen.c
    src/code_gen/code_gen_util.c
    src/code_gen/code_gen_expr.c
    src/code_gen/code_gen_expr_core.c
    src/code_gen/code_gen_expr_binary.c
    src/code_gen/code_gen_expr_lambda.c
    src/code_gen/code_gen_expr_call.c
    src/code_gen/code_gen_expr_call_array.c
    src/code_gen/code_gen_expr_call_file.c
    src/code_gen/code_gen_expr_call_string.c
    src/code_gen/code_gen_expr_call_time.c
    src/code_gen/code_gen_expr_call_random.c
    src/code_gen/code_gen_expr_call_uuid.c
    src/code_gen/code_gen_expr_array.c
    src/code_gen/code_gen_expr_static.c
    src/code_gen/code_gen_expr_string.c
    src/code_gen/code_gen_expr_thread.c
    src/code_gen/code_gen_stmt_core.c
    src/code_gen/code_gen_stmt_loop.c
    src/code_gen/code_gen_stmt_capture.c
)

set(SN_BACKEND_SOURCES
    src/gcc_backend.c
)

set(SN_RUNTIME_SOURCES
    src/runtime.c
    src/runtime/runtime_arena.c
    src/runtime/runtime_string.c
    src/runtime/runtime_array.c
    src/runtime/runtime_text_file.c
    src/runtime/runtime_binary_file.c
    src/runtime/runtime_io.c
    src/runtime/runtime_byte.c
    src/runtime/runtime_path.c
    src/runtime/runtime_date.c
    src/runtime/runtime_time.c
    src/runtime/runtime_thread.c
    src/runtime/runtime_process.c
    src/runtime/runtime_net.c
    src/runtime/runtime_random_core.c
    src/runtime/runtime_random_basic.c
    src/runtime/runtime_random_static.c
    src/runtime/runtime_random_choice.c
    src/runtime/runtime_random_collection.c
    src/runtime/runtime_random.c
    src/runtime/runtime_uuid.c
    src/runtime/runtime_sha1.c
    src/runtime/runtime_env.c
)

# All compiler sources (excluding main.c for library usage)
set(SN_COMPILER_SOURCES
    ${SN_CORE_SOURCES}
    ${SN_LEXER_SOURCES}
    ${SN_AST_SOURCES}
    ${SN_PARSER_SOURCES}
    ${SN_SYMBOL_TABLE_SOURCES}
    ${SN_TYPE_CHECKER_SOURCES}
    ${SN_OPTIMIZER_SOURCES}
    ${SN_CODEGEN_SOURCES}
    ${SN_BACKEND_SOURCES}
    ${SN_RUNTIME_SOURCES}
)

# Main compiler executable
add_executable(sn
    src/main.c
    ${SN_COMPILER_SOURCES}
)

target_include_directories(sn PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/runtime
    ${CMAKE_SOURCE_DIR}/src/platform
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(sn PRIVATE ws2_32 bcrypt pthread)
else()
    target_link_libraries(sn PRIVATE pthread m)
endif()

# Unit test executable
add_executable(tests
    tests/unit/_all_.c
    ${SN_COMPILER_SOURCES}
)

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/runtime
    ${CMAKE_SOURCE_DIR}/src/platform
)

if(WIN32)
    target_link_libraries(tests PRIVATE ws2_32 bcrypt pthread)
else()
    target_link_libraries(tests PRIVATE pthread m)
endif()

# Create output directories
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/bin/include)
set(RUNTIME_INCLUDE_DIR ${INCLUDE_DIR}/runtime)

# Create include directory structure
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
file(MAKE_DIRECTORY ${INCLUDE_DIR})
file(MAKE_DIRECTORY ${RUNTIME_INCLUDE_DIR})

# Copy config files to bin directory
# Use file(INSTALL) which handles existing files gracefully and works cross-platform
file(GLOB CONFIG_FILES "${CMAKE_SOURCE_DIR}/etc/*.cfg")
if(CONFIG_FILES)
    file(INSTALL ${CONFIG_FILES} DESTINATION "${CMAKE_SOURCE_DIR}/bin")
endif()

# Copy runtime headers
if(EXISTS "${CMAKE_SOURCE_DIR}/src/runtime.h")
    file(INSTALL "${CMAKE_SOURCE_DIR}/src/runtime.h" DESTINATION "${INCLUDE_DIR}")
endif()

file(GLOB RUNTIME_HEADERS "${CMAKE_SOURCE_DIR}/src/runtime/*.h")
if(RUNTIME_HEADERS)
    file(INSTALL ${RUNTIME_HEADERS} DESTINATION "${RUNTIME_INCLUDE_DIR}")
endif()

# Copy platform compatibility headers (needed for generated code on Windows)
# These go in bin/include/platform/ to match the source directory structure
# (runtime headers reference ../platform/compat_pthread.h etc.)
set(PLATFORM_INCLUDE_DIR ${INCLUDE_DIR}/platform)
file(MAKE_DIRECTORY ${PLATFORM_INCLUDE_DIR})
file(GLOB PLATFORM_HEADERS "${CMAKE_SOURCE_DIR}/src/platform/*.h")
if(PLATFORM_HEADERS)
    file(INSTALL ${PLATFORM_HEADERS} DESTINATION "${PLATFORM_INCLUDE_DIR}")
endif()

# Build runtime object files for linking with generated code
# On Windows with Clang/MinGW, put in bin/lib/clang/
# The gcc_backend expects individual .o files
if(WIN32 AND NOT MSVC)
    set(RUNTIME_LIB_DIR ${CMAKE_SOURCE_DIR}/bin/lib/clang)
    file(MAKE_DIRECTORY ${RUNTIME_LIB_DIR})

    # Common include directories for runtime compilation
    set(RUNTIME_INCLUDES
        -I${CMAKE_SOURCE_DIR}/src
        -I${CMAKE_SOURCE_DIR}/src/runtime
        -I${CMAKE_SOURCE_DIR}/src/platform
    )

    # Compile each runtime source file directly to the target location
    # This avoids issues with different generators (Ninja vs Make) using different paths
    set(RUNTIME_OBJECTS)

    # Helper macro to add a runtime object compilation
    macro(add_runtime_object SOURCE_FILE OUTPUT_NAME)
        set(OUTPUT_FILE ${RUNTIME_LIB_DIR}/${OUTPUT_NAME}.o)
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CMAKE_C_COMPILER} -c ${SOURCE_FILE} -o ${OUTPUT_FILE} ${RUNTIME_INCLUDES}
            DEPENDS ${SOURCE_FILE}
            COMMENT "Compiling ${OUTPUT_NAME}.o"
        )
        list(APPEND RUNTIME_OBJECTS ${OUTPUT_FILE})
    endmacro()

    # Add all runtime objects
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/arena.c arena)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/debug.c debug)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime.c runtime)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_arena.c runtime_arena)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_string.c runtime_string)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_array.c runtime_array)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_text_file.c runtime_text_file)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_binary_file.c runtime_binary_file)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_io.c runtime_io)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_byte.c runtime_byte)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_path.c runtime_path)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_date.c runtime_date)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_time.c runtime_time)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_thread.c runtime_thread)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_process.c runtime_process)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_net.c runtime_net)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_core.c runtime_random_core)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_basic.c runtime_random_basic)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_static.c runtime_random_static)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_choice.c runtime_random_choice)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_collection.c runtime_random_collection)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_random.c runtime_random)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_uuid.c runtime_uuid)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_sha1.c runtime_sha1)
    add_runtime_object(${CMAKE_SOURCE_DIR}/src/runtime/runtime_env.c runtime_env)

    # Custom target that depends on all runtime objects
    add_custom_target(sn_runtime ALL DEPENDS ${RUNTIME_OBJECTS})
endif()

# Custom target to run tests
add_custom_target(run-tests
    COMMAND $<TARGET_FILE:tests>
    DEPENDS tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running unit tests"
)

# Print configuration summary
message(STATUS "")
message(STATUS "Sn Compiler Build Configuration:")
message(STATUS "  Platform:     ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:     ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Debug:        ${SN_DEBUG}")
message(STATUS "  ASAN:         ${SN_ASAN}")
message(STATUS "  Output:       ${CMAKE_SOURCE_DIR}/bin")
message(STATUS "")
