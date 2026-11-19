cmake_minimum_required(VERSION 3.16)

set(livearea_files
    sce_sys/icon0.png
    sce_sys/livearea/contents/bg.png
    sce_sys/livearea/contents/startup.png
    sce_sys/livearea/contents/template.xml
)
set(vpk_files)
foreach(asset IN LISTS livearea_files)
    set(asset_path "${CMAKE_CURRENT_SOURCE_DIR}/${asset}")
    if(NOT EXISTS "${asset_path}" OR IS_DIRECTORY "${asset_path}")
        message(FATAL_ERROR "Recurso da LiveArea ausente ou inválido: '${asset_path}'.")
    endif()
    list(APPEND vpk_files FILE "${asset}" "${asset}")
endforeach()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if("$ENV{VITASDK}" STREQUAL "")
        message(FATAL_ERROR "Defina VITASDK com o caminho de um VitaSDK existente.")
    endif()
    foreach(sdk_file IN ITEMS share/vita.toolchain.cmake share/vita.cmake bin/arm-vita-eabi-gcc)
        if(NOT EXISTS "$ENV{VITASDK}/${sdk_file}" OR IS_DIRECTORY "$ENV{VITASDK}/${sdk_file}")
            message(FATAL_ERROR
                "VitaSDK incompleto ou caminho incorreto: arquivo ausente ou inválido em '$ENV{VITASDK}/${sdk_file}'.")
        endif()
    endforeach()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE FILEPATH "VitaSDK toolchain")
endif()

project(vista_walk LANGUAGES C)
if(NOT VITASDK)
    message(FATAL_ERROR "A toolchain selecionada precisa definir VITASDK com o caminho da instalação.")
endif()
if(NOT VITA)
    message(FATAL_ERROR "A toolchain selecionada precisa definir VITA. Use share/vita.toolchain.cmake do VitaSDK.")
endif()
set(vita_macros "${VITASDK}/share/vita.cmake")
if(NOT EXISTS "${vita_macros}" OR IS_DIRECTORY "${vita_macros}")
    message(FATAL_ERROR "Macros de empacotamento do VitaSDK ausentes ou inválidas em '${vita_macros}'.")
endif()
include("${vita_macros}")

foreach(sdk_tool IN ITEMS VITA_ELF_CREATE VITA_MAKE_FSELF VITA_MKSFOEX VITA_PACK_VPK)
    if(NOT EXISTS "${${sdk_tool}}" OR IS_DIRECTORY "${${sdk_tool}}")
        message(FATAL_ERROR
            "VitaSDK incompleto: ferramenta ${sdk_tool} ausente ou inválida em '${${sdk_tool}}'.")
    endif()
endforeach()

add_executable(vista_walk main.c sprite.c)
target_compile_features(vista_walk PRIVATE c_std_11)
target_compile_options(vista_walk PRIVATE -Wall -Wextra)
target_link_libraries(vista_walk PRIVATE
    SceDisplay_stub SceCtrl_stub SceSysmem_stub SceKernelThreadMgr_stub)

vita_create_self(vista_walk.self vista_walk)
vita_create_vpk(vista_walk.vpk HKVW00001 vista_walk.self
    VERSION "01.00"
    NAME "Handkerchief - VISTA 0"
    ${vpk_files}
)
