# =============================================================================
# TitanApi -- the WON crypto/auth libraries, built the way the original was.
#
# One target per original Developer Studio project:
#
#   common/common.dsp  -> common.lib  (won_common)  ---.
#   crypt/crypt.dsp    -> crypt.lib   (won_crypt)   ---+-> linked into BOTH DLLs
#   auth/auth.dsp      -> auth.lib    (won_auth)    ---'
#
#   crypt/WONCrypt.dsp -> WONCrypt.dll = crypt/WON_*.cpp + the three libs
#   auth/WONAuth.dsp   -> WONAuth.dll  = auth/WON_*.cpp  + the three libs
#   Makefile           -> WONDLL.dll   = the flat C API (not yet built, see below)
#
# The WON_* wrapper TUs are the only difference between the two DLLs: each one
# #defines _WON_SOURCE_ at the top, which flips DLL_OPER from dllimport to
# dllexport.  So the set of wrappers compiled into a target *is* its export
# table -- no .def files and no macro surgery.  The wrappers also keep STL and
# exceptions out of the ABI, which is why these headers still compile as VC4.2
# did and why the modern toolchain reproduces the 2001 mangled names exactly.
#
# Source lists are spelled out in .dsp order rather than globbed: the glob is
# what let the two DLLs collapse into one merged binary, and the order is the
# record of what each original project contained.
#
# Targets are prefixed won_ to avoid colliding with such generic CMake names;
# OUTPUT_NAME puts the original names back on the .lib files.
# =============================================================================

# common.dsp.  UTF8String.cpp is listed there but is left out here: it assumes the
# VC6 default of wchar_t as a typedef, so that common/won.h's `wchar` (unsigned
# short) is what MultiByteToWideChar takes.  Modern MSVC makes wchar_t a distinct
# type and it no longer compiles.  Nothing links it, so it stays vendored but
# unbuilt rather than being ported away from the original.
set(WON_COMMON_SOURCES
    common/CRC16.cpp
    common/CRC32.cpp
    common/DataObject.cpp
    common/EventLog.cpp
    common/RawList.cpp
    common/RegKey.cpp
    common/Threadbase.cpp
    common/Utils.cpp
    common/WONDebug.cpp
    common/WONException.cpp
    common/WONString.cpp)

# crypt.dsp.  algebra.cpp and eprecomp.cpp are listed in the original project but
# are template-only Crypto++ TUs that do not stand alone on a conformant
# compiler; they are included by their headers where needed.
set(WON_CRYPT_SOURCES
    crypt/cryptoFiles/asn.cpp
    crypt/cryptoFiles/bfinit.cpp
    crypt/cryptoFiles/blowfish.cpp
    crypt/cryptoFiles/cbc.cpp
    crypt/cryptoFiles/cryptlib.cpp
    crypt/cryptoFiles/elgamal.cpp
    crypt/cryptoFiles/filters.cpp
    crypt/cryptoFiles/integer.cpp
    crypt/cryptoFiles/iterhash.cpp
    crypt/cryptoFiles/md5.cpp
    crypt/cryptoFiles/misc.cpp
    crypt/cryptoFiles/modes.cpp
    crypt/cryptoFiles/modexppc.cpp
    crypt/cryptoFiles/nbtheory.cpp
    crypt/cryptoFiles/nr.cpp
    crypt/cryptoFiles/pch.cpp
    crypt/cryptoFiles/pubkey.cpp
    crypt/cryptoFiles/queue.cpp
    crypt/cryptoFiles/randpool.cpp
    crypt/BFSymmetricKey.cpp
    crypt/BigInteger.cpp
    crypt/BigNum.cpp
    crypt/CryptKeyBase.cpp
    crypt/EGPrivateKey.cpp
    crypt/EGPublicKey.cpp
    crypt/IntegerExtractor.cpp
    crypt/IntegerInserter.cpp
    crypt/MD5Digest.cpp
    crypt/MD5Hash.cpp
    crypt/MiniBlowfish.cpp
    crypt/MiniElGamal.cpp
    crypt/Random.cpp
    crypt/Randomizer.cpp)

# auth.dsp
set(WON_AUTH_SOURCES
    auth/Auth1Certificate.cpp
    auth/Auth1PublicKeyBlock.cpp
    auth/Auth2Certificate.cpp
    auth/AuthCertificateBase.cpp
    auth/AuthFamilyBuffer.cpp
    auth/AuthPublicKeyBlockBase.cpp
    auth/Session.cpp)

# WONCrypt.dsp / WONAuth.dsp -- the exported wrapper layer.
set(WONCRYPT_SOURCES
    crypt/WON_BFSymmetricKey.cpp
    crypt/WON_CryptFactory.cpp
    crypt/WON_CryptKeyBase.cpp
    crypt/WON_EGPrivateKey.cpp
    crypt/WON_EGPublicKey.cpp)

set(WONAUTH_SOURCES
    auth/WON_AuthCertificate1.cpp
    auth/WON_AuthFactory.cpp
    auth/WON_AuthFamilyBuffer.cpp
    auth/WON_AuthPublicKeyBlock1.cpp)

set(TITANAPI_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/auth"
    "${CMAKE_CURRENT_SOURCE_DIR}/crypt"
    "${CMAKE_CURRENT_SOURCE_DIR}/crypt/cryptoFiles"
    "${CMAKE_CURRENT_SOURCE_DIR}/common")

# The DLLs go next to hl.exe rather than under this subdirectory, so the launcher
# runs from the build tree and the export check has one place to look.
set(WON_DLL_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")

# The .dsp flag lines: /MT[d], /GR /GX, and the debug-only WON trace defines.
function(won_titanapi_target tgt)
    set_target_properties(${tgt} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")   # /MT, /MTd
    target_compile_features(${tgt} PRIVATE cxx_std_14)
    target_include_directories(${tgt} PUBLIC ${TITANAPI_INCLUDES})
    target_compile_definitions(${tgt} PRIVATE
        WIN32 _WINDOWS _CRT_SECURE_NO_WARNINGS
        $<$<CONFIG:Debug>:_DEBUG WON_DEBUGENABLE WON_TRACEENABLE _NO_TOPLEVEL_CATCH>
        $<$<CONFIG:Release>:NDEBUG>)
    target_compile_options(${tgt} PRIVATE /Zc:threadSafeInit- /Zc:forScope- /EHsc /GR)
endfunction()

add_library(won_common STATIC ${WON_COMMON_SOURCES})
set_target_properties(won_common PROPERTIES OUTPUT_NAME "common")
won_titanapi_target(won_common)

add_library(won_crypt STATIC ${WON_CRYPT_SOURCES})
set_target_properties(won_crypt PROPERTIES OUTPUT_NAME "crypt")
won_titanapi_target(won_crypt)

add_library(won_auth STATIC ${WON_AUTH_SOURCES})
set_target_properties(won_auth PROPERTIES OUTPUT_NAME "auth")
won_titanapi_target(won_auth)

# Both DLLs link all three libs, as both original projects do.  Only the objects
# they actually reference are pulled in, which is why the 2001 binaries -- and
# these -- import nothing but KERNEL32.
#
# LAUNCHER_STATIC (see the root CMakeLists.txt) builds these as STATIC libraries
# instead, folded straight into hl.exe rather than imported by name -- the whole
# point of the option.  WON_STATIC_LINK goes with that: it collapses the
# dllexport/dllimport in WON_CryptFactory.h/WON_AuthFactory.h's DLL_OPER macro to
# nothing, since there is no DLL boundary left for it to describe on either side.
if(LAUNCHER_STATIC)
    set(WON_DLL_LIB_TYPE STATIC)
else()
    set(WON_DLL_LIB_TYPE SHARED)
endif()

add_library(WONCrypt ${WON_DLL_LIB_TYPE} ${WONCRYPT_SOURCES})
set_target_properties(WONCrypt PROPERTIES
    OUTPUT_NAME "WONCrypt"
    RUNTIME_OUTPUT_DIRECTORY "${WON_DLL_OUTPUT_DIR}")
won_titanapi_target(WONCrypt)
target_compile_definitions(WONCrypt PRIVATE _USRDLL _WINDLL
    $<$<BOOL:${LAUNCHER_STATIC}>:WON_STATIC_LINK>)
target_link_libraries(WONCrypt PRIVATE won_crypt won_auth won_common)

add_library(WONAuth ${WON_DLL_LIB_TYPE} ${WONAUTH_SOURCES})
set_target_properties(WONAuth PROPERTIES
    OUTPUT_NAME "WONAuth"
    RUNTIME_OUTPUT_DIRECTORY "${WON_DLL_OUTPUT_DIR}")
won_titanapi_target(WONAuth)
target_compile_definitions(WONAuth PRIVATE _USRDLL _WINDLL
    $<$<BOOL:${LAUNCHER_STATIC}>:WON_STATIC_LINK>)
target_link_libraries(WONAuth PRIVATE won_auth won_crypt won_common)

# -----------------------------------------------------------------------------
# WONDLL.dll -- the flat C API (WONInitialize, WONAuthLoginA, ... 399 exports).
#
# Built from the object list in this directory's Makefile, which is the real
# recipe: WONDLL.dsp is a stub listing only wondll.h, wondll.def and two Routing
# headers.  The 45 TUs below sit on top of the same common/crypt/auth code the
# two C++ DLLs use, so those are linked in rather than recompiled.
#
# /Zc:wchar_t- is required, not cosmetic: these sources assume the VC6 default
# where wchar_t is a typedef, so won.h's `wchar` (unsigned short) IS wchar_t and
# WONString's basic_string<wchar> resolves.  It is scoped to this target -- it
# changes wchar_t name mangling, and hl.exe uses wchar_t throughout.
# -----------------------------------------------------------------------------
set(WONDLL_SOURCES
    Errors.cpp
    Ping.cpp
    WONHTTP.cpp
    SDKCommon/Completion.cpp
    SDKCommon/MemStream.cpp
    SDKCommon/Stream.cpp
    Socket/IPSocket.cpp
    Socket/RawIPSocket.cpp
    Socket/TCPSocket.cpp
    Socket/IPXSocket.cpp
    Socket/SPXSocket.cpp
    Socket/TMsgSocket.cpp
    Socket/WSSocket.cpp
    Socket/PXSocket.cpp
    Socket/WONSocket.cpp
    Socket/UDPSocket.cpp
    # Windows-only, so the (Linux) Makefile does not list it; Winsock lives here.
    Socket/WONWS.cpp
    auth/Auth1PrivateKeyBlock.cpp
    msg/BadMsgException.cpp
    msg/TMessage.cpp
    msg/Contest/MMsgContestDBCall.cpp
    msg/Auth/AuthMsgs.cpp
    msg/Comm/CommMsgs.cpp
    msg/Routing/RoutingMsgs.cpp
    msg/Firewall/FirewallMsgs.cpp
    msg/Fact/FactMsgs.cpp
    msg/Dir/DirMsgs.cpp
    msg/Dir/DirMsgsG2.cpp
    msg/Obs/ObsMsgs.cpp
    msg/Profile/SMsgProfileMsgs.cpp
    Authentication/AuthSocket.cpp
    Authentication/ClientCDKey.cpp
    Authentication/Identity.cpp
    Authentication/Md52.cpp
    Contest/ContestAPI.cpp
    Directory/DirectoryAPI.cpp
    Directory/Legacy.cpp
    Factory/FactoryAPI.cpp
    Firewall/FirewallAPI.cpp
    Observation/ObservationAPI.cpp
    Observation/SocketMgr.cpp
    Routing/RoutingAPI.cpp
    Routing/RoutingAPITypes.cpp
    Routing/CRoutingAPI.cpp
    Profile/ProfileAPI.cpp
    Event/EventAPI.cpp
    # EventAPI needs these; the Makefile's object list predates the Event service.
    msg/Event/EventMsgs.cpp)

# The Makefile compiles the common/crypt/auth sources straight into WONDLL rather
# than linking common.lib/crypt.lib/auth.lib, and here that is required, not just
# faithful: /Zc:wchar_t- makes WONString's basic_string<wchar> a
# basic_string<unsigned short>, which does not match the same class built without
# the flag for WONCrypt/WONAuth.  Sharing the static libs would be an ABI mismatch.
add_library(WONDLL SHARED
    ${WONDLL_SOURCES}
    ${WON_COMMON_SOURCES}
    ${WON_CRYPT_SOURCES}
    ${WON_AUTH_SOURCES})
set_target_properties(WONDLL PROPERTIES
    OUTPUT_NAME "WONDLL"
    RUNTIME_OUTPUT_DIRECTORY "${WON_DLL_OUTPUT_DIR}"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_features(WONDLL PRIVATE cxx_std_14)
target_include_directories(WONDLL PRIVATE ${TITANAPI_INCLUDES}
    "${CMAKE_CURRENT_SOURCE_DIR}/msg"
    "${CMAKE_CURRENT_SOURCE_DIR}/Socket"
    "${CMAKE_CURRENT_SOURCE_DIR}/SDKCommon"
    "${CMAKE_CURRENT_SOURCE_DIR}/Routing"
    "${CMAKE_CURRENT_SOURCE_DIR}/Directory")
target_compile_definitions(WONDLL PRIVATE
    WIN32 _WINDOWS _MBCS _USRDLL WONDLL_EXPORTS
    _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS
    # Selects Socket/WONWS.cpp's LoadLibrary/GetProcAddress path over binding
    # Winsock statically.  The 2001 DLL imports only ADVAPI32/KERNEL32/USER32/
    # WINMM, so that is the setting it shipped with.
    WINSOCK_DYNAMIC
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(WONDLL PRIVATE
    /Zc:threadSafeInit- /Zc:wchar_t- /Zc:forScope- /EHsc /GR)
# No wsock32 here on purpose: Socket/WONWS.cpp loads Winsock at runtime through
# LoadLibrary/GetProcAddress, which is why the 2001 DLL imports only ADVAPI32,
# KERNEL32, USER32 and WINMM.  Linking it statically would add an import the
# original does not have.
target_link_libraries(WONDLL PRIVATE winmm advapi32)
# The original exports undecorated C names through a .def rather than dllexport.
# wondll_1792.def carries the 399 the 2001 binary exports; the SDK's own
# wondll.def is a later revision listing 483 and is kept only as reference.
target_link_options(WONDLL PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/wondll_1792.def")
