/* Direct Horizon touch -> SDL mouse. Independent of game/UI state. */
#include <switch.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "touch.h"

#define NX_TAP_SLOP 8 /* logical pixels; jitter before a selection drag */
#define NX_DOUBLE_DELAY 280 /* ms; singles wait so a double never selects first */
#define NX_RIGHT_DELAY 500 /* milliseconds; stationary hold becomes right click */
static struct {
    bool active, second;
    unsigned finger;
    int start_x,start_y,x,y,button;
    Uint32 started;
} touch;
static struct {bool active;int x,y;Uint32 deadline;} pending;
static struct {
    bool blocked, active;
    unsigned id[2];
    float x[2], y[2];
} gesture;

/* Only our sampled contacts may enter the engine; discard duplicate backend synthesis. */
static int NX_FilterTouch(void *unused, SDL_Event *ev) {
    (void)unused;
    if (ev->type==SDL_FINGERDOWN || ev->type==SDL_FINGERMOTION || ev->type==SDL_FINGERUP)
        return ev->tfinger.touchId==NX_TOUCH_DEVICE_ID;
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

/* The client consumes normalized finger events through its existing camera-input path. */
static void NX_Finger(unsigned i,Uint32 type,SDL_Window *win,Uint32 now) {
    SDL_Event ev={0};ev.type=type;
    ev.tfinger=(SDL_TouchFingerEvent){.type=type,.timestamp=now,.touchId=NX_TOUCH_DEVICE_ID,
        .fingerId=gesture.id[i],.x=gesture.x[i],.y=gesture.y[i],.pressure=type==SDL_FINGERUP ? 0 : 1,
        .windowID=win ? SDL_GetWindowID(win) : 0};
    SDL_PushEvent(&ev);
}

static void NX_EndGesture(SDL_Window *win,Uint32 now) {
    if(gesture.active)for(unsigned i=0;i<2;i++)NX_Finger(i,SDL_FINGERUP,win,now);
    gesture.active=false;
}

void NX_TouchCancel(void) {
    NX_EndGesture(NULL,SDL_GetTicks());
    if(touch.button) NX_MouseSource(1,touch.button,false,touch.x,touch.y);
    touch.active=false;touch.button=0;pending.active=false;gesture.blocked=false;
}

/* Keep the original pair across HID array reordering and ignore extra contacts.
 * After either contact lifts, consume remaining fingers until the screen is clear. */
static bool NX_MultiTouch(HidTouchScreenState const *sample,SDL_Window *win,Uint32 now) {
    if(!gesture.blocked && sample->count<2)return false;
    if(!gesture.blocked) {
        gesture.blocked=gesture.active=true;pending.active=false;
        for(unsigned i=0;i<2;i++) {
            gesture.id[i]=sample->touches[i].finger_id;
            gesture.x[i]=SDL_clamp(sample->touches[i].x/1280.0f,0,1);
            gesture.y[i]=SDL_clamp(sample->touches[i].y/720.0f,0,1);
            NX_Finger(i,SDL_FINGERDOWN,win,now);
        }
        /* Finger-down must cancel the engine selection before releasing a held mouse button. */
        if(touch.button)NX_MouseSource(1,touch.button,false,touch.x,touch.y);
        touch.active=false;touch.button=0;touch.second=false;
        return true;
    }
    if(gesture.active) {
        int slots[2]={-1,-1};
        for(int k=0;k<sample->count && k<16;k++)for(unsigned i=0;i<2;i++)
            if(sample->touches[k].finger_id==gesture.id[i])slots[i]=k;
        if(slots[0]<0 || slots[1]<0)NX_EndGesture(win,now);
        else for(unsigned i=0;i<2;i++) {
            HidTouchState const *finger=&sample->touches[slots[i]];
            float x=SDL_clamp(finger->x/1280.0f,0,1), y=SDL_clamp(finger->y/720.0f,0,1);
            if(x==gesture.x[i] && y==gesture.y[i])continue;
            gesture.x[i]=x;gesture.y[i]=y;NX_Finger(i,SDL_FINGERMOTION,win,now);
        }
    }
    if(sample->count==0)gesture.blocked=false;
    return true;
}

static void NX_TouchClick(int button,int x,int y) {
    NX_MouseSource(1,button,true,x,y);NX_MouseSource(1,button,false,x,y);
}
static void NX_FlushTap(void) {
    if(pending.active)NX_TouchClick(SDL_BUTTON_LEFT,pending.x,pending.y);
    pending.active=false;
}

void NX_TouchFrame(SDL_Window *win,Uint32 now,int w,int h) {
    /* SDL_Quit during video restart clears the global filter. Restore it once. */
    SDL_EventFilter filter;void *userdata;
    if(!SDL_GetEventFilter(&filter,&userdata) || filter!=NX_FilterTouch)
        SDL_SetEventFilter(NX_FilterTouch,NULL);
    HidTouchScreenState sample;
    bool available=hidGetTouchScreenStates(&sample,1)!=0;
    if(available && NX_MultiTouch(&sample,win,now))return;
    if(pending.active && !touch.active && (Sint32)(now-pending.deadline)>=0)NX_FlushTap();
    if(!available || w<=0 || h<=0)return;
    if(sample.count<=0) {
        if(touch.active) {
            if(touch.button)NX_MouseSource(1,touch.button,false,touch.x,touch.y);
            else if(touch.second) {pending.active=false;NX_TouchClick(SDL_BUTTON_RIGHT,touch.x,touch.y);}
            else {pending.active=true;pending.x=touch.x;pending.y=touch.y;pending.deadline=now+NX_DOUBLE_DELAY;}
        }
        touch.active=false;touch.button=0;touch.second=false;return;
    }
    HidTouchState *finger=&sample.touches[0];
    if(touch.active) {
        for(int i=0;i<sample.count && i<16;i++)
            if(sample.touches[i].finger_id==touch.finger) {finger=&sample.touches[i];break;}
    }
    int x=(int)((u64)finger->x*w/1280),y=(int)((u64)finger->y*h/720);
    x=SDL_max(0,SDL_min(w-1,x));y=SDL_max(0,SDL_min(h-1,y));
    if(!touch.active) {
        int px=x-pending.x,py=y-pending.y;
        int distance=SDL_max(16,w/40);
        touch.second=pending.active && (Sint32)(pending.deadline-now)>=0 && px*px+py*py<=distance*distance;
        if(pending.active && !touch.second)NX_FlushTap();
        touch.active=true;touch.finger=finger->finger_id;touch.started=now;
        touch.start_x=touch.x=x;touch.start_y=touch.y=y;
        NX_TouchMotion(win,x,y);
    }
    int dx=x-touch.start_x,dy=y-touch.start_y;
    if(!touch.button) {
        if(now-touch.started>=NX_RIGHT_DELAY && dx*dx+dy*dy<=NX_TAP_SLOP*NX_TAP_SLOP) {
            pending.active=false;
            touch.button=SDL_BUTTON_RIGHT;
            NX_MouseSource(1,touch.button,true,x,y);
        } else if(dx*dx+dy*dy>NX_TAP_SLOP*NX_TAP_SLOP) {
            NX_FlushTap();touch.second=false;
            touch.button=SDL_BUTTON_LEFT;
            NX_MouseSource(1,touch.button,true,touch.start_x,touch.start_y);
        }
    }
    if(x!=touch.x || y!=touch.y) NX_TouchMotion(win,x,y);
}
