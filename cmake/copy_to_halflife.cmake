# =============================================================================
# Post-build copy step: copy the built launcher + WONCrypt DLL into the
# Half-Life game folder named in a config file on disk.
#
# Run via `cmake -P` from a POST_BUILD command.  Required -D variables:
#   CONFIG_FILE   - path to the config file holding the destination folder
#   FILES_TO_COPY - ';'-separated list of built files to copy
#
# The config file is read at *build* time (not configure time), so editing the
# path takes effect on the next build without re-running CMake.  A missing
# config file is not an error -- the copy is simply skipped.
# =============================================================================

if(NOT EXISTS "${CONFIG_FILE}")
    message(STATUS "copy_to_halflife: no config file at '${CONFIG_FILE}' -- skipping copy. "
                   "Create it (see halflife_path.txt.example) to enable.")
    return()
endif()

# Read the config and pick the first non-blank, non-comment line as the path.
file(STRINGS "${CONFIG_FILE}" _lines)
set(DEST "")
foreach(_line IN LISTS _lines)
    string(STRIP "${_line}" _line)
    if(_line STREQUAL "" OR _line MATCHES "^#")
        continue()
    endif()
    set(DEST "${_line}")
    break()
endforeach()

if(DEST STREQUAL "")
    message(WARNING "copy_to_halflife: config file '${CONFIG_FILE}' has no path -- skipping copy.")
    return()
endif()

if(NOT IS_DIRECTORY "${DEST}")
    message(WARNING "copy_to_halflife: destination '${DEST}' is not an existing directory -- skipping copy.")
    return()
endif()

foreach(_file IN LISTS FILES_TO_COPY)
    if(NOT EXISTS "${_file}")
        message(WARNING "copy_to_halflife: '${_file}' does not exist -- skipping it.")
        continue()
    endif()
    message(STATUS "copy_to_halflife: copying '${_file}' -> '${DEST}'")
    file(COPY "${_file}" DESTINATION "${DEST}")
endforeach()
