cmake_minimum_required(VERSION 3.23)
project(Sol
        DESCRIPTION "KMQuake2 fork for use in the development of Sol."
        HOMEPAGE_URL "https://hogsy.me/"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Get the current working branch
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the latest abbreviated commit hash of the working branch
execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the number of commits on the working branch
execute_process(
        COMMAND git rev-list HEAD --count
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
add_definitions(-DGIT_COMMIT_COUNT="${GIT_COMMIT_COUNT}")
add_definitions(-DGIT_BRANCH="${GIT_BRANCH}")

set(SOL_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SOL_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SOL_OUTPUT_DIR}")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${SOL_OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${SOL_OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${SOL_OUTPUT_DIR})
endforeach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})

add_library(SolGame OBJECT
        game/entity/entity.cpp
        game/entity/entity_manager.cpp
        game/entity/entity_player.cpp
        game/entity/entity_player_start.cpp

        game/acebot_ai.cpp
        game/acebot_cmds.cpp
        game/acebot_compress.cpp
        game/acebot_items.cpp
        game/acebot_movement.cpp
        game/acebot_nodes.cpp
        game/acebot_spawn.cpp

        game/g_ai.cpp
        game/g_camera.cpp
        game/g_chase.cpp
        game/g_cmds.cpp
        game/g_combat.cpp
        game/g_crane.cpp
        game/g_ctf.cpp
        game/g_fog.cpp
        game/g_func.cpp
        game/g_items.cpp
        game/g_jetpack.cpp
        game/g_lights.cpp
        game/g_lock.cpp
        game/g_main.cpp
        game/g_misc.cpp
        game/g_misc_kex.cpp
        game/g_misc_laz.cpp
        game/g_model.cpp
        game/g_monster.cpp
        game/g_moreai.cpp
        game/g_mtrain.cpp
        game/g_patchmonstermodels.cpp
        game/g_patchplayermodels.cpp
        game/g_pendulum.cpp
        game/g_phys.cpp
        game/g_reflect.cpp
        game/g_save.cpp
        game/g_spawn.cpp
        game/g_svcmds.cpp
        game/g_target.cpp
        game/g_thing.cpp
        game/g_tracktrain.cpp
        game/g_trigger.cpp
        game/g_turret.cpp
        game/g_utils.cpp
        game/g_vehicle.cpp
        game/g_weapon.cpp
        game/km_cvar.cpp
        game/m_actor.cpp
        game/m_actor_weap.cpp
        game/m_berserk.cpp
        game/m_boss2.cpp
        game/m_boss3.cpp
        game/m_boss31.cpp
        game/m_boss32.cpp
        game/m_brain.cpp
        game/m_chick.cpp
        game/m_flash.cpp
        game/m_flipper.cpp
        game/m_float.cpp
        game/m_flyer.cpp
        game/m_gladiator.cpp
        game/m_gunner.cpp
        game/m_hover.cpp
        game/m_infantry.cpp
        game/m_insane.cpp
        game/m_medic.cpp
        game/m_move.cpp
        game/m_mutant.cpp
        game/m_parasite.cpp
        game/m_soldier.cpp
        game/m_supertank.cpp
        game/m_tank.cpp
        game/p_chase.cpp
        game/p_client.cpp
        game/p_hud.cpp
        game/p_menu.cpp
        game/p_text.cpp
        game/p_trail.cpp
        game/p_view.cpp
        game/p_weapon.cpp
        game/q_shared.cpp
)

if (WIN32)
    message("TODO")
else ()
    set(SOL_SYSTEM_SOURCES
            unix/sdl_gl.cpp
            unix/sdl_snd.cpp
            unix/unix_cd.cpp
            unix/unix_glob.cpp
            unix/unix_input.cpp
            unix/unix_main.cpp
            unix/unix_net.cpp
            unix/unix_qgl.cpp
            unix/unix_qsh.cpp
            unix/unix_vid.cpp

            unix/zip/ioapi.c
            unix/zip/unzip.c
            unix/zip/zip.c
    )
endif ()

add_executable(SolEngine WIN32
        ${SOL_SYSTEM_SOURCES}

        client/cl_cin.cpp
        client/cl_cinematic.cpp
        client/cl_console.cpp
        client/cl_download.cpp
        client/cl_effects.cpp
        client/cl_ents.cpp
        client/cl_event.cpp
        client/cl_http.cpp
        client/cl_hud.cpp
        client/cl_hud_script.cpp
        client/cl_input.cpp
        client/cl_keys.cpp
        client/cl_lights.cpp
        client/cl_loc.cpp
        client/cl_main.cpp
        client/cl_parse.cpp
        client/cl_particle.cpp
        client/cl_predict.cpp
        client/cl_qcurl.cpp
        client/cl_screen.cpp
        client/cl_string.cpp
        client/cl_tempent.cpp
        client/cl_utils.cpp
        client/cl_view.cpp
        client/snd_dma.cpp
        client/snd_mem.cpp
        client/snd_mix.cpp
        client/snd_stream.cpp
        client/x86.cpp

        server/sv_ccmds.cpp
        server/sv_ents.cpp
        server/sv_game.cpp
        server/sv_init.cpp
        server/sv_main.cpp
        server/sv_send.cpp
        server/sv_user.cpp
        server/sv_world.cpp

        ui/menu_apply_changes.cpp
        ui/menu_credits.cpp
        ui/menu_defaults_confirm.cpp
        ui/menu_game.cpp
        ui/menu_game_load.cpp
        ui/menu_game_save.cpp
        ui/menu_main.cpp
        ui/menu_mods.cpp
        ui/menu_mp_addressbook.cpp
        ui/menu_mp_dmoptions.cpp
        ui/menu_mp_download.cpp
        ui/menu_mp_joinserver.cpp
        ui/menu_mp_playersetup.cpp
        ui/menu_mp_startserver.cpp
        ui/menu_multiplayer.cpp
        ui/menu_options.cpp
        ui/menu_options_controls.cpp
        ui/menu_options_effects.cpp
        ui/menu_options_interface.cpp
        ui/menu_options_keys.cpp
        ui/menu_options_screen.cpp
        ui/menu_options_sound.cpp
        ui/menu_quit.cpp
        ui/menu_video.cpp
        ui/menu_video_advanced.cpp
        ui/ui_draw.cpp
        ui/ui_main.cpp
        ui/ui_menu.cpp
        ui/ui_mouse.cpp
        ui/ui_utils.cpp
        ui/ui_widgets.cpp

        renderer/r_alias.cpp
        renderer/r_alias_md2.cpp
        renderer/r_alias_misc.cpp
        renderer/r_arb_program.cpp
        renderer/r_backend.cpp
        renderer/r_beam.cpp
        renderer/r_bloom.cpp
        renderer/r_draw.cpp
        renderer/r_entity.cpp
        renderer/r_fog.cpp
        renderer/r_fragment.cpp
        renderer/r_glstate.cpp
        renderer/r_image.cpp
        renderer/r_light.cpp
        renderer/r_main.cpp
        renderer/r_misc.cpp
        renderer/r_model.cpp
        renderer/r_particle.cpp
        renderer/r_sky.cpp
        renderer/r_sprite.cpp
        renderer/r_surface.cpp
        renderer/r_surface_singletex.cpp
        renderer/r_upscale.cpp
        renderer/r_utils.cpp
        renderer/r_vlights.cpp
        renderer/r_warp.cpp

        qcommon/cmd.cpp
        qcommon/cmodel.cpp
        qcommon/common.cpp
        qcommon/crc.cpp
        qcommon/cvar.cpp
        qcommon/filesystem.cpp
        qcommon/json_parse.cpp
        qcommon/md4.cpp
        qcommon/net_chan.cpp
        qcommon/pmove.cpp
        qcommon/wildcard.cpp
)

target_link_libraries(SolEngine SolGame m vorbisfile z png jpeg SDL2)
target_compile_options(SolEngine PRIVATE -fpermissive)
