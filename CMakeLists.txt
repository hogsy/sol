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

add_library(SolGame STATIC
        game/acebot_ai.c
        game/acebot_cmds.c
        game/acebot_compress.c
        game/acebot_items.c
        game/acebot_movement.c
        game/acebot_nodes.c
        game/acebot_spawn.c

        game/g_ai.c
        game/g_camera.c
        game/g_chase.c
        game/g_cmds.c
        game/g_combat.c
        game/g_crane.c
        game/g_ctf.c
        game/g_fog.c
        game/g_func.c
        game/g_items.c
        game/g_jetpack.c
        game/g_lights.c
        game/g_lock.c
        game/g_main.c
        game/g_misc.c
        game/g_misc_kex.c
        game/g_misc_laz.c
        game/g_model.c
        game/g_monster.c
        game/g_moreai.c
        game/g_mtrain.c
        game/g_patchmonstermodels.c
        game/g_patchplayermodels.c
        game/g_pendulum.c
        game/g_phys.c
        game/g_reflect.c
        game/g_save.c
        game/g_spawn.c
        game/g_svcmds.c
        game/g_target.c
        game/g_thing.c
        game/g_tracktrain.c
        game/g_trigger.c
        game/g_turret.c
        game/g_utils.c
        game/g_vehicle.c
        game/g_weapon.c
        game/km_cvar.c
        game/m_actor.c
        game/m_actor_weap.c
        game/m_berserk.c
        game/m_boss2.c
        game/m_boss3.c
        game/m_boss31.c
        game/m_boss32.c
        game/m_brain.c
        game/m_chick.c
        game/m_flash.c
        game/m_flipper.c
        game/m_float.c
        game/m_flyer.c
        game/m_gladiator.c
        game/m_gunner.c
        game/m_hover.c
        game/m_infantry.c
        game/m_insane.c
        game/m_medic.c
        game/m_move.c
        game/m_mutant.c
        game/m_parasite.c
        game/m_soldier.c
        game/m_supertank.c
        game/m_tank.c
        game/p_chase.c
        game/p_client.c
        game/p_hud.c
        game/p_menu.c
        game/p_text.c
        game/p_trail.c
        game/p_view.c
        game/p_weapon.c
        game/q_shared.c
)

if (WIN32)
    message("TODO")
else ()
    set(SOL_SYSTEM_SOURCES
            unix/sdl_gl.c
            unix/sdl_snd.c
            unix/unix_cd.c
            unix/unix_glob.c
            unix/unix_input.c
            unix/unix_main.c
            unix/unix_net.c
            unix/unix_qgl.c
            unix/unix_qsh.c
            #unix/unix_snd.c
            unix/unix_vid.c

            unix/zip/ioapi.c
            unix/zip/unzip.c
            unix/zip/zip.c
    )
endif ()

add_executable(SolEngine WIN32
        ${SOL_SYSTEM_SOURCES}

        client/cl_cin.c
        client/cl_cinematic.c
        client/cl_console.c
        client/cl_download.c
        client/cl_effects.c
        client/cl_ents.c
        client/cl_event.c
        client/cl_http.c
        client/cl_hud.c
        client/cl_hud_script.c
        client/cl_input.c
        client/cl_keys.c
        client/cl_lights.c
        client/cl_loc.c
        client/cl_main.c
        client/cl_parse.c
        client/cl_particle.c
        client/cl_predict.c
        client/cl_qcurl.c
        client/cl_screen.c
        client/cl_string.c
        client/cl_tempent.c
        client/cl_utils.c
        client/cl_view.c
        client/snd_dma.c
        client/snd_mem.c
        client/snd_mix.c
        client/snd_stream.c
        client/x86.c

        server/sv_ccmds.c
        server/sv_ents.c
        server/sv_game.c
        server/sv_init.c
        server/sv_main.c
        #server/sv_null.c
        server/sv_send.c
        server/sv_user.c
        server/sv_world.c

        ui/menu_apply_changes.c
        ui/menu_credits.c
        ui/menu_defaults_confirm.c
        ui/menu_game.c
        ui/menu_game_load.c
        ui/menu_game_save.c
        ui/menu_main.c
        ui/menu_mods.c
        ui/menu_mp_addressbook.c
        ui/menu_mp_dmoptions.c
        ui/menu_mp_download.c
        ui/menu_mp_joinserver.c
        ui/menu_mp_playersetup.c
        ui/menu_mp_startserver.c
        ui/menu_multiplayer.c
        ui/menu_options.c
        ui/menu_options_controls.c
        ui/menu_options_effects.c
        ui/menu_options_interface.c
        ui/menu_options_keys.c
        ui/menu_options_screen.c
        ui/menu_options_sound.c
        ui/menu_quit.c
        ui/menu_video.c
        ui/menu_video_advanced.c
        ui/ui_draw.c
        ui/ui_main.c
        ui/ui_menu.c
        ui/ui_mouse.c
        ui/ui_utils.c
        ui/ui_widgets.c

        renderer/r_alias.c
        renderer/r_alias_md2.c
        renderer/r_alias_misc.c
        renderer/r_arb_program.c
        renderer/r_backend.c
        renderer/r_beam.c
        renderer/r_bloom.c
        #renderer/r_cin.c
        renderer/r_draw.c
        renderer/r_entity.c
        renderer/r_fog.c
        renderer/r_fragment.c
        renderer/r_glstate.c
        renderer/r_image.c
        renderer/r_light.c
        #renderer/r_lightmap.c
        renderer/r_main.c
        renderer/r_misc.c
        renderer/r_model.c
        renderer/r_particle.c
        renderer/r_sky.c
        renderer/r_sprite.c
        renderer/r_surface.c
        renderer/r_surface_singletex.c
        renderer/r_upscale.c
        renderer/r_utils.c
        renderer/r_vlights.c
        renderer/r_warp.c

        game/m_flash.c
        game/q_shared.c

        qcommon/cmd.c
        qcommon/cmodel.c
        qcommon/common.c
        qcommon/crc.c
        qcommon/cvar.c
        qcommon/filesystem.c
        qcommon/json_parse.c
        qcommon/md4.c
        qcommon/net_chan.c
        qcommon/pmove.c
        qcommon/wildcard.c
)

target_link_libraries(SolEngine SolGame m vorbisfile z png jpeg SDL2)
