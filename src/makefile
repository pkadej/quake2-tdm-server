# Makefile to use with lccwin32 compiler (lcc)

CC=lcc
CFLAGS=-DC_ONLY -DLCC -D__WIN32__ -DTEXTURES_NAMES -O -errout=gamex86.err
OBJS= g_chase.obj g_cmds.obj g_combat.obj g_func.obj g_items.obj g_main.obj \
g_misc.obj g_monster.obj g_phys.obj g_spawn.obj g_svcmds.obj g_target.obj g_tdm.obj \
g_trigger.obj g_utils.obj g_weapon.obj m_move.obj p_client.obj p_hud.obj p_menu.obj \
p_view.obj p_weapon.obj q_shared.obj lcchack.obj g_convert.obj tdm_plugins.obj \
tdm_plugins_functions.obj

all:	gamex86.dll

gamex86.dll:	$(OBJS) game_lcc.def
	lcclnk -s -subsystem windows -dll -entry DLLMain *.obj pthreadvc2.lib game_lcc.def -o ../gamex86.dll

q_shared.obj:	q_shared.c q_shared.h
	$(CC) $(CFLAGS) q_shared.c

p_client.obj:	p_client.c m_player.h g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) p_client.c

p_hud.obj:	p_hud.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) p_hud.c

p_menu.obj:	p_menu.c m_player.h g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) p_menu.c

p_view.obj:	p_view.c m_player.h g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) p_view.c

p_weapon.obj:	p_weapon.c m_player.h g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) p_weapon.c

g_chase.obj:	g_chase.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_chase.c

g_cmds.obj:	g_cmds.c m_player.h g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_cmds.c

g_combat.obj:	g_combat.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_combat.c

g_convert.obj:	g_convert.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_convert.c

g_func.obj:	g_func.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_func.c

g_items.obj:	g_items.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_items.c

g_main.obj:	g_main.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_main.c

g_misc.obj:	g_misc.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_misc.c

g_monster.obj:	g_monster.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_monster.c

g_phys.obj:	g_phys.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_phys.c

g_spawn.obj:	g_spawn.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_spawn.c

g_svcmds.obj:	g_svcmds.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_svcmds.c

g_target.obj:	g_target.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_target.c

g_tdm.obj:	g_tdm.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_tdm.c

g_trigger.obj:	g_trigger.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_trigger.c

g_utils.obj:	g_utils.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_utils.c

g_weapon.obj:	g_weapon.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) g_weapon.c

m_move.obj:	m_move.c g_local.h q_shared.h game.h
	$(CC) $(CFLAGS) m_move.c

tdm_plugins.obj: tdm_plugins.c tdm_plugins_internal.h
	$(CC) $(CFLAGS) tdm_plugins.c

tdm_plugins_functions.obj: tdm_plugins_functions.c tdm_plugins_internal.h
	$(CC) $(CFLAGS) tdm_plugins_functions.c

#hack for LCC - this file contains our entry point function only required for LCC

lcchack.obj:	lcchack.c
	$(CC) $(CFLAGS) lcchack.c

clean:
	del *.obj
