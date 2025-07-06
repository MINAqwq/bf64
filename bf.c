#include <libdragon.h>
#include <joypad.h>

typedef struct
{
    uint32_t type;
    char filename[MAX_FILENAME_LEN+1];
} direntry_t;

static direntry_t *list;
static int listn = 0;

char dir[512] = "rom://bf/";

enum {
	Smenu,
	Sprog,
};

static wav64_t wav;

static int curstate = Smenu;
static int curinit = 0;
static int progend = 0;

static char *mem = NULL;
const static int memsize = 16384; /* 16KiB */

static int *stack = NULL;
static int stacksize = 0;

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
stackpush(int addr)
{
	if(stacksize == 0){
		stack = malloc(sizeof(int));
		if(stack == NULL)
			fatal("memory allocation failed");
	}else{
		stack = realloc(stack, (stacksize + 1) * sizeof(int));
	}

	stack[stacksize] = addr;

	stacksize++;
}

int
stackpop(int rm)
{
	int addr;

	if(stacksize == 0)
		fatal("tried to pop on empty stack");

	addr = rm
			? stack[--stacksize]
			: stack[stacksize - 1];

	if(stacksize != 0)
		stack = realloc(stack, stacksize * sizeof(int));
	else
		free(stack);

	return addr;
}

void
skiploop(FILE *code)
{
	char ch;
	int c;
	int l;

	l = 0;
	do{
		c = fgetc(code);
		ch = (char)c;

		if(ch == '[')
			l++;

		if(ch == ']'){
			if(l != 0)
				l--;
			else
				break;
		}
	}while(c != EOF);
}

char
getinput()
{
	joypad_buttons_t btn;

	while(1){
		joypad_poll();
		JOYPAD_PORT_FOREACH(port){
			btn = joypad_get_buttons_pressed(port);
			if(btn.start)	return  0;
			if(btn.a)		return  1;
			if(btn.b)		return  2;
			if(btn.z)		return  3;
			if(btn.d_up)	return  4;
			if(btn.d_down)	return  5;
			if(btn.d_left)	return  6;
			if(btn.d_right)	return  7;
			if(btn.x)		return  8;
			if(btn.y)		return  9;
			if(btn.l)		return 10;
			if(btn.r)		return 11;
			if(btn.c_up)	return 12;
			if(btn.c_down)	return 13;
			if(btn.c_left)	return 14;
			if(btn.c_right)	return 15;
		}
	}
}

void
musicplay()
{
	if(!audio_can_write())
		return;

	mixer_poll(audio_write_begin(), audio_get_buffer_length());
	audio_write_end();
}

void
interpret(FILE *code)
{
	int in;
	char c;

	memset(mem, 0, memsize);
	for(in = fgetc(code); in != EOF; in = fgetc(code)){
		/* is this hack? i think so */
		musicplay();

		c = (char)in;
		switch(c){
			case '+': mem[idx]++; break;
			case '-': mem[idx]--; break;
			case '<': idx = (idx == 0)
							? (memsize - 1)
							: idx - 1; break;
			case '>': idx = (idx == (memsize - 1))
							? 0
							: idx + 1; break;
			case '[':
				if(mem[idx] == 0){
					skiploop(code);
					break;
				}

				stackpush(ftell(code));
				break;
			case ']':
				if(mem[idx] == 0){
					(void)stackpop(1);
					break;
				}

				fseek(code, stackpop(0), SEEK_SET); break;
			case '.': putchar(mem[idx]); break;
			case ',': mem[idx] = getinput(); break;
			case 'L': printf("%d\n", idx); break;
			default:
				continue;
		}
	}
}

int
interpretfile(int idx)
{
	int ret;
	FILE *fp;
	int size;
	char path[MAX_FILENAME_LEN + 10];

	strcpy(path, "rom://bf/");
	strncpy(path + 9, list[idx].filename, MAX_FILENAME_LEN + 1);

	printf("running %s...\n", path);

	fp = fopen(path, "r");
	if(!fp)
		return 1;

	ret = 1;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	if(size == 0){
		printf("error: file empty\n");
		goto close;
	}

	interpret(fp);

	ret = 0;

close:
	fclose(fp);
	return ret;
}

void
musicinit()
{
	wav64_open(&wav, "rom:/audio/menu.wav64");
	wav64_play(&wav, 0);
}

void
stateinit()
{
	switch(curstate){
		case Smenu:
			/* if display already initialised */
			display_close();
			display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
			selectchanged = 1;
			break;
		case Sprog:
			/* console init will close old display */
			console_init();
			progend = 1;
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

    if(ret != 0) {
        /* Free stuff */
        free(list);
        *count = 0;

        /* Dir was bad! */
        return 0;
    }

    /* Copy in loop */
    while(ret == 0){
        list[(*count)-1].type = buf.d_type;
        strcpy(list[(*count)-1].filename, buf.d_name);

        /* Grab next */
        ret = dir_findnext(dir,&buf);

        if(ret == 0){
            (*count)++;
            list = realloc(list, sizeof(direntry_t) * (*count));
        }
    }

/*	if(*count > 0){
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
statechange(int s)
{
	curstate = s;
	curinit = 0;
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

		if(btn.a)
			statechange(Sprog);

		if(btn.b && curstate == Sprog)
			statechange(Smenu);
	}
}

int
main(void)
{
	timer_init();
	joypad_init();
	audio_init(44100, 128);
	mixer_init(32);
	mixer_ch_set_limits(6, 0, 128000, 0);

	console_init();
	
	if(dfs_init(DFS_DEFAULT_LOCATION) != DFS_ESUCCESS)
		fatal("failed to init dfs");

	list = populate_dir(&listn);
	if(list == NULL)
		fatal("failed reading root dir");

	mem = malloc(memsize);
	if(mem == NULL)
		fatal("buffer allocation failed");
	
	musicinit();
	while(1){
		musicplay();
		render();
		readinput();

		if(progend == 1){
			interpretfile(selected);
			progend = 0;
		}
	}

	return 0;
}
