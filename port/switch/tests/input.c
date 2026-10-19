#include "switch.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include "../platform.h"
HidTouchScreenState test_touch;
bool test_touch_available=true;
u64 test_buttons;
HidAnalogStickState test_sticks[2];
bool test_running = true;
static int events(Uint32 type,int code) {
    SDL_Event ev; int count=0;
    while (SDL_PollEvent(&ev)) {
        if (ev.type==type && (type==SDL_QUIT ||
            ((type==SDL_KEYDOWN || type==SDL_KEYUP) ? ev.key.keysym.sym==code : ev.button.button==code))) count++;
    }
    return count;
}
int main(void) {
    assert(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_TIMER)==0);
    test_buttons=HidNpadButton_A;NX_PumpInput();assert(events(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT)==1);
    test_buttons|=HidNpadButton_ZR;NX_PumpInput();assert(events(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT)==0);
    test_buttons=HidNpadButton_ZR;NX_PumpInput();assert(events(SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT)==0);
    test_buttons=0;NX_PumpInput();assert(events(SDL_MOUSEBUTTONUP,SDL_BUTTON_LEFT)==1);
    test_buttons=HidNpadButton_ZL|HidNpadButton_R;NX_PumpInput();assert(events(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_RIGHT)==1);
    assert(SDL_GetModState()&KMOD_LSHIFT);
    test_buttons=0;NX_PumpInput();assert(events(SDL_MOUSEBUTTONUP,SDL_BUTTON_RIGHT)==1);assert(SDL_GetModState()==KMOD_NONE);
    NX_SetMenuInput(1);
    test_sticks[1].x=20000;NX_PumpInput();assert(events(SDL_KEYDOWN,SDLK_RIGHT)==1);
    NX_PumpInput();assert(events(SDL_KEYDOWN,SDLK_RIGHT)==0);
    test_sticks[1].x=0;NX_PumpInput();assert(events(SDL_KEYUP,SDLK_RIGHT)==1);
    test_buttons=HidNpadButton_Plus;NX_PumpInput();assert(events(SDL_KEYDOWN,SDLK_F10)==1);
    test_buttons=0;NX_PumpInput();assert(events(SDL_KEYUP,SDLK_F10)==1);
    NX_StartTextInput();
    test_buttons=HidNpadButton_L|HidNpadButton_Plus;NX_PumpInput();
    assert(test_keyboard_shows==1);assert(events(SDL_KEYDOWN,SDLK_F10)==0);
    NX_PumpInput();assert(test_keyboard_shows==1);
    test_buttons=0;NX_PumpInput();assert(events(SDL_KEYUP,SDLK_F10)==0);
    test_buttons=HidNpadButton_Up;NX_PumpInput();assert(events(SDL_KEYDOWN,SDLK_UP)==1);
    SDL_Delay(360);NX_PumpInput();assert(events(SDL_KEYDOWN,SDLK_UP)==1);
    NX_SetMenuInput(0);NX_PumpInput();
    bool old_up=false,new_page=false;SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type==SDL_KEYUP && e.key.keysym.sym==SDLK_UP)old_up=true;
        if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_PAGEUP)new_page=true;
    }
    assert(old_up && new_page);
    test_buttons=0;NX_PumpInput();events(SDL_KEYUP,SDLK_PAGEUP);
    test_sticks[1].x=12000;SDL_Delay(30);NX_PumpInput();
    float ax,ay;NX_GetCameraAxes(&ax,&ay);assert(ax>0 && ax<0.3f && ay==0);
    assert(events(SDL_KEYDOWN,SDLK_RIGHT)==0);
    test_sticks[1].x=32767;SDL_Delay(30);NX_PumpInput();NX_GetCameraAxes(&ax,&ay);assert(ax>0.3f && ax<=1);
    test_sticks[1].x=0;
    for(int i=0;i<10;i++){SDL_Delay(30);NX_PumpInput();}
    NX_GetCameraAxes(&ax,&ay);assert(ax==0 && ay==0);
    test_running=false;NX_PumpInput();assert(events(SDL_QUIT,0)==1);
    SDL_Quit();puts("Switch input integration: menu rebind/repeat and proportional camera passed");
}
