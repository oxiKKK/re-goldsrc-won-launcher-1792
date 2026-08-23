# =============================================================================
# unzip531 -- vendored Info-ZIP UnZip 5.31 (static library).
#
# Standalone third-party subproject: NOT part of the launcher's own code.  It is
# pulled in by the top-level build via `add_subdirectory(external/unzip)` and
# consumed through `target_link_libraries(hl ... unzip531)`; its PUBLIC include
# directories propagate to consumers automatically.
#
# Built WIN32 + DLL so the public entry point is UzpMain().  This is 1997 K&R C
# compiled with /TC; all warnings are suppressed on this target only.  Local
# modifications to the upstream sources are documented in PATCHES.md.
# =============================================================================

file(GLOB UNZIP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/*.c")

add_library(unzip531 STATIC ${UNZIP_SOURCES})

set_target_properties(unzip531 PROPERTIES
    OUTPUT_NAME "unzip531"
    MSVC_RUNTIME_LIBRARY "${HL_MSVC_RUNTIME_LIBRARY}")  # match hl -- /MD,/MDd or /MT,/MTd per LAUNCHER_STATIC

target_include_directories(unzip531 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/win32")

target_compile_definitions(unzip531 PRIVATE
    WIN32 DLL                              # DLL => MAIN==UzpMain + REENTRANT
    _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    # NB: ASM_CRC is intentionally left *undefined* -- crc32.c gates the portable
    # C crc32() body on `#ifndef ASM_CRC`, so even `ASM_CRC=0` would drop it.

# Compile every file in this target as C (/TC) -- the *.h-only files are ignored.
target_compile_options(unzip531 PRIVATE /TC /w /WX-)
# Strip any inherited /WX (warnings-as-errors) just in case.
set_target_properties(unzip531 PROPERTIES COMPILE_WARNING_AS_ERROR OFF)
