#include "g_local.h"
#include "m_player.h"
#include "bot.h"

char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	Com_strcpy (value, sizeof(value), Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	Com_strcpy (ent1Team, sizeof(ent1Team), ClientTeam (ent1));
	Com_strcpy (ent2Team, sizeof(ent2Team), ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

//ZOID
/*	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}*/
	// Bouman fix- Fixed 'cmd invnext' in the CTF menu. It only works with Chasecam before.
	if (cl->menu) {
		PMenu_Next(ent);
		return;
	} else if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}
//ZOID

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

//ZOID
	if (cl->menu) {
		PMenu_Prev(ent);
		return;
	} else if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}
//ZOID

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.dprintf ("unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.dprintf ("non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		// RAFAEL
		if (strcmp (it->pickup_name, "HyperBlaster") == 0)
		{
			it = Fdi_BOOMER;//FindItem ("Ionripper");
			index = ITEM_INDEX (it);
			if (!ent->client->pers.inventory[index])
			{
				gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		// RAFAEL
		else if (strcmp (it->pickup_name, "Railgun") == 0)
		{
			it = Fdi_PHALANX;//FindItem ("Phalanx");
			index = ITEM_INDEX (it);
			if (!ent->client->pers.inventory[index])
			{
				gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		else 
		{
			gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
			return;
		}
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

//ZOID--special case for tech powerups
	if (Q_stricmp(gi.args(), "tech") == 0 && (it = CTFWhat_Tech(ent)) != NULL) {
		it->drop (ent, it);
		return;
	}
//ZOID

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		// RAFAEL
		if (strcmp (it->pickup_name, "HyperBlaster") == 0)
		{
			it = Fdi_BOOMER;//FindItem ("Ionripper");
			index = ITEM_INDEX (it);
			if (!ent->client->pers.inventory[index])
			{
				gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		// RAFAEL
		else if (strcmp (it->pickup_name, "Railgun") == 0)
		{
			it = Fdi_PHALANX;//FindItem ("Phalanx");
			index = ITEM_INDEX (it);
			if (!ent->client->pers.inventory[index])
			{
				gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		else 
		{
			gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
			return;
		}
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	if(ent->svflags & SVF_MONSTER) return;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

//ZOID
	if (ent->client->menu) {
		PMenu_Close(ent);
		ent->client->update_chase = true;
		return;
	}
//ZOID

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

//ZOID
	if (ctf->value && cl->resp.ctf_team == CTF_NOTEAM) {
		CTFOpenJoinMenu(ent);
		return;
	}
//ZOID

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

//ZOID
	if (ent->client->menu) {
		PMenu_Select(ent);
		return;
	}
//ZOID

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

//ZOID
/*
=================
Cmd_LastWeap_f
=================
*/
void Cmd_LastWeap_f (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	cl->pers.lastweapon->use (ent, cl->pers.lastweapon);
}
//ZOID


/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
//ZOID
	if (ent->solid == SOLID_NOT)
		return;
//ZOID

	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
	// don't even bother waiting for death frames
	ent->deadflag = DEAD_DEAD;
	respawn (ent);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
//ZOID
	if (ent->client->menu)
		PMenu_Close(ent);
	ent->client->update_chase = true;
//ZOID
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			Com_strcat (large, sizeof(large), "...\n");
			break;
		}
		Com_strcat (large, sizeof(large), small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		j;
	edict_t	*other;
	char	*p;
	char	text[2048];

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		Com_strcat (text, sizeof(text), gi.argv(0));
		Com_strcat (text, sizeof(text), " ");
		Com_strcat (text, sizeof(text), gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		Com_strcat (text, sizeof(text), p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	Com_strcat (text, sizeof(text), "\n");

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		if (other->svflags & SVF_MONSTER) continue;

		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}
//�X�i�C�p�[�pZoomIn Out
void Cmd_ZoomIn (edict_t *ent)
{
	if( ent->client->zc.autozoom )
	{
		gi.cprintf(ent,PRINT_HIGH,"autozoom has been selected.\n");
		return;
	}

//	if(	ent->client->pers.weapon != FindItem("Railgun")) return;

	if( ent->client->zc.aiming != 1 && ent->client->zc.aiming != 3) return;

	if(ent->client->zc.distance < 15 || ent->client->zc.distance > 90)
	{
		ent->client->zc.distance = 90;
		ent->client->ps.fov = 90;
	}
	
	if(ent->client->zc.distance > 15)
	{
		gi.sound (ent, CHAN_AUTO, gi.soundindex("3zb/zoom.wav"), 1, ATTN_NORM, 0);
		if(ent->client->zc.distance == 90 ) ent->client->zc.distance = 65;
		else if(ent->client->zc.distance == 65 ) ent->client->zc.distance = 40;
		else ent->client->zc.distance = 15;
		ent->client->ps.fov = ent->client->zc.distance;
	}
}
void Cmd_ZoomOut (edict_t *ent)
{
	if( ent->client->zc.autozoom )
	{
		gi.cprintf(ent,PRINT_HIGH,"autozoom has been selected.\n");
		return;
	}

//	if(	ent->client->pers.weapon != FindItem("Railgun")) return;

	if(ent->client->zc.aiming != 1 && ent->client->zc.aiming != 3) return;	

	if(ent->client->zc.distance < 15 || ent->client->zc.distance > 90)
	{
		ent->client->zc.distance = 90;
		ent->client->ps.fov = 90;
	}
	
	if(ent->client->zc.distance < 90)
	{
		gi.sound (ent, CHAN_AUTO, gi.soundindex("3zb/zoom.wav"), 1, ATTN_NORM, 0);
		if(ent->client->zc.distance == 15 ) ent->client->zc.distance = 40;
		else if(ent->client->zc.distance == 40 ) ent->client->zc.distance = 65;
		else ent->client->zc.distance = 90;		
		ent->client->ps.fov = ent->client->zc.distance;
	}
}

void Cmd_AutoZoom (edict_t *ent)
{
	if( ent->client->zc.autozoom )
	{
		gi.cprintf(ent,PRINT_HIGH,"autozoom off.\n");
		ent->client->zc.autozoom = false;
	}
	else
	{
		gi.cprintf(ent,PRINT_HIGH,"autozoom on.\n");
		ent->client->zc.autozoom = true;
	}
}

//chain �� undo
void UndoChain (edict_t *ent ,int step)
{
	int	count,i;
	trace_t	rs_trace;

	if(step < 2) count = 2;
	else count = step;

	if(chedit->value && !ent->deadflag && ent == &g_edicts[1])
	{
		for(i = CurrentIndex - 1;i > 0 ;i--)
		{
			if(Route[i].state == GRS_NORMAL)
			{
				rs_trace = gi.trace(Route[i].Pt,ent->mins,ent->maxs,Route[i].Pt,ent,MASK_BOTSOLID);

				if(--count <= 0 && !rs_trace.allsolid && !rs_trace.startsolid) break;
			}  
		}

		gi.cprintf(ent,PRINT_HIGH,"backed %i %i steps.\n",CurrentIndex - i,step);
		CurrentIndex = i;
		VectorCopy(Route[CurrentIndex].Pt,ent->s.origin);
		VectorCopy(Route[CurrentIndex].Pt,ent->s.old_origin);

		memset(&Route[CurrentIndex],0,sizeof(route_t));
		if(CurrentIndex > 0) Route[CurrentIndex].index = Route[CurrentIndex - 1].index + 1; 
	}
}

// From Yamagi Q2
static int get_ammo_usage(gitem_t *weap)
{
	if (!weap) {
		return 0;
	}

	// handles grenades and tesla which only use 1 ammo per shot
	// have to check this because they don't store their ammo usage in weap->quantity
	if (weap->flags & IT_AMMO) {
		return 1;
	}

	// weapons store their ammo usage in the quantity field
	return weap->quantity;
}

static gitem_t *cycle_weapon (edict_t *ent)
{
	gclient_t	*cl = NULL;
	gitem_t		*noammo_fallback = NULL;
	gitem_t		*noweap_fallback = NULL;
	gitem_t		*weap = NULL;
	gitem_t		*ammo = NULL;
	int			i;
	int			start;
	int			num_weaps;
	const char	*weapname = NULL;

	if (!ent) {
		return NULL;
	}

	cl = ent->client;

	if (!cl) {
		return NULL;
	}

	num_weaps = gi.argc();

	// find where we want to start the search for the next eligible weapon
	if (cl->newweapon) {
		weapname = cl->newweapon->classname;
	}
	else if (cl->pers.weapon) {
		weapname = cl->pers.weapon->classname;
	}

	if (weapname)
	{
		for (i = 1; i < num_weaps; i++) {
			if (Q_stricmp((char *)weapname, gi.argv(i)) == 0) {
				break;
			}
		}
		i++;

		if (i >= num_weaps) {
			i = 1;
		}
	}
	else {
		i = 1;
	}

	start = i;
	noammo_fallback = NULL;
	noweap_fallback = NULL;

	// find the first eligible weapon in the list we can switch to
	do
	{
		weap = FindItemByClassname(gi.argv(i));

		if (weap && weap != cl->pers.weapon && (weap->flags & IT_WEAPON) && weap->use)
		{
			if (cl->pers.inventory[ITEM_INDEX(weap)] > 0)
			{
				if (weap->ammo)
				{
					ammo = FindItem(weap->ammo);
					if (ammo)
					{
						if (cl->pers.inventory[ITEM_INDEX(ammo)] >= get_ammo_usage(weap)) {
							return weap;
						}
						if (!noammo_fallback) {
							noammo_fallback = weap;
						}
					}
				}
				else {
					return weap;
				}
			}
			else if (!noweap_fallback) {
				noweap_fallback = weap;
			}
		}

		i++;

		if (i >= num_weaps) {
			i = 1;
		}
	}
	while (i != start);

	// if no weapon was found, the fallbacks will be used for
	// printing the appropriate error message to the console
	if (noammo_fallback) {
		return noammo_fallback;
	}

	return noweap_fallback;
}

void Cmd_CycleWeap_f (edict_t *ent)
{
	gitem_t		*weap = NULL;

	if (!ent) {
		return;
	}

	if (gi.argc() <= 1) {
		gi.cprintf(ent, PRINT_HIGH, "Usage: cycleweap classname1 classname2 .. classnameN\n");
		return;
	}

	weap = cycle_weapon(ent);
	if (weap)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(weap)] <= 0) {
			gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", weap->pickup_name);
		}
		else {
			weap->use(ent, weap);
		}
	}
}

static gitem_t *preferred_weapon (edict_t *ent)
{
	gclient_t	*cl = NULL;
	gitem_t		*noammo_fallback = NULL;
	gitem_t		*noweap_fallback = NULL;
	gitem_t		*weap = NULL;
	gitem_t		*ammo = NULL;
	int			i;
	int			num_weaps;

	if (!ent) {
		return NULL;
	}

	cl = ent->client;

	if (!cl) {
		return NULL;
	}

	num_weaps = gi.argc();

	// find the first eligible weapon in the list we can switch to
	for (i = 1; i < num_weaps; i++)
	{
		weap = FindItemByClassname(gi.argv(i));

		if (weap && (weap->flags & IT_WEAPON) && weap->use)
		{
			if (cl->pers.inventory[ITEM_INDEX(weap)] > 0)
			{
				if (weap->ammo)
				{
					ammo = FindItem(weap->ammo);
					if (ammo)
					{
						if (cl->pers.inventory[ITEM_INDEX(ammo)] >= get_ammo_usage(weap)) {
							return weap;
						}

						if (!noammo_fallback) {
							noammo_fallback = weap;
						}
					}
				}
				else {
					return weap;
				}
			}
			else if (!noweap_fallback) {
				noweap_fallback = weap;
			}
		}
	}

	// If no weapon was found, the fallbacks will be used for
	// printing the appropriate error message to the console
	if (noammo_fallback) {
		return noammo_fallback;
	}

	return noweap_fallback;
}


void Cmd_PrefWeap_f (edict_t *ent)
{
	gitem_t *weap;

	if (!ent) {
		return;
	}

	if (gi.argc() <= 1) {
		gi.cprintf(ent, PRINT_HIGH, "Usage: prefweap classname1 classname2 .. classnameN\n");
		return;
	}

	weap = preferred_weapon (ent);
	if (weap)
	{
		if (ent->client->pers.inventory[ITEM_INDEX(weap)] <= 0) {
			gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", weap->pickup_name);
		}
		else {
			weap->use(ent, weap);
		}
	}
}
// end from Yamagi Q2

/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{	// Fix by Bouman- use CTF Say
		if (ctf->value)
			CTFSay_Team (ent, gi.args());
		else
			Cmd_Say_f (ent, true, false);
		// end Bouman fix
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_stricmp (cmd, "zoomin") == 0)		//zoom
		Cmd_ZoomIn(ent);
	else if (Q_stricmp (cmd, "zoomout") == 0)
		Cmd_ZoomOut(ent);
	else if (Q_stricmp (cmd, "autozoom") == 0)
		Cmd_AutoZoom(ent);
	else if (Q_stricmp (cmd, "air") == 0)
		Cmd_AirStrike(ent);
	else if (Q_stricmp (cmd, "undo") == 0)
	{
		if (gi.argc() <= 1)
			UndoChain (ent, 1);
		else
			UndoChain (ent, atoi(gi.argv(1)));
	}
//ZOID
	else if (Q_stricmp (cmd, "team") == 0)
	{
		CTFTeam_f (ent);
	} else if (Q_stricmp(cmd, "id") == 0) {
		CTFID_f (ent);
	}
//	else if (Q_stricmp(cmd, "techcount") == 0)
//		Cmd_TechCount_f (ent);
//ZOID
	// from Yamagi Q2
	else if (!Q_stricmp(cmd, "cycleweap"))
	{
		Cmd_CycleWeap_f (ent);
	}
	else if (!Q_stricmp(cmd, "prefweap"))
	{
		Cmd_PrefWeap_f (ent);
	}
	// end from Yamagi Q2

	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
