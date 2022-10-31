#
# Quake2 gamei386.so Makefile for Linux 2.0
#
# Jan '98 by Zoid <zoid@idsoftware.com>
#
# Modified for EraserULTRA by Anthony Jacques <AnthonyJ@btinternet.com>
#
# ELF only
#
# Probably requires GNU make
#

# Check OS type.
OSTYPE := $(shell uname -s)

#ARCH=i386
# this nice line comes from the linux kernel makefile
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc/ -e s/sparc64/sparc/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/alpha/axp/)

ifeq ($(OSTYPE),Linux)
  LDFLAGS+='-Wl,-rpath,$$ORIGIN/lib64'
else ifeq ($(OSTYPE),Darwin)
  BASE_CFLAGS:=-mmacosx-version-min=10.9
  LDFLAGS+='-Wl,-rpath,@executable_path/osx'
endif

CC=gcc
BASE_CFLAGS+=-Dstricmp=strcasecmp 

#use these cflags to optimize it
CFLAGS=$(BASE_CFLAGS) -m486 -O6 -fomit-frame-pointer \
	-fexpensive-optimizations -malign-loops=2 \
	-malign-jumps=2 -malign-functions=2
#use these when debugging 
#CFLAGS=$(BASE_CFLAGS) -g

LDFLAGS+=-ldl -lm

#SHLIBEXT=so
ifeq ($(OSTYPE),Darwin)
	SHLIBEXT=dylib
else
	SHLIBEXT=so
endif

SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

DO_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<

#############################################################################
# SETUP AND BUILD
# GAME
#############################################################################

.c.o:
	$(DO_CC)

GAME_OBJS = \
	aj_banning.o aj_confcycle.o aj_cvar.o aj_lmctf.o aj_main.o aj_menu.o \
	aj_replacelist.o aj_runes.o aj_scoreboard.o aj_statusbars.o \
	bot_ai.o bot_die.o bot_items.o bot_misc.o bot_nav.o bot_spawn.o \
	bot_wpns.o camclient.o \
	dm_ball.o dm_tag.o e_grapple.o e_hook.o funhook.o \
	g_ai.o g_chase.o g_cmds.o g_combat.o g_compress.o g_ctf.o g_func.o \
	g_items.o g_main.o g_map_mod.o g_misc.o g_model.o g_monster.o g_mtrain.o \
	g_newdm.o g_newfnc.o g_newtarg.o g_newtrig.o g_newweap.o \	
	g_phys.o g_save.o g_spawn.o g_svcmds.o g_target.o \
	g_trigger.o g_utils.o g_weapon.o g_weapon_lmctf.o g_weapon_xatrix.o \
	m_move.o \
	p_client.o p_hud.o p_menu.o p_trail.o p_view.o p_weapon.o \
	q_shared.o sf_ammosort.o \
	zlib/adler32.o \
	zlib/compress.o \
	zlib/crc32.o \
	zlib/deflate.o \
	zlib/gzclose.o \
	zlib/gzlib.o \
	zlib/gzread.o \
	zlib/gzwrite.o \
	zlib/infback.o \
	zlib/inffast.o \
	zlib/inflate.o \
	zlib/inftrees.o \
	zlib/ioapi.o \
	zlib/trees.o \
	zlib/uncompr.o \
	zlib/unzip.o \
	zlib/zip.o \
	zlib/zutil.o

game$(ARCH).$(SHLIBEXT) : $(GAME_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(GAME_OBJS)


#############################################################################
# MISC
#############################################################################

clean:
	-rm -f $(GAME_OBJS)

depend:
	gcc -MM $(GAME_OBJS:.o=.c)

