# =============================================================================
# re-won-launcher-1792 -- standalone build for the WON-era Half-Life launcher
# (hl.exe, build 1792, Sep 20 2001) reconstruction.
#
# Targets, mirroring the original Visual Studio projects:
#   won_common / won_crypt / won_auth -- common.lib, crypt.lib, auth.lib [STATIC]
#   WONCrypt  -- the WON crypto DLL   (hlwon/TitanApi/crypt)       [SHARED]
#   WONAuth   -- the WON auth DLL     (hlwon/TitanApi/auth)        [SHARED]
#   WONDLL    -- the WON flat C API   (hlwon/TitanApi, per its Makefile) [SHARED]
#   cryptapi  -- the launcher's crypto/auth wrapper (hlwon/cryptapi) [STATIC]
#   hl        -- the launcher itself (MFC app)                    [WIN32 EXE]
#
# WONCrypt and WONAuth are separate DLLs because the 2001 binaries are: hl.exe,
# hw.dll, sw.dll, cstrike.exe and hltv.exe each import 15 symbols from one and 18
# from the other, by name.  See hlwon/TitanApi/CMakeLists.txt.
#
# 32-bit (Win32) only -- the whole reconstruction is x86.  Configure with the
# Visual Studio 2026 generator and the Win32 platform, e.g.:
#
#   cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
#   cmake --build build --config Debug
#
# See README.md for prerequisites (the MFC/MBCS toolset components).
# =============================================================================
cmake_minimum_required(VERSION 3.25)

project(re-won-launcher-1792 LANGUAGES C CXX RC)

if(NOT MSVC)
    message(FATAL_ERROR "This project builds only with the MSVC toolchain (Visual Studio).")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "This is a 32-bit (Win32) project. Re-configure with: -A Win32")
endif()

# Post-release fixes to the reconstructed launcher: behaviour that departs from
# the retail binary on purpose.  Every one of them sits behind LAUNCHER_FIXES so
# the original behaviour is a -DLAUNCHER_FIXES=OFF away.
option(LAUNCHER_FIXES "Enable the launcher's post-release fixes" ON)

# Fold the CRT, MFC and the WONCrypt/WONAuth libraries straight into hl.exe instead
# of importing them, so the one exe can be dropped into any WON-era Half-Life
# install without also deploying mfc140.dll, the VC++ redistributable's DLLs, and
# our own WONCrypt.dll/WONAuth.dll alongside it.  This is closer to the retail
# binary than the default, not further from it: hl_stock.exe's own import table has
# no MFC or CRT DLL at all -- see the WONCrypt/WONAuth target below and
# hlwon/TitanApi/CMakeLists.txt for how the two DLLs fold in.
#
# vgui.dll is the one dependency this cannot absorb: it is a prebuilt third-party
# binary (see launcher/vgui1/vgui.lib's comment) we only have an import lib for,
# not its source. Every WON-era game folder already ships one, so it is not a
# deployment burden the way the others are.
option(LAUNCHER_STATIC "Statically link the CRT, MFC and the WON libraries into hl.exe" ON)

# CMP0091: select the MSVC runtime library via MSVC_RUNTIME_LIBRARY (per target).
cmake_policy(SET CMP0091 NEW)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# hl's CRT linkage, and (see external/unzip/CMakeLists.txt) unzip531's -- they must
# match, or the linker rejects the mix with a RuntimeLibrary-mismatch error.
if(LAUNCHER_STATIC)
    set(HL_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")      # /MT, /MTd
else()
    set(HL_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")   # /MD, /MDd
endif()

# The original is a VC6 build, where time_t is 32 bits.  That is not cosmetic: the
# WON auth wire format embeds it directly -- AuthFamilyBuffer.h documents its fields
# as "Issue Date/Time (time_t - 4 bytes)" -- so a modern 64-bit time_t shifts every
# field after the timestamps and the genuine WONAuth.dll throws while parsing our
# public-key block (which is what aborts the vanilla launcher).  Applied project-wide,
# before add_subdirectory, so every target agrees on the layout.
add_compile_definitions(_USE_32BIT_TIME_T)

# The WON libraries and the two DLLs the launcher imports, laid out as the
# original Developer Studio projects were.
add_subdirectory(hlwon/TitanApi)

# -----------------------------------------------------------------------------
# cryptapi -- the launcher-facing crypto/auth layer over WONCrypt.
# (hlwon/cryptapi -- EasySocket / EasyTitanSocket / WriteBuffer / AuthRequest)
#
# The original hl.exe links this in as cryptapi.lib: the bodies sit in its own
# .text (EasySocket at 0x40c110-0x40d0d0, EasyTitanSocket at 0x40d0d0-0x40e470),
# and only WONAuth.dll / WONCrypt.dll are imports.  Compile these straight into
# the hl target the same way, so the launcher calls the classes directly and
# there is no static/shared CRT boundary between them.
# -----------------------------------------------------------------------------
file(GLOB CRYPTAPI_SOURCES CONFIGURE_DEPENDS "${REPO_ROOT}/hlwon/cryptapi/*.cpp")

# -----------------------------------------------------------------------------
# unzip531 -- vendored Info-ZIP UnZip 5.31, built as a standalone third-party
# subproject (see external/unzip/CMakeLists.txt).  Defines the `unzip531` target
# linked by `hl` below; its PUBLIC include dirs propagate to consumers.
# -----------------------------------------------------------------------------
add_subdirectory(external/unzip)

# -----------------------------------------------------------------------------
# wonserver -- WON server emulator subproject.  Builds the standalone `wonserverd`
# EXE only; nothing from it is linked into the launcher.  Added BEFORE CMAKE_MFC_FLAG
# so wonserverd stays non-MFC.
# -----------------------------------------------------------------------------
add_subdirectory(wonserver)

# -----------------------------------------------------------------------------
# hl -- the launcher (MFC application, MultiByte charset).
# CMAKE_MFC_FLAG must be set *here*, after the libraries above, so only this
# target links against MFC.  (1 = static MFC, 2 = MFC in a shared DLL.)
# -----------------------------------------------------------------------------
if(LAUNCHER_STATIC)
    set(CMAKE_MFC_FLAG 1)
else()
    set(CMAKE_MFC_FLAG 2)
endif()

# launcher/ -- the launcher's own code (UI/dialogs/controls + services) alongside
#              the standalone forks of GoldSrc engine foundation files
#              (sys/common/cd/snd/token/info).
file(GLOB LAUNCHER_SOURCES CONFIGURE_DEPENDS
    "${REPO_ROOT}/launcher/*.cpp"
    "${REPO_ROOT}/launcher/*.c"             # engine-lineage modules converted to C
    "${REPO_ROOT}/hlwon/won/*.cpp")   # WON directory / factory / message client

add_executable(hl WIN32
    ${LAUNCHER_SOURCES}
    ${CRYPTAPI_SOURCES}                     # cryptapi.lib, linked in as the original does
    "${REPO_ROOT}/launcher/launcher.rc"
    "${REPO_ROOT}/launcher/launcher_version.rc"
    "${REPO_ROOT}/shell/hl_res/hl_res.rc")
set_target_properties(hl PROPERTIES
    OUTPUT_NAME "hl"
    C_STANDARD 90                # ANSI C89 -- matches the 1998 QuakeWorld-lineage sources
    C_STANDARD_REQUIRED OFF      # MSVC has no /std:c89; fall back to its (lenient) default C mode
    MSVC_RUNTIME_LIBRARY "${HL_MSVC_RUNTIME_LIBRARY}")
target_include_directories(hl PRIVATE
    "${REPO_ROOT}/common"
    "${REPO_ROOT}/launcher"
    "${REPO_ROOT}/hlwon/won"
    "${REPO_ROOT}/hlwon/cryptapi"         # EasySocket / EasyTitanSocket / WriteBuffer / AuthRequest
    "${REPO_ROOT}/hlwon/TitanApi"
    "${REPO_ROOT}/hlwon/TitanApi/auth"
    "${REPO_ROOT}/hlwon/TitanApi/common"
    "${REPO_ROOT}/hlwon/TitanApi/crypt"
    "${REPO_ROOT}/hlwon"
    "${REPO_ROOT}/engine"
    "${REPO_ROOT}/external/dx6sdk/include"
    "${REPO_ROOT}/shell/hl_res"
    "${REPO_ROOT}/launcher/vgui1/include")
target_compile_definitions(hl PRIVATE
    WIN32 _MBCS _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
    _WIN32_WINNT=0x0501 WINVER=0x0501    # target Windows XP; silences the SDK's "_WIN32_WINNT not defined -> MAXVER" warning
    $<$<CONFIG:Debug>:_DEBUG LAUNCHER_RE>
    $<$<CONFIG:Release>:NDEBUG LAUNCHER_RE>
    $<$<BOOL:${LAUNCHER_FIXES}>:LAUNCHER_FIXES>
    # Consumer side of the WON_STATIC_LINK switch in WON_CryptFactory.h/WON_AuthFactory.h --
    # see hlwon/TitanApi/CMakeLists.txt for the producer side.  LAUNCHER_STATIC itself is
    # also exposed to the launcher's own code -- see CNetGameApp::LoadRes (launcher.cpp).
    $<$<BOOL:${LAUNCHER_STATIC}>:WON_STATIC_LINK LAUNCHER_STATIC>)

# The vendored WON sources use RTTI + C++ exceptions.
target_compile_options(hl PRIVATE /EHsc /GR)

# -----------------------------------------------------------------------------
# Precompiled header.  launcher/precompiled.h carries everything the launcher
# compiles against -- MFC/Win32/CRT, the WON and engine-lineage headers, and all
# of launcher/*.h -- and is the single #include in every launcher TU.
#
# The vendored subprojects compiled into this target -- hlwon/cryptapi and
# hlwon/won -- are excluded: they are non-MFC WON code and must keep compiling
# on their own terms.
# -----------------------------------------------------------------------------
target_precompile_headers(hl PRIVATE "${REPO_ROOT}/launcher/precompiled.h")
file(GLOB WON_SOURCES CONFIGURE_DEPENDS "${REPO_ROOT}/hlwon/won/*.cpp")
set_source_files_properties(${CRYPTAPI_SOURCES} ${WON_SOURCES}
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# The DirectX TUs opt out too.  Two reasons, both structural:
#   * ddraw/gameui/modes/vid_win define CINTERFACE before any Windows header to
#     get DirectDraw's C-style vtable interface -- a PCH has windows.h parsed by
#     then, so the define arrives too late.
#   * these five reach the DX6 SDK headers in external/dx6sdk, whose d3dtypes.h
#     redefines the structs in the Windows SDK's d3d9types.h -- which MFC's
#     afxwin.h drags in unconditionally (via afxrendertarget.h).  None of the
#     five includes afxwin.h itself, and they must keep it that way.
set_source_files_properties(
    "${REPO_ROOT}/launcher/ddraw.cpp"
    "${REPO_ROOT}/launcher/gameui.cpp"
    "${REPO_ROOT}/launcher/modes.cpp"
    "${REPO_ROOT}/launcher/snd_win.cpp"
    "${REPO_ROOT}/launcher/vid_win.cpp"
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# Resource compiler needs to find resource.h (shell/hl_res) and resource_dlg.h (launcher).
target_include_directories(hl PRIVATE "${REPO_ROOT}/shell/hl_res" "${REPO_ROOT}/launcher")

target_link_libraries(hl PRIVATE
    WONCrypt        # the WON DLL boundary in the original: two DLLs, imported by name.
    WONAuth         # LAUNCHER_STATIC makes these STATIC libraries instead -- see hlwon/TitanApi/CMakeLists.txt.
    unzip531
    "${REPO_ROOT}/launcher/vgui1/vgui.lib"
    winmm
    ws2_32
    ddraw
    dxguid)

# The 1786 software renderer self-modifies its span-drawing asm (VirtualProtect to
# PAGE_READWRITE, then executes it), which predates DEP.  Opt out of /NXCOMPAT so
# the patched page can run, matching the 2001 binary.
target_link_options(hl PRIVATE /NXCOMPAT:NO)

# The original 2001 hl.exe ships with NO application manifest, which is what lets
# the Windows AppCompat "HighDpiAware" shim adopt it (the shim is keyed on the
# Valve / Half-Life / product-version<=1 fields in our VERSIONINFO and is skipped
# for any manifest-bearing exe).  The modern MSVC/MFC toolchain otherwise embeds a
# default manifest with <dpiAware>true</dpiAware>; suppress it so DPI awareness is
# governed by the OS shim exactly as in the original.
target_link_options(hl PRIVATE /MANIFEST:NO)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT hl)

# -----------------------------------------------------------------------------
# Post-build: copy hl.exe (and, when not LAUNCHER_STATIC, WONCrypt.dll and
# WONAuth.dll) into the Half-Life game folder named in "halflife_path.txt" (see
# halflife_path.txt.example).  The config file is read at build time by
# cmake/copy_to_halflife.cmake, so changing the path does not require re-running
# CMake; a missing config file just skips the copy.
#
# WONDLL.dll always goes along too: it is not a dependency of hl.exe (nothing in
# this repo links it), but the same 2001 game folder is where hw.dll/sw.dll would
# look for it.
#
# This overwrites the game folder's WON DLLs with the ones built here.  Keep a
# backup of the pristine 2001 DLLs from your game install before the first run
# if you want to restore them later.
#
# Only a LAUNCHER_FIXES build deploys.  A -DLAUNCHER_FIXES=OFF tree is built to
# check that the original code path still compiles, and must never be the binary
# left sitting in the game folder.
# -----------------------------------------------------------------------------
if(LAUNCHER_FIXES)
    set(HL_DEPLOY_FILES "$<TARGET_FILE:hl>" "$<TARGET_FILE:WONDLL>")
    if(NOT LAUNCHER_STATIC)
        list(APPEND HL_DEPLOY_FILES "$<TARGET_FILE:WONCrypt>" "$<TARGET_FILE:WONAuth>")
    endif()
    list(JOIN HL_DEPLOY_FILES "$<SEMICOLON>" HL_DEPLOY_FILES_ARG)
add_custom_command(TARGET hl POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DCONFIG_FILE=${REPO_ROOT}/halflife_path.txt"
        "-DFILES_TO_COPY=${HL_DEPLOY_FILES_ARG}"
        -P "${REPO_ROOT}/cmake/copy_to_halflife.cmake"
    VERBATIM
    COMMENT "Copying hl.exe and the WON DLL(s) to the Half-Life folder (halflife_path.txt)")
else()
    message(STATUS "LAUNCHER_FIXES=OFF: not copying to the Half-Life folder")
endif()

# -----------------------------------------------------------------------------
# Visual Studio debugger settings (F5), for every configuration.
#
# hl.exe imports vgui.dll, and that DLL -- like hl_res.dll and the game dirs --
# lives only in the Half-Life folder, so starting the exe from build/<config>/
# dies with STATUS_DLL_NOT_FOUND before it ever opens a window.  F5 has to launch
# the *deployed* copy the POST_BUILD step above writes to the game folder.  These
# properties land in hl.vcxproj.user for all configs, so Release behaves as Debug
# does instead of depending on a hand-edited .user file (which is gitignored).
# -----------------------------------------------------------------------------
set(LAUNCHER_DEBUG_ARGS "-console -windowed -nodat -wonservertest -noauth"
    CACHE STRING "Command line the Visual Studio debugger (F5) starts hl.exe with")

set(HL_GAME_DIR "")
if(EXISTS "${REPO_ROOT}/halflife_path.txt")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${REPO_ROOT}/halflife_path.txt")
    file(STRINGS "${REPO_ROOT}/halflife_path.txt" HL_PATH_LINES)
    foreach(_line IN LISTS HL_PATH_LINES)
        string(STRIP "${_line}" _line)
        if(NOT _line STREQUAL "" AND NOT _line MATCHES "^#")
            file(TO_CMAKE_PATH "${_line}" HL_GAME_DIR)
            string(REGEX REPLACE "/+$" "" HL_GAME_DIR "${HL_GAME_DIR}")
            break()
        endif()
    endforeach()
endif()

if(IS_DIRECTORY "${HL_GAME_DIR}")
    file(TO_NATIVE_PATH "${HL_GAME_DIR}" HL_GAME_DIR_NATIVE)
    file(TO_NATIVE_PATH "${HL_GAME_DIR}/hl.exe" HL_GAME_EXE_NATIVE)
    set_target_properties(hl PROPERTIES
        VS_DEBUGGER_COMMAND "${HL_GAME_EXE_NATIVE}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "${LAUNCHER_DEBUG_ARGS}"
        VS_DEBUGGER_WORKING_DIRECTORY "${HL_GAME_DIR_NATIVE}")
else()
    message(STATUS "No usable halflife_path.txt: F5 will run build/<config>/hl.exe, "
                   "which cannot find vgui.dll -- it will exit before showing a window.")
endif()
