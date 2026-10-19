#include "switch.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include "../platform.h"
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
    test_sticks[1].x=20000;NX_PumpInput();assert(events(SDL_KEYDOWN,SDLK_RIGHT)==1);
    NX_PumpInput();assert(events(SDL_KEYDOWN,SDLK_RIGHT)==0);
    test_sticks[1].x=0;NX_PumpInput();assert(events(SDL_KEYUP,SDLK_RIGHT)==1);
    test_buttons=HidNpadButton_Plus;NX_PumpInput();assert(events(SDL_KEYDOWN,SDLK_F10)==1);
    test_buttons=0;NX_PumpInput();assert(events(SDL_KEYUP,SDLK_F10)==1);
    test_running=false;NX_PumpInput();assert(events(SDL_QUIT,0)==1);
    SDL_Quit();puts("Switch input integration: 14 assertions passed");
}
