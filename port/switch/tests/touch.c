/* Production gesture adapter, real SDL queue, deterministic libnx samples/time. */
#include "switch.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include "../touch.h"
u64 test_buttons;
HidAnalogStickState test_sticks[2];
bool test_running = true;
HidTouchScreenState test_touch;
bool test_touch_available = true;
static SDL_Event queue[64];
static int drain(void) {
    int n=0; SDL_Event e;
    while(SDL_PollEvent(&e)) {assert(n<64);queue[n++]=e;}
    return n;
}
static void frame(int count,int x,int y,Uint32 now) {
    test_touch.count=count;
    test_touch.touches[0].finger_id=7;
    test_touch.touches[0].x=x;test_touch.touches[0].y=y;
    test_touch.touches[1].finger_id=8;
    NX_TouchFrame(NULL,now,640,360);
}
static int buttons(Uint32 type,int button) {
    int n=0;
    for(int i=0;i<64 && queue[i].type;i++)
        if(queue[i].type==type && queue[i].button.button==button)n++;
    return n;
}
static void clear(void){drain();SDL_memset(queue,0,sizeof(queue));}
int main(void) {
    assert(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_TIMER)==0);
    NX_InitInput();clear();
    /* A tap scales screen coordinates and emits exactly one left click. */
    frame(1,640,360,100);assert(drain()==1);
    assert(queue[0].motion.x==320 && queue[0].motion.y==180);
    clear();frame(0,0,0,150);assert(drain()==0);
    frame(0,0,0,431);assert(drain()==2);
    assert(queue[0].type==SDL_MOUSEBUTTONDOWN && queue[1].type==SDL_MOUSEBUTTONUP);
    assert(queue[0].button.which==SDL_TOUCH_MOUSEID);clear();
    /* Drag starts at the original point, then moves to the finger. */
    frame(1,100,100,200);clear();frame(1,200,200,250);drain();
    assert(queue[0].type==SDL_MOUSEBUTTONDOWN && queue[0].button.x==50);
    assert(queue[1].type==SDL_MOUSEMOTION && queue[1].motion.x==100);
    clear();frame(0,0,0,300);drain();assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT)==1);clear();
    /* Long press orders without selecting first. */
    frame(1,600,400,400);clear();frame(1,600,400,900);drain();
    assert(buttons(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_RIGHT)==1);
    assert(buttons(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT)==0);clear();
    frame(0,0,0,950);drain();assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_RIGHT)==1);clear();
    /* Two fingers right-click once, retaining ownership until all lift. */
    frame(2,400,200,1000);drain();assert(buttons(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_RIGHT)==1);clear();
    frame(1,400,200,1050);drain();assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_RIGHT)==0);clear();
    frame(0,0,0,1100);drain();assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_RIGHT)==1);clear();
    /* Native SDL touch synthesis must not duplicate our events. */
    SDL_Event e={.type=SDL_FINGERDOWN};assert(SDL_PushEvent(&e)==0);
    e=(SDL_Event){.button={.type=SDL_MOUSEBUTTONDOWN,.which=SDL_TOUCH_MOUSEID}};
    assert(SDL_PushEvent(&e)==0);assert(drain()==0);clear();
    /* A temporarily unavailable sample cannot release a drag. */
    frame(1,100,100,1200);frame(1,200,200,1250);clear();
    test_touch_available=false;frame(0,0,0,1300);assert(drain()==0);test_touch_available=true;
    NX_TouchCancel();drain();assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT)==1);clear();
    /* Touch releases cannot release the same button held on the controller. */
    NX_MouseSource(0,SDL_BUTTON_LEFT,true,0,0);
    frame(1,100,100,1400);frame(1,200,200,1450);clear();
    frame(0,0,0,1500);drain();assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT)==0);clear();
    NX_MouseSource(0,SDL_BUTTON_LEFT,false,0,0);drain();assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT)==1);
    clear();
    /* Physical screen bounds are clamped; finger array reorder keeps identity. */
    frame(1,9999,9999,1600);drain();assert(queue[0].motion.x==639 && queue[0].motion.y==359);
    clear();frame(0,0,0,1650);clear();
    frame(2,300,200,1700);clear();
    test_touch.touches[0]=(HidTouchState){.finger_id=8,.x=900,.y=600};
    test_touch.touches[1]=(HidTouchState){.finger_id=7,.x=320,.y=220};
    NX_TouchFrame(NULL,1750,640,360);drain();assert(queue[0].motion.x==160 && queue[0].motion.y==110);
    clear();NX_TouchCancel();drain();assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_RIGHT)==1);
    clear();
    frame(1,640,360,2000);clear();frame(0,0,0,2040);assert(drain()==0);
    frame(1,640,360,2120);clear();frame(0,0,0,2160);drain();
    assert(buttons(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT)==0);
    assert(buttons(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_RIGHT)==1);
    assert(buttons(SDL_MOUSEBUTTONUP,SDL_BUTTON_RIGHT)==1);
    clear();frame(0,0,0,2500);assert(drain()==0);
    frame(1,640,360,2600);frame(0,0,0,2650);clear();NX_TouchCancel();
    frame(0,0,0,3000);assert(drain()==0);
    SDL_Quit();puts("Switch touch integration: tap, drag, long press, two fingers, duplicate filtering, cancellation, shared buttons passed");
}
