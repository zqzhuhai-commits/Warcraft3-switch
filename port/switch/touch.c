/* Direct Horizon touch -> SDL mouse. Independent of game/UI state. */
#include <switch.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "touch.h"

#define NX_TAP_SLOP 8 /* logical pixels; jitter before a selection drag */
#define NX_RIGHT_DELAY 500 /* milliseconds; stationary hold becomes right click */
static struct {
    bool active;
    unsigned finger;
    int start_x,start_y,x,y,button;
    Uint32 started;
} touch;

/* Do not let SDL's backend synthesize a second click or two-finger camera pan. */
static int NX_FilterTouch(void *unused, SDL_Event *ev) {
    (void)unused;
    if (ev->type==SDL_FINGERDOWN || ev->type==SDL_FINGERMOTION || ev->type==SDL_FINGERUP) return 0;
    Uint32 *which=NULL;
    if(ev->type==SDL_MOUSEMOTION) which=&ev->motion.which;
    if(ev->type==SDL_MOUSEBUTTONDOWN || ev->type==SDL_MOUSEBUTTONUP) which=&ev->button.which;
    if(which && *which==SDL_TOUCH_MOUSEID) return 0;
    if(which && *which==NX_TOUCH_MOUSEID) *which=SDL_TOUCH_MOUSEID;
    return 1;
}

void NX_InitInput(void) {
    hidInitializeTouchScreen();
    SDL_InitSubSystem(SDL_INIT_EVENTS);
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS,"0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS,"0");
    SDL_SetEventFilter(NX_FilterTouch,NULL);
}

static void NX_TouchMotion(SDL_Window *win,int x,int y) {
    SDL_Event ev={0};ev.type=SDL_MOUSEMOTION;
    ev.motion.timestamp=SDL_GetTicks();ev.motion.which=NX_TOUCH_MOUSEID;
    ev.motion.windowID=win ? SDL_GetWindowID(win) : 0;
    ev.motion.x=x;ev.motion.y=y;
    ev.motion.xrel=x-touch.x;ev.motion.yrel=y-touch.y;
    ev.motion.state=touch.button ? SDL_BUTTON(touch.button) : 0;
    SDL_PushEvent(&ev);touch.x=x;touch.y=y;
    NX_TouchPointer(x,y);
}

void NX_TouchCancel(void) {
    if(touch.button) NX_MouseSource(1,touch.button,false,touch.x,touch.y);
    touch.active=false;touch.button=0;
}

void NX_TouchFrame(SDL_Window *win,Uint32 now,int w,int h) {
    /* SDL_Quit during video restart clears the global filter. Restore it once. */
    SDL_EventFilter filter;void *userdata;
    if(!SDL_GetEventFilter(&filter,&userdata) || filter!=NX_FilterTouch)
        SDL_SetEventFilter(NX_FilterTouch,NULL);
    HidTouchScreenState sample;
    if(!hidGetTouchScreenStates(&sample,1) || w<=0 || h<=0) return;
    if(sample.count<=0) {
        if(touch.active && !touch.button) {
            NX_MouseSource(1,SDL_BUTTON_LEFT,true,touch.x,touch.y);
            NX_MouseSource(1,SDL_BUTTON_LEFT,false,touch.x,touch.y);
        }
        NX_TouchCancel();return;
    }
    HidTouchState *finger=&sample.touches[0];
    if(touch.active) {
        for(int i=0;i<sample.count && i<16;i++)
            if(sample.touches[i].finger_id==touch.finger) {finger=&sample.touches[i];break;}
    }
    int x=(int)((u64)finger->x*w/1280),y=(int)((u64)finger->y*h/720);
    x=SDL_max(0,SDL_min(w-1,x));y=SDL_max(0,SDL_min(h-1,y));
    if(!touch.active) {
        touch.active=true;touch.finger=finger->finger_id;touch.started=now;
        touch.start_x=touch.x=x;touch.start_y=touch.y=y;
        NX_TouchMotion(win,x,y);
    }
    int dx=x-touch.start_x,dy=y-touch.start_y;
    if(!touch.button) {
        if(sample.count>=2 || (now-touch.started>=NX_RIGHT_DELAY && dx*dx+dy*dy<=NX_TAP_SLOP*NX_TAP_SLOP)) {
            touch.button=SDL_BUTTON_RIGHT;
            NX_MouseSource(1,touch.button,true,x,y);
        } else if(dx*dx+dy*dy>NX_TAP_SLOP*NX_TAP_SLOP) {
            touch.button=SDL_BUTTON_LEFT;
            NX_MouseSource(1,touch.button,true,touch.start_x,touch.start_y);
        }
    }
    if(x!=touch.x || y!=touch.y) NX_TouchMotion(win,x,y);
}
