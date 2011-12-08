/* Copyright (c) Daniel Thaler, 2011.                             */
/* NetHack may be freely redistributed.  See license for details. */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "nhcurses.h"

static void dummy_update_screen(struct nh_dbuf_entry dbuf[ROWNO][COLNO]) {}
static void dummy_delay_output(void) {}

static struct nh_window_procs curses_replay_windowprocs = {
    curses_pause,
    curses_display_buffer,
    curses_update_status_silent,
    curses_print_message_nonblocking,
    NULL,
    NULL,
    curses_list_items_nonblocking,
    dummy_update_screen,
    curses_raw_print,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    dummy_delay_output,
    curses_notify_level_changed,
    curses_outrip,
};


static void draw_replay_info(struct nh_replay_info *rinfo)
{
    char buf[BUFSZ];
    
    sprintf(buf, "REPLAY action %d/%d", rinfo->actions, rinfo->max_actions);
    if (*rinfo->nextcmd)
	sprintf(buf + strlen(buf), "; next command: %s.", rinfo->nextcmd);
    
    mvwhline(stdscr, 2 + ui_flags.msgheight + ROWNO, 1, ACS_HLINE, COLNO);
    wattron(stdscr, COLOR_PAIR(4) | A_BOLD);
    mvwaddstr(stdscr, 2 + ui_flags.msgheight + ROWNO, 2, buf);
    wattroff(stdscr, COLOR_PAIR(4) | A_BOLD);
    wrefresh(stdscr);
}


static void show_replay_help()
{
    static struct nh_menuitem items[] = {
	{0, MI_TEXT, "KEY_RIGHT or SPACE\t- advance one move"},
	{0, MI_TEXT, "KEY_LEFT\t- go back one move"},
	{0, MI_TEXT, "g\t- go to any move"},
	{0, MI_TEXT, "ESC\t- leave the replay"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "Informational game commands like \"inventory\" or \"discoveries\" will also work"}
    };
    curses_display_menu(items, 6, "Replay help:", PICK_NONE, NULL);
}


static void replay_commandloop(int fd)
{
    int key, move, count;
    char buf[BUFSZ], qbuf[BUFSZ];
    boolean ret, firsttime = TRUE;
    struct nh_replay_info rinfo;
    struct nh_cmd_arg noarg;
    struct nh_cmd_desc *cmd;
    
    create_game_windows();
    nh_view_replay_start(fd, &curses_replay_windowprocs, &rinfo);
    load_keymap();
    
    while (1) {
	draw_msgwin();
	curses_update_status(NULL);
	draw_sidebar();
	draw_replay_info(&rinfo);
	if (firsttime)
	    show_replay_help();
	firsttime = FALSE;

	key = get_cmdkey();
	switch (key) {
	    /* step forward */
	    case KEY_RIGHT:
	    case ' ':
		ret = nh_view_replay_step(&rinfo, REPLAY_FORWARD, 1);
		draw_replay_info(&rinfo);
		if (ret == FALSE) {
		    key = curses_msgwin("You have reached the end of this game. "
		                        "Go back or press ESC to exit.");
		    if (key == KEY_ESC)
			goto out;
		}
		break;
		
	    /* step backward */
	    case KEY_LEFT:
		nh_view_replay_step(&rinfo, REPLAY_BACKWARD, 1);
		draw_replay_info(&rinfo);
		break;
		
	    case KEY_ESC:
		goto out;

	    case 'g':
		strncpy(qbuf, "What move do you want to jump to?", BUFSZ);
		if (rinfo.max_moves > 0)
		    sprintf(qbuf + strlen(qbuf), " (Max: %d)", rinfo.max_moves);
		
		curses_getline(qbuf, buf);
		if (buf[0] == '\033' || !(move = atoi(buf)))
		    break;
		nh_view_replay_step(&rinfo, REPLAY_GOTO, move);
		break;
		
	    default:
		count = 0;
		noarg.argtype = CMD_ARG_NONE;
		cmd = keymap[key];
		if (!cmd)
		    break;
		if (cmd->flags & CMD_UI)
		    handle_internal_cmd(&cmd, &noarg, &count);
		if (cmd)
		    nh_command(cmd->name, count, &noarg);
		break;
	}
    }
    
out:
    nh_view_replay_finish();
    free_keymap();
    destroy_game_windows();
    cleanup_messages();
}


void replay(void)
{
    char buf[BUFSZ], logdir[BUFSZ], savedir[BUFSZ], filename[1024], *dir, **files;
    struct nh_menuitem *items;
    int i, n, fd, icount, size, filecount, pick[1];
    enum nh_log_status status;
    struct nh_game_info gi;
    
    if (!get_gamedir(LOG_DIR, logdir))	logdir[0] = '\0';
    if (!get_gamedir(SAVE_DIR, savedir))savedir[0] = '\0';
    
    if (*logdir)	dir = logdir;
    else if (*savedir)	dir = savedir;
    else {
	curses_msgwin("There are no games to replay.");
	return;
    }
    
    while (1) {
	filename[0] = '\0';
	files = list_gamefiles(dir, &filecount);
	/* make sure there are some files to show */
	if (!filecount) {
	    if (dir == savedir) {
		curses_msgwin("There are no saved games to replay.");
		savedir[0] = '\0';
	    } else {
		curses_msgwin("There are no completed games to replay.");
		logdir[0] = '\0';
	    }
	    
	    dir = (dir == savedir) ? logdir : savedir;
	    if (!*dir) return;
	    continue;
	}
	
	icount = 0;
	size = filecount + 2;
	items = malloc(size * sizeof(struct nh_menuitem));
	
	/* add all the files to the menu */
	for (i = 0; i < filecount; i++) {
	    fd = open(files[i], O_RDWR, 0660);
	    status = nh_get_savegame_status(fd, &gi);
	    close(fd);
	    
	    describe_game(buf, status, &gi);
	    add_menu_item(items, size, icount,
			    (status == LS_IN_PROGRESS) ? 0 : icount + 1,
			    buf, 0, FALSE);
	}
	
	if (dir == logdir && *savedir) {
	    add_menu_item(items, size, icount, -1, "View saved games instead", 0, FALSE);
	} else if (dir == savedir && *logdir) {
	    add_menu_item(items, size, icount, -1, "View saved games instead", 0, FALSE);
	}
	
	n = curses_display_menu(items, icount, "Pick a game to view", PICK_ONE, pick);
	free(items);
	if (n > 0 && pick[0] != -1)
	    strncpy(filename, files[pick[0]-1], sizeof(filename));
	
	for (i = 0; i < filecount; i++)
	    free(files[i]);
	free(files);
	
	if (n <= 0)
	    return;
	
	if (pick[0] == -1) {
	    dir = (dir == savedir) ? logdir : savedir;
	    continue;
	}
	
	/* we have a valid filename */
	break;
    }
    
    fd = open(filename, O_RDWR, 0660);
    replay_commandloop(fd);
    close(fd);
}
