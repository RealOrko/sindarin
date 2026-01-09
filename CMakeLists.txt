# Sn Compiler - CMake Build System
# Cross-platform build for Linux (GCC/Clang) and Windows (MSVC)

cmake_minimum_required(VERSION 3.16)
project(sn C)

# Use C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# For multi-config generators (MSVC), put binaries directly in bin/
# Set output directories for all standard configuration types
foreach(CONFIG_TYPE IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_SOURCE_DIR}/bin)
endforeach()

# Options
option(SN_DEBUG "Build with debug symbols and sanitizers" OFF)
option(SN_ASAN "Enable AddressSanitizer (GCC/Clang only)" OFF)

# Platform-specific settings
if(MSVC)
    # MSVC compiler flags
    add_compile_options(/W3)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

    if(SN_DEBUG)
        add_compile_options(/Zi /Od /RTC1)
    else()
        add_compile_options(/O2)
    endif()
else()
    # GCC/Clang compiler flags
    add_compile_options(-Wall -Wextra)
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
if(MSVC)
    # Windows libraries
    target_link_libraries(sn PRIVATE ws2_32 bcrypt)
else()
    # Unix libraries
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

if(MSVC)
    target_link_libraries(tests PRIVATE ws2_32 bcrypt)
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

# Build runtime as separate object files for linking with generated code
# These go in bin/lib/msvc/ to match the Linux structure (bin/lib/gcc/, bin/lib/clang/)
if(MSVC)
    set(RUNTIME_LIB_DIR ${CMAKE_SOURCE_DIR}/bin/lib/msvc)
    file(MAKE_DIRECTORY ${RUNTIME_LIB_DIR})

    # Runtime-only sources (subset needed for generated code)
    set(SN_RUNTIME_LIB_SOURCES
        ${CMAKE_SOURCE_DIR}/src/arena.c
        ${CMAKE_SOURCE_DIR}/src/debug.c
        ${CMAKE_SOURCE_DIR}/src/runtime.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_arena.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_string.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_array.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_text_file.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_binary_file.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_io.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_byte.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_path.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_date.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_time.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_thread.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_process.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_net.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_core.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_basic.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_static.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_choice.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_random_collection.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_random.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_uuid.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_sha1.c
        ${CMAKE_SOURCE_DIR}/src/runtime/runtime_env.c
    )

    # Create static library for runtime
    add_library(sn_runtime STATIC ${SN_RUNTIME_LIB_SOURCES})

    target_include_directories(sn_runtime PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/runtime
        ${CMAKE_SOURCE_DIR}/src/platform
    )

    target_compile_definitions(sn_runtime PRIVATE _CRT_SECURE_NO_WARNINGS)

    # Output the library to bin/lib/msvc/
    set_target_properties(sn_runtime PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${RUNTIME_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${RUNTIME_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${RUNTIME_LIB_DIR}
        OUTPUT_NAME "sn_runtime"
    )

    # Also create individual object files for compatibility with the gcc/clang approach
    # This is done via a custom command after the library is built
    add_custom_command(TARGET sn_runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Runtime library built in ${RUNTIME_LIB_DIR}"
    )
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
