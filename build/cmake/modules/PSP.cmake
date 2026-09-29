# PSP.cmake - Sony PSP (pspdev / psp-cmake) build.
#
#   psp-cmake -S . -B out-psp -DCMAKE_BUILD_TYPE=Release && cmake --build out-psp
#
# Software renderer only (no OpenGL), no networking (enet/curl/upnp stubbed),
# FMV through libsmacker instead of ffmpeg. SDL3 / SDL3_mixer / SDL3_image / OpenAL / Lua 5.4 /
# zlib / minizip come from pspdev; spng, centijson and astronomy are compiled
# from the sources vendored under deps/psp/.

kfx_status("PLATFORM" "Sony PSP (MIPS Allegrex, pspdev)")

set(KFX_PSP_DEPS "${CMAKE_SOURCE_DIR}/deps/psp")

add_compile_definitions("DEBUG=$<IF:$<CONFIG:Debug>,1,0>" _GNU_SOURCE KFX_PSP=1 KFX_NO_OPENGL=1 KFX_NO_NETWORK=1 KFX_FMV_SMACKER=1 KFX_LAZY_SOUND_BANKS=1 KFX_GAME_ON_HEAP=1)

# ---- Sources ----
file(GLOB_RECURSE KEEPERFX_SOURCES_C   CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE KEEPERFX_SOURCES_CXX CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(FILTER KEEPERFX_SOURCES_C   EXCLUDE REGEX "/src/ftests/")
list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX "/src/ftests/")
list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX "/PlatformWindows\\.cpp$|/WindowCompositorWin\\.cpp$|/GLContextSDL\\.cpp$")
list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX "/kfx/renderer/opengl/|/RendererOpenGL\\.cpp$")
# Networking backends, replaced by src/psp/net_stub.c.
list(FILTER KEEPERFX_SOURCES_CXX EXCLUDE REGEX "/bflib_enet\\.cpp$|/net_portforward\\.cpp$")
list(FILTER KEEPERFX_SOURCES_C   EXCLUDE REGEX "/net_lan\\.c$|/net_holepunch\\.c$|/net_matchmaking\\.c$")

# Window icon (referenced by WindowSystemSDL; harmless on the PSP).
set(KFX_ICON_C "${CMAKE_BINARY_DIR}/generated/window_icon.c")
if(NOT EXISTS "${KFX_ICON_C}")
    file(READ "${CMAKE_SOURCE_DIR}/res/keeperfx_icon256-24bpp.png" _icon_hex HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _icon_bytes "${_icon_hex}")
    file(WRITE "${KFX_ICON_C}"
        "const unsigned char kfx_window_icon_png[] = {${_icon_bytes}};\n"
        "const unsigned int kfx_window_icon_png_size = sizeof(kfx_window_icon_png);\n")
endif()
list(APPEND KEEPERFX_SOURCES_C "${KFX_ICON_C}")

add_executable(keeperfx ${KEEPERFX_SOURCES_C} ${KEEPERFX_SOURCES_CXX})
target_compile_definitions(keeperfx PUBLIC BFDEBUG_LEVEL=0)
target_include_directories(keeperfx PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/psp")
target_compile_options(keeperfx PRIVATE
    -O2 -G0 -Wall -Wno-unused-parameter -Wno-unknown-pragmas -Wno-sign-compare
    -Wno-format-truncation -Wno-missing-field-initializers
    "SHELL:-include ${CMAKE_SOURCE_DIR}/src/psp/psp_compat.h")

# ---- Third-party libs built from source ----
add_library(kfx_spng STATIC "${KFX_PSP_DEPS}/libspng/spng/spng.c")
target_include_directories(kfx_spng PUBLIC "${KFX_PSP_DEPS}/libspng/spng")
target_compile_definitions(kfx_spng PUBLIC SPNG_STATIC=1)

add_library(kfx_centijson STATIC
    "${KFX_PSP_DEPS}/centijson/src/json.c"
    "${KFX_PSP_DEPS}/centijson/src/json-dom.c"
    "${KFX_PSP_DEPS}/centijson/src/json-ptr.c"
    "${KFX_PSP_DEPS}/centijson/src/value.c")
target_include_directories(kfx_centijson PUBLIC "${KFX_PSP_DEPS}/centijson/src")

add_library(kfx_astronomy STATIC "${KFX_PSP_DEPS}/astronomy.c")
target_include_directories(kfx_astronomy PUBLIC "${KFX_PSP_DEPS}")
target_compile_definitions(kfx_astronomy PRIVATE ASTRONOMY_ENGINE_WHOLE_SECOND)

# Classic zlib contrib/minizip (unzip.h API); pspdev ships minizip-ng without the compat layer.
add_library(kfx_minizip STATIC
    "${KFX_PSP_DEPS}/minizip/minizip/unzip.c"
    "${KFX_PSP_DEPS}/minizip/minizip/ioapi.c")
target_include_directories(kfx_minizip PUBLIC "${KFX_PSP_DEPS}/minizip")
target_compile_definitions(kfx_minizip PRIVATE NOCRYPT USE_FILE32API)

add_library(kfx_smacker STATIC "${KFX_PSP_DEPS}/libsmacker/smacker.c")
target_include_directories(kfx_smacker PUBLIC "${KFX_PSP_DEPS}/libsmacker")

add_library(centitoml OBJECT "${CMAKE_SOURCE_DIR}/deps/centitoml/toml_api.c")
target_link_libraries(centitoml PUBLIC kfx_centijson)
target_include_directories(centitoml INTERFACE "${CMAKE_SOURCE_DIR}/deps/centitoml")

# ---- pspdev libraries ----
target_link_libraries(keeperfx PRIVATE
    kfx_spng kfx_centijson kfx_astronomy kfx_minizip kfx_smacker centitoml
    SDL3_mixer SDL3_image SDL3
    openal lua z
    xmp-lite vorbisfile vorbis ogg FLAC mpg123 opusfile opus png jpeg
    GL pspvram pspvramalloc pspgu pspge pspdisplay pspctrl pspaudio pspaudiolib psphprm
    psppower psprtc pspmp3 pspatrac3 pspsdk pspnet_inet pspnet_apctl pspnet_resolver psputility
    pspvfpu atomic pthread stdc++ m)

target_link_options(keeperfx PRIVATE -Wl,-Map,keeperfx.map)

option(KFX_PSP_MEMDEBUG "Log large heap allocations and allocation failures to stderr" OFF)
option(KFX_PSP_PERFLOG "Log presented frames per second to keeperfx.log" OFF)
if(KFX_PSP_PERFLOG)
    target_compile_definitions(keeperfx PRIVATE KFX_PSP_PERFLOG=1)
endif()
if(KFX_PSP_MEMDEBUG)
    target_link_options(keeperfx PRIVATE -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=free)
else()
    list(FILTER KEEPERFX_SOURCES_C EXCLUDE REGEX "/psp_memdebug\\.c$")
    set_source_files_properties("${CMAKE_SOURCE_DIR}/src/psp/psp_memdebug.c" PROPERTIES HEADER_FILE_ONLY ON)
endif()

# Signed (encrypted) PRX, which would also start without custom firmware
# (unsigned homebrew fails there with 80020148). Off: PrxEncrypter can only
# forge headers for PRX files up to ~5.5 MB and KeeperFX's is ~10 MB, so the
# EBOOT needs an active custom firmware (PRO/LME/ARK/Infinity).
option(KFX_PSP_ENC_PRX "Encrypt/sign the PRX inside EBOOT.PBP" OFF)
if(KFX_PSP_ENC_PRX)
    set(KFX_PSP_ENC_ARG ENC_PRX)
endif()

include("${PSPDEV}/psp/share/CreatePBP.cmake")
create_pbp_file(
    TARGET keeperfx
    TITLE "KeeperFX"
    # Must not be empty: an empty APP_VER string makes the XMB reject the
    # PARAM.SFO (no title shown, and the game refuses to start).
    VERSION "01.40"
    ICON_PATH "${CMAKE_SOURCE_DIR}/psp/ICON0.PNG"
    BACKGROUND_PATH NULL
    PREVIEW_PATH NULL
    MEMSIZE 1 # PSP-2000+ extended (~52 MB) user memory
    BUILD_PRX
    ${KFX_PSP_ENC_ARG}
)

kfx_status("BUILD" "${CMAKE_CXX_COMPILER_ID} -> keeperfx (EBOOT.PBP)")
