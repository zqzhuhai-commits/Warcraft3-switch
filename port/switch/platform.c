/* Horizon lifecycle and gamepad-to-SDL translation. No simulation state lives here. */
#include <switch.h>
#include <SDL2/SDL.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "touch.h"

#define NX_DEADZONE 6000 // stick units; suppress drift on Joy-Con analog axes
#define NX_CURSOR_SPEED 800.0f // pixels per second at full stick; 720p RTS pointer speed
#define NX_PRECISE_SCALE 0.25f // multiplier while L is held; fine HUD targeting
#define NX_FRAME_LIMIT 0.05f // seconds; avoid pointer jumps after suspend or blocking loads

static PadState pad;
static float cursor_x = 640, cursor_y = 360;
static bool pointer_from_touch;
void NX_TouchPointer(int x,int y) {
    cursor_x=x;cursor_y=y;pointer_from_touch=true;
}
static Uint32 previous;
static u64 last_buttons;
static bool cam_keys[4];
extern int OpenRealm_Main(int argc, char **argv);

/* Keep push-based engine input and bind dispatch unchanged. */
static void NX_Key(SDL_Keycode key, bool down, SDL_Keymod mods) {
    SDL_Event ev = {0};
    ev.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    ev.key.timestamp = SDL_GetTicks();
    ev.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    ev.key.keysym.sym = key;
    ev.key.keysym.scancode = SDL_GetScancodeFromKey(key);
    ev.key.keysym.mod = mods;
    SDL_PushEvent(&ev);
}

/* Merge controller and touchscreen ownership: either may keep a drag held. */
static unsigned mouse_sources[8];
void NX_MouseSource(unsigned source,int button,bool down,int x,int y) {
    if(source>1 || button<=0 || button>=8) return;
    unsigned before=mouse_sources[button],bit=1u<<source;
    if(down) mouse_sources[button]|=bit; else mouse_sources[button]&=~bit;
    if((before!=0)==(mouse_sources[button]!=0)) return;
    SDL_Event ev={0};
    ev.type=down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    ev.button.timestamp=SDL_GetTicks();
    ev.button.which=source==1 ? NX_TOUCH_MOUSEID : 0;
    ev.button.button=button;ev.button.state=down ? SDL_PRESSED : SDL_RELEASED;
    ev.button.clicks=1;ev.button.x=x;ev.button.y=y;
    SDL_PushEvent(&ev);
}
static void NX_Mouse(int button,bool down) {
    NX_MouseSource(0,button,down,(int)cursor_x,(int)cursor_y);
}

/* Sample once before the engine drains events. Button releases retain press bindings. */
void NX_PumpInput(void) {
    if (!appletMainLoop()) {
        NX_TouchCancel();
        SDL_Event ev = {.type = SDL_QUIT}; SDL_PushEvent(&ev); return;
    }
    padUpdate(&pad);
    u64 held = padGetButtons(&pad), changed = held ^ last_buttons;
    SDL_Keymod mods = (held & HidNpadButton_ZL) ? KMOD_LSHIFT : KMOD_NONE;
    SDL_SetModState(mods);
    HidAnalogStickState left = padGetStickPos(&pad, 0), right = padGetStickPos(&pad, 1);
    Uint32 now = SDL_GetTicks();
    float dt = previous ? fminf((now - previous) / 1000.0f, NX_FRAME_LIMIT) : 0;
    previous = now;
    SDL_Window *win = SDL_GetKeyboardFocus();
    if (win) {
        int w, h, mx, my; SDL_GetWindowSize(win, &w, &h); SDL_GetMouseState(&mx, &my);
        if(!pointer_from_touch) {cursor_x = mx; cursor_y = my;}
        float speed = NX_CURSOR_SPEED * dt * ((held & HidNpadButton_L) ? NX_PRECISE_SCALE : 1.0f);
        float x = abs(left.x) > NX_DEADZONE ? (float)left.x / 32767 : 0;
        float y = abs(left.y) > NX_DEADZONE ? (float)left.y / 32767 : 0;
        cursor_x = fmaxf(0, fminf(w - 1, cursor_x + x * speed));
        cursor_y = fmaxf(0, fminf(h - 1, cursor_y - y * speed));
        if (x || y) {
            SDL_WarpMouseInWindow(win, (int)cursor_x, (int)cursor_y);
            pointer_from_touch=false;
        }
    }
    /* A/ZR share one logical left button; releasing either cannot release the other. */
    bool lnow = (held & (HidNpadButton_A | HidNpadButton_ZR)) != 0;
    bool lwas = (last_buttons & (HidNpadButton_A | HidNpadButton_ZR)) != 0;
    if (lnow != lwas) NX_Mouse(SDL_BUTTON_LEFT, lnow);
    if (changed & HidNpadButton_R) NX_Mouse(SDL_BUTTON_RIGHT, (held & HidNpadButton_R) != 0);
    static struct {u64 bit; SDL_Keycode key;} const keys[] = {
        {HidNpadButton_B,SDLK_ESCAPE},{HidNpadButton_X,SDLK_F1},{HidNpadButton_Y,SDLK_F8},
        {HidNpadButton_Plus,SDLK_F10},{HidNpadButton_Minus,SDLK_F9},
        {HidNpadButton_Up,SDLK_PAGEUP},{HidNpadButton_Down,SDLK_PAGEDOWN},
        {HidNpadButton_Left,SDLK_TAB},{HidNpadButton_Right,SDLK_SPACE},
        {HidNpadButton_StickL,SDLK_F6},{HidNpadButton_StickR,SDLK_F12}
    };
    for (unsigned i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
        if (changed & keys[i].bit) NX_Key(keys[i].key, (held & keys[i].bit) != 0, mods);
    bool next[] = {right.x < -NX_DEADZONE,right.x > NX_DEADZONE,right.y > NX_DEADZONE,right.y < -NX_DEADZONE};
    SDL_Keycode arrows[] = {SDLK_LEFT,SDLK_RIGHT,SDLK_UP,SDLK_DOWN};
    for (int i = 0; i < 4; i++) {
        if (next[i] != cam_keys[i]) NX_Key(arrows[i], next[i], KMOD_NONE);
        cam_keys[i] = next[i];
    }
    if(win) {
        int w,h;SDL_GetWindowSize(win,&w,&h);
        NX_TouchFrame(win,now,w,h);
    } else NX_TouchCancel();
    last_buttons = held;
}

/* Match FAT case-insensitive names without assuming archive filename capitalization. */
static bool NX_HasArchive(char const *name) {
    DIR *dir = opendir("sdmc:/switch/warcraft3/data");
    if (!dir) return false;
    struct dirent *entry; bool found = false;
    while ((entry = readdir(dir))) if (!strcasecmp(entry->d_name, name)) { found = true; break; }
    closedir(dir); return found;
}

/* Display a recoverable first-run screen instead of exiting without explanation. */
static void NX_Error(char const *message) {
    consoleInit(NULL);
    printf("\x1b[2;2HOpenRealm / Warcraft III - Switch preview\n\n%s\n\n", message);
    printf("Copy your original Classic MPQ files to:\n sd:/switch/warcraft3/data/\n\nWar3.mpq + War3Local.mpq\nOptional: War3x.mpq + War3xLocal.mpq\n\nPress B to exit.\n");
    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_B) break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
}

int NX_Random(void *buf, size_t size) { randomGet(buf, size); return 0; }

/* C11 time is only used by the optional engine test command. */
int timespec_get(struct timespec *ts, int base) {
    if (base != TIME_UTC || clock_gettime(CLOCK_REALTIME, ts)) return 0;
    return base;
}

static void NX_Exit(void) { romfsExit(); }

/* Application and NRO share the same embedded defaults and SD data/save contract. */
int main(int argc, char **argv) {
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    mkdir("sdmc:/switch",0777); mkdir("sdmc:/switch/warcraft3",0777);
    mkdir("sdmc:/switch/warcraft3/data",0777); mkdir("sdmc:/switch/warcraft3/user",0777);
    if (appletGetAppletType() != AppletType_Application && appletGetAppletType() != AppletType_SystemApplication) {
        NX_Error("Full application memory is required.\nUse the NSP or title override, not Album applet mode."); return 1;
    }
    if (R_FAILED(romfsInit())) { NX_Error("Embedded defaults could not be mounted."); return 1; }
    atexit(NX_Exit);
    if (!NX_HasArchive("war3.mpq")) { NX_Error("Warcraft III data not found."); return 1; }
    freopen("sdmc:/switch/warcraft3/warcraft3.log","w",stderr); setvbuf(stderr,NULL,_IONBF,0);
    chdir("sdmc:/switch/warcraft3");
    char *args[128] = {"openrealm","-data","sdmc:/switch/warcraft3/data",NX_HasArchive("war3x.mpq") ? "-tft" : "-roc"};
    int count = 4;
    /* Preserve hbmenu arguments after defaults so explicit launch settings still work. */
    for (int i = 1; i < argc && count < 110; i++) args[count++] = argv[i];
    char *video[] = {"+set","com_maxfps","30","+set","r_vsync","1","+set","vid_native","0","+set","vid_mode","4"};
    for (unsigned i = 0; i < sizeof(video)/sizeof(video[0]); i++) args[count++] = video[i];
    args[count] = NULL;
    NX_InitInput();
    return OpenRealm_Main(count,args);
}
