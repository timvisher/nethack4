#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "nethack.h"
#include "wintty.h"

struct nh_option_desc tty_options[] = {
    {NULL, NULL, OPTTYPE_BOOL, { NULL }}
};

struct nh_boolopt_map boolopt_map[] = {
    {NULL, NULL}
};


boolean option_change_callback(struct nh_option_desc *option)
{
	return FALSE;
}


void tty_init_options(void)
{
	nh_setup_ui_options(tty_options, boolopt_map, option_change_callback);
}


static char* get_option_string(struct nh_option_desc *option, char *valbuf)
{
	char *valstr;
	int i;
	
	switch (option->type) {
	    case OPTTYPE_BOOL:
		valstr = option->value.b ? "true" : "false";
		break;
		
	    case OPTTYPE_ENUM:
		valstr = "(invalid)";
		for (i = 0; i < option->e.numchoices; i++)
		    if (option->value.e == option->e.choices[i].id)
			valstr =  option->e.choices[i].caption;
		break;
		
	    case OPTTYPE_INT:
		sprintf(valbuf, "%d", option->value.i);
		valstr = valbuf;
		break;
		
	    case OPTTYPE_STRING:
		if (!option->value.s)
		    valstr = "";
		else
		    valstr = option->value.s;
		break;
	}
	return valstr;
}


/* add a list of options to the given selection menu */
static int menu_add_options(winid window, int firstid,
			    struct nh_option_desc *options, boolean read_only)
{
	int i;
	char *valstr = NULL, valbuf[8], optbuf[128];
	const char *opttxt;
	anything any;
	
	any.a_void = NULL;
	for (i = 0; options[i].name; i++) {
	    any.a_int = (read_only) ? 0 : firstid + i;
	    valstr = get_option_string(&options[i], valbuf);
	    
	    opttxt = options[i].helptxt;
	    if (!opttxt || strlen(opttxt) < 2)
		opttxt = options[i].name;
	    
	    sprintf(optbuf, "%-60.60s [%s]", opttxt, valstr);
	    tty_add_menu(window, NO_GLYPH, &any, 0, 0, 0,
		 optbuf, MENU_UNSELECTED);
	}
	
	/* add an empty line */
	any.a_void = NULL;
	tty_add_menu(window, NO_GLYPH, &any, 0, 0, 0, "", MENU_UNSELECTED);
	
	return i + firstid;
}


/* display a selection menu for enum options */
static void select_enum_value(union nh_optvalue *value, struct nh_option_desc *option)
{
	winid window;
	anything any;
	menu_item *selected;
	int i, n, selectidx;
	
	any.a_void = NULL;
	window = tty_create_nhwindow(NHW_MENU);
	tty_start_menu(window);
	
	for (i = 0; i < option->e.numchoices; i++) {
	    /* don't use the choice ids directly, 0 is a valid value for those */
	    any.a_int = i+1;
	    tty_add_menu(window, NO_GLYPH, &any, 0, 0, 0,
		 option->e.choices[i].caption, MENU_UNSELECTED);
	}
	tty_end_menu(window, option->name);
	n = tty_select_menu(window, PICK_ONE, &selected);
	
	value->e = option->value.e; /* in case of ESC */
	if (n == 1) {
	    selectidx = selected[0].item.a_int - 1;
	    value->e = option->e.choices[selectidx].id;
	}
	tty_destroy_nhwindow(window);
}


/* get a new value of the appropriate type for the given option */
static void get_option_value(union nh_optvalue *value, struct nh_option_desc *option)
{
	char buf[BUFSZ], query[BUFSZ];
	
	switch (option->type) {
	    case OPTTYPE_BOOL:
		value->b = !option->value.b;
		break;
		
	    case OPTTYPE_INT:
		sprintf(query, "New value for %s (number)", option->name);
		sprintf(buf, "%d", value->i);
		tty_getlin(query, buf);
		sscanf(buf, "%d", &value->i);
		break;
		
	    case OPTTYPE_ENUM:
		select_enum_value(value, option);
		break;
		
	    case OPTTYPE_STRING:
		sprintf(query, "New value for %s (text)", option->name);
		tty_getlin(query, value->s);
		break;
	}
}


/* display the option dialog */
void display_options(boolean change_birth_opt)
{
	winid tmpwin;
	anything any;
	int gameoptidx, birthoptidx, ttyoptidx, pick_cnt, i;
	menu_item *pick_list;
	struct nh_option_desc *nhoptions = nh_get_options(FALSE);
	struct nh_option_desc *birthoptions = nh_get_options(TRUE);
	struct nh_option_desc *option = NULL;
	union nh_optvalue value;
	char strbuf[BUFSZ];
	
	tmpwin = tty_create_nhwindow(NHW_MENU);
	tty_start_menu(tmpwin);
	
	/* add general game options */
	gameoptidx = 1;
	any.a_void = NULL;
	tty_add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
		 "Game options:", MENU_UNSELECTED);
	birthoptidx = menu_add_options(tmpwin, gameoptidx, nhoptions, FALSE);
	
	/* add or display birth options */
	tty_add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
		 "Birth options:", MENU_UNSELECTED);
	ttyoptidx = menu_add_options(tmpwin, gameoptidx, birthoptions, !change_birth_opt);
	
	/* add tty-specific options */
	tty_add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
		 "TTY interface options:", MENU_UNSELECTED);
	menu_add_options(tmpwin, gameoptidx, tty_options, FALSE);
	
	tty_end_menu(tmpwin, "Set what options?");
	
	pick_cnt = tty_select_menu(tmpwin, PICK_ANY, &pick_list);
	if (pick_cnt <= 0)
	    return;
	
	value.s = strbuf;
	for (i = 0; i < pick_cnt; ++i) {
	    int idx = pick_list[i].item.a_int;
	    
	    if (idx < birthoptidx)
		option = &nhoptions[idx - gameoptidx];
		
	    else if (idx < ttyoptidx)
		option = &birthoptions[idx - birthoptidx];
		
	    else
		option = &tty_options[idx - ttyoptidx];
	    
	    get_option_value(&value, option);
	    if (!nh_set_option(option->name, value, FALSE))
		pline("new value for %s rejected", option->name);
	}
	tty_destroy_nhwindow(tmpwin);
	
	write_config();
}


/* parse a single line from the config file and set the option */
static void read_config_line(char* line)
{
	char *comment, *delim, *name, *value;
	union nh_optvalue optval;

	comment = index(line, '#');
	if (comment)
	    comment = '\0';
	delim = index(line, '=');
	if (!delim)
	    return; /* could whine about junk chars in the config, but why bother */
	
	name = line;
	value = delim + 1;
	*delim-- = '\0';
	
	/* remove space around name */
	while (isspace(*name))
	    name++;
	while (isspace(*delim))
	    *delim-- = '\0';
	
	/* remove spaces around value */
	delim = value;
	while (*delim)
	    delim++;
	delim--;
	
	while (isspace(*value))
	    value++;
	while (isspace(*delim))
	    *delim-- = '\0';
	
	/* value may be enclosed with double quotes (") */
	if (*value == '"' && *delim == '"') {
	    value++;
	    *delim ='\0';
	}
	
	optval.s = value;
	nh_set_option(name, optval, TRUE);
}


/* open a config file and separate it into lines for read_config_line() */
static void read_config_file(const char *filename)
{
	FILE *fp;
	int fsize;
	char *buf, *line;
	
	fp = fopen(filename, "rb");
	if (!fp)
		return;

	/* obtain file size. */
	fseek(fp , 0 , SEEK_END);
	fsize = ftell(fp);
	rewind(fp);

	buf = malloc(fsize+1);
	if (!buf)
		return;

	fread(buf, fsize, 1, fp);
	fclose(fp);
	
	buf[fsize] = '\0';
	
	/* each option is expected to have the following format:
	 * name=value\n
	 */
	line = strtok(buf, "\n");
	do {
	    read_config_line(line);
	    
	    line = strtok(NULL, "\n");
	} while (line);
	
	free(buf);
}


/* determine the correct filename for the config file */
static void get_config_name(char *buf, boolean ui)
{
	char *envval;
	buf[0] = '\0';
	
	if (!ui) {
	    /* check for env override first */
	    envval = getenv("NETHACKOPTIONS");
	    if (envval) {
		strncpy(buf, envval, BUFSZ);
		return;
	    }
	}
	
	/* look in regular location */
	envval = getenv("XDG_CONFIG_HOME");
	if (envval)
	    snprintf(buf, BUFSZ, "%s/NetHack/%s", envval, ui ? "tty.conf" : "NetHack.conf");
	else {
	    envval = getenv("HOME");
	    if (!envval) /* HOME not set? just give up... */
		return;
	    snprintf(buf, BUFSZ, "%s/.config/NetHack/%s", envval,
		     ui ? "tty.conf" : "NetHack.conf");
	}
}


void read_config(void)
{
#if defined(UNIX)
	char filename[BUFSZ];
	char uiconfname[BUFSZ];
	
	get_config_name(filename, FALSE);
	get_config_name(uiconfname, TRUE);
	
	read_config_file(filename);
	read_config_file(uiconfname);
	
#elif defined(WIN32)
	TCHAR szPath[MAX_PATH];
	/* get the application data directory: 
	 * C:\Users\somename\AppData\roaming\ on Vista and 7 */
	if (!SUCCEEDED(SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, szPath )))
	    return;
	PathAppend( szPath, _T("\\NetHack\\NetHack.conf") );
#endif
}


static void make_config_dir(char *filename)
{
	char filename_copy[BUFSZ];
	char *dir;
	
	/* dirname may modify its argument */
	strncpy(filename_copy, filename, BUFSZ);
	dir = dirname(filename_copy);
	
	if (mkdir(dir, 0755) == -1) {
	    /* couldn't create last level dir; try creating 2 levels */
	    dir = dirname(dir);
	    mkdir(dir, 0755);
	    
	    strncpy(filename_copy, filename, BUFSZ);
	    dir = dirname(filename_copy);
	    mkdir(dir, 0755);
	}
}


static FILE *open_config_file(char *filename)
{
	FILE *fp;
	
	fp = fopen(filename, "w");
	if (!fp && (errno == ENOTDIR || errno == ENOENT)) {
	    make_config_dir(filename);
	    fp = fopen(filename, "w");
	}
	
	if (!fp) {
	    fprintf(stderr, "could not open %s: %s", filename, strerror(errno));
	    return NULL;
	}
	
	fprintf(fp, "# note: this file is rewritten whenever options are changed ingame\n");
	
	return fp;
}


static void write_config_options(FILE *fp, struct nh_option_desc *options)
{
	int i;
	char workbuf[8];
	char *optval;
	
	for (i = 0; options[i].name; i++) {
	    optval = get_option_string(&options[i], workbuf);
	    if (options[i].type == OPTTYPE_STRING ||
		options[i].type == OPTTYPE_ENUM)
		fprintf(fp, "%s=\"%s\"\n", options[i].name, optval);
	    else
		fprintf(fp, "%s=%s\n", options[i].name, optval);
	}
}


void write_config(void)
{
	FILE *fp;
	char filename[BUFSZ];
	char uiconfname[BUFSZ];
	
	get_config_name(filename, FALSE);
	get_config_name(uiconfname, TRUE);
	
	fp = open_config_file(filename);
	if (fp) {
	    write_config_options(fp, nh_get_options(FALSE));
	    write_config_options(fp, nh_get_options(TRUE));
	    fclose(fp);
	}
	
	fp = open_config_file(uiconfname);
	if (fp) {
	    write_config_options(fp, tty_options);
	    fclose(fp);
	}
}
