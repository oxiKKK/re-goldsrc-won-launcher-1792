# =============================================================================
# wonserver -- local WON (World Opponent Network) server emulator.
#
# Builds the standalone server executable `wonserverd` and exports the launcher-side
# (client) sources to the parent build so the `hl` target can compile them into the
# launcher.  See README.md for the architecture.
#
# Used two ways:
#   * As a subproject:  add_subdirectory(wonserver) from the top-level CMakeLists.
#     The parent then picks up ${WONSERVER_CLIENT_SOURCES} for its launcher target.
#   * Standalone:        cmake -S wonserver -B out -A Win32   (builds wonserverd only)
# =============================================================================
cmake_minimum_required(VERSION 3.25)

# Stand up a project + the MSVC-runtime policy only when built on our own (when
# included via add_subdirectory the parent has already done this).
if(NOT DEFINED PROJECT_NAME)
    project(wonserver LANGUAGES CXX)
    cmake_policy(SET CMP0091 NEW)   # MSVC_RUNTIME_LIBRARY selects the CRT per target
    set(WONSERVER_STANDALONE TRUE)
endif()

# The hlwon/won headers (won_transport.h, won_msg.h) sit one level up.
set(WONSERVER_HLWON_WON "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/won")
set(WONSERVER_TITANAPI  "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/TitanApi")

# The server SIGNS key blocks / certificates and runs the EG+Blowfish handshake, so
# it needs the WON crypto/auth code statically.  That is exactly common.lib +
# crypt.lib + auth.lib, which hlwon/TitanApi builds from the original project source
# lists -- link those instead of keeping a second copy of the lists here.  Built
# standalone, pull that directory in ourselves.
if(WONSERVER_STANDALONE)
    add_compile_definitions(_USE_32BIT_TIME_T)   # see the root CMakeLists
    add_subdirectory("${WONSERVER_TITANAPI}" "${CMAKE_CURRENT_BINARY_DIR}/TitanApi")
endif()

set(WONSERVER_WONLIBS won_auth won_crypt won_common)

# -----------------------------------------------------------------------------
# wonserverd -- the standalone server EXE (TCP Titan protocol + UDP room status).
# Console app, no MFC, dynamic CRT.
# -----------------------------------------------------------------------------
add_executable(wonserverd
    # The WON serialize/deserialize classes -- the same ones the launcher uses, so
    # both ends of the wire share one implementation.
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/WriteBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/ReadBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonwire.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonserver.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonserver_udp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonserver_master.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonserver_mod.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonserver_gamesv.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonserver_auth.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonserver_room.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wonserverd_main.cpp")
set_target_properties(wonserverd PROPERTIES
    OUTPUT_NAME "wonserverd"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")   # /MT, /MTd (match WONCrypt)
target_include_directories(wonserverd PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${WONSERVER_HLWON_WON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi"
    "${WONSERVER_TITANAPI}")
target_compile_definitions(wonserverd PRIVATE
    WIN32 _CONSOLE WONSERVER_MOCK _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(wonserverd PRIVATE /EHsc /GR)
target_link_libraries(wonserverd PRIVATE ${WONSERVER_WONLIBS} ws2_32)

# wondirtest -- drives the directory fetch against wonserverd through the launcher's
# OWN code path (TitanRequest -> EasyTitanSocket -> EasySocket, WONComm_GetDirectory,
# WON_ParseDirReply), so the protocol is verified against the real client stack rather
# than a stand-in.
add_executable(wondirtest
    "${CMAKE_CURRENT_SOURCE_DIR}/dirtest.cpp"
    "${WONSERVER_HLWON_WON}/won_dir.cpp"
    "${WONSERVER_HLWON_WON}/won_dirparse.cpp"
    "${WONSERVER_HLWON_WON}/won_msg.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/TitanRequest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/AuthRequest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/crc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/EasySocket.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/EasySocketError.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/EasyTitanSocket.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/WriteBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi/ReadBuffer.cpp")
set_target_properties(wondirtest PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_include_directories(wondirtest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${WONSERVER_HLWON_WON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../hlwon/cryptapi"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common"
    "${CMAKE_CURRENT_SOURCE_DIR}/../shell/hl_res"
    "${WONSERVER_TITANAPI}"
    "${WONSERVER_TITANAPI}/auth"
    "${WONSERVER_TITANAPI}/common"
    "${WONSERVER_TITANAPI}/crypt")
target_compile_definitions(wondirtest PRIVATE WIN32 _CONSOLE _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG>
    # AuthRequest reaches WONCrypt/WONAuth exactly the way hl.exe does, so this
    # follows the same LAUNCHER_STATIC switch -- see the root CMakeLists.txt.
    $<$<BOOL:${LAUNCHER_STATIC}>:WON_STATIC_LINK>)
target_compile_options(wondirtest PRIVATE /EHsc /GR)
target_link_libraries(wondirtest PRIVATE WONCrypt WONAuth ws2_32)
if(NOT LAUNCHER_STATIC)
add_custom_command(TARGET wondirtest POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:WONCrypt>" "$<TARGET_FILE_DIR:wondirtest>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:WONAuth>" "$<TARGET_FILE_DIR:wondirtest>"
    VERBATIM)
endif()

# wonauthtest -- drives the Auth handshake against wonserverd the way the launcher
# does (same WONCrypt classes), to validate the server without the original binary.
add_executable(wonauthtest "${CMAKE_CURRENT_SOURCE_DIR}/authtest.cpp")
set_target_properties(wonauthtest PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_include_directories(wonauthtest PRIVATE "${WONSERVER_TITANAPI}")
target_compile_definitions(wonauthtest PRIVATE WIN32 _CONSOLE _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(wonauthtest PRIVATE /EHsc /GR)
target_link_libraries(wonauthtest PRIVATE ${WONSERVER_WONLIBS} ws2_32)

add_executable(wonprotocoltest "${CMAKE_CURRENT_SOURCE_DIR}/protocoltest.cpp")
set_target_properties(wonprotocoltest PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(wonprotocoltest PRIVATE WIN32 _CONSOLE _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(wonprotocoltest PRIVATE /EHsc /GR)
target_link_libraries(wonprotocoltest PRIVATE ws2_32)

# Nothing from here is compiled into the launcher.  The client reaches this server
# the same way it reached the real WON: over a socket, at the address it reads from
# woncomm.lst.  Point that at localhost and the launcher needs no knowledge of us.

# wonauthcompat -- checks our packed Auth1 public-key block against the GENUINE
# WONAuth.dll (loaded at runtime from the Half-Life folder).  This is the loop the
# vanilla launcher runs inside WONAuth_FetchPubKeyBlock (0x47ba10); a block the real
# DLL cannot parse throws through hl.exe and aborts it.
add_executable(wonauthcompat "${CMAKE_CURRENT_SOURCE_DIR}/authcompat.cpp")
set_target_properties(wonauthcompat PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_include_directories(wonauthcompat PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${WONSERVER_TITANAPI}"
    "${WONSERVER_TITANAPI}/auth"
    "${WONSERVER_TITANAPI}/common"
    "${WONSERVER_TITANAPI}/crypt")
target_compile_definitions(wonauthcompat PRIVATE WIN32 _CONSOLE _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(wonauthcompat PRIVATE /EHa /GR)
target_link_libraries(wonauthcompat PRIVATE ${WONSERVER_WONLIBS})
