#include <libdragon.h>
#include <joypad.h>

typedef struct
{
    uint32_t type;
    char filename[MAX_FILENAME_LEN+1];
} direntry_t;

static direntry_t *list;
static int listn = 0;

char dir[512] = "rom://";

enum {
	Smenu,
	Sprog,
};

static int curstate = Smenu;
static int curinit = 0;

static char *memory = NULL;
const static int memsize = 16384; /* 16KiB */

static int idx = 0;

/* selected entry */
static int selected = 0;
static int selectchanged = 1;

_Noreturn void
fatal(const char *msg)
{
	printf("[FATAL] %s\n", msg);
	while(1);
}

void
interpret(const char *code)
{
	char c;

	for(c = *code; c != 0; c = *(++code)){
		switch(c){
			case '+': memory[idx]++; break;
			case '-': memory[idx]--; break;
			case '<': idx = (idx == 0)
							? (memsize - 1)
							: idx - 1; break;
			case '>': idx = (memsize - 1)
							? 0
							: idx + 1; break;
			case '[': break; /* TODO */
			case ']': break; /* TODO */
			case '.': putchar(memory[idx]); break;
			case ',': break; /* TODO */
			default:
				continue;
		}
	}
}

void
stateinit()
{
	switch(curstate){
		case Smenu:
			/* if display already initialised */
			display_close();
			display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
			break;
		case Sprog:
			/* console init will close old display */
			console_init();
			break;
		default:
			break;
	}

	curinit = 1;
}

void
drawentry(surface_t *dc, int i, char *name)
{
		graphics_draw_box(dc, 20, 20 + (i * 10), 280, 10,
			(i != selected)
				? graphics_make_color(65, 7, 92, 255)
				: graphics_make_color(141, 54, 181, 255)
		);

		graphics_draw_text(dc, 21, 21 + (i * 10), name);
}

direntry_t *
populate_dir(int *count)
{
    /* Grab a slot */
    *count = 1;

    /* Grab first */
    dir_t buf;
	direntry_t *list = malloc(sizeof(*list));
    int ret = dir_findfirst(dir, &buf);

    if( ret != 0 ) 
    {
        /* Free stuff */
        free(list);
        *count = 0;

        /* Dir was bad! */
        return 0;
    }

    /* Copy in loop */
    while( ret == 0 )
    {
        list[(*count)-1].type = buf.d_type;
        strcpy(list[(*count)-1].filename, buf.d_name);

        /* Grab next */
        ret = dir_findnext(dir,&buf);

        if( ret == 0 )
        {
            (*count)++;
            list = realloc(list, sizeof(direntry_t) * (*count));
        }
    }

/*	if(*count > 0)
    {
        / Should sort! /
        qsort(list, *count, sizeof(direntry_t), compare);
    }*/

    return list;
}

void
render()
{
	int i;
	surface_t *dc;

	if(curinit == 0)
		stateinit();

	/* console will draw itself */
	if(curstate != Smenu)
		return;

	/* render menu */

	if(selectchanged == 0)
		return;

	dc = display_get();

	selectchanged = 0;

	graphics_fill_screen(dc, 0);
	graphics_draw_box(dc, 20, 20, 280, 200, graphics_make_color(65, 7, 92, 255));

	/* draw entries */
	for(i = 0; i < listn; i++){
		drawentry(dc, i, list[i].filename);
	}

	display_show(dc);
}

void
readinput()
{
	joypad_buttons_t btn;
	
	joypad_poll();
	JOYPAD_PORT_FOREACH(port){
		if(joypad_is_connected(port) == 0)
			continue;

		btn = joypad_get_buttons_pressed(port);
		if(btn.d_up){
			if(selected > 0){
				selected--;
				selectchanged = 1;
			}
		}

		if(btn.d_down){
			if(selected < (listn - 1)){
				selected++;
				selectchanged = 1;
			}
		}
	}
}

int
main(void)
{
	timer_init();
	joypad_init();
	console_init();
	
	if(dfs_init(DFS_DEFAULT_LOCATION) != DFS_ESUCCESS)
		fatal("failed to init dfs");

	list = populate_dir(&listn);
	if(list == NULL)
		fatal("failed reading root dir");

	while(1){
		render();
		readinput();
	}

	return 0;
}
