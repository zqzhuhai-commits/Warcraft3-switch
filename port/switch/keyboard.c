/* Explicit native text entry; menu text-event activation never opens an applet. */
#include <switch.h>
#include <SDL2/SDL.h>
#include <string.h>
#include "platform.h"

/* The Switch SDL backend opens its inline keyboard in StartTextInput itself,
 * even when SDL_HINT_ENABLE_SCREEN_KEYBOARD is false. Keep event delivery for
 * physical keyboards; a separate, deliberate shortcut owns the native dialog. */
/* SDL also invokes this backend hook on focus regain, bypassing its public
 * StartTextInput function. Link-wrap the callback to prevent automatic dialogs. */
void __wrap_SWITCH_StartTextInput(void *device) { (void)device; }

void NX_StartTextInput(void) {
    SDL_EventState(SDL_TEXTINPUT,SDL_ENABLE);
    SDL_EventState(SDL_TEXTEDITING,SDL_ENABLE);
}

void NX_ShowKeyboard(void) {
    if(!SDL_IsTextInputActive()) return;
    SwkbdConfig config;
    if(R_FAILED(swkbdCreate(&config,0))) return;
    swkbdConfigMakePresetDefault(&config);
    swkbdConfigSetStringLenMax(&config,128);
    char text[1024]={0};
    Result result=swkbdShow(&config,text,sizeof(text));
    swkbdClose(&config);
    if(R_FAILED(result)) return;
    /* SDL_TEXTINPUT holds at most 31 UTF-8 bytes; preserve codepoint boundaries. */
    for(char const *p=text;*p;) {
        size_t length=0;
        while(p[length]) {
            unsigned char c=(unsigned char)p[length];
            size_t bytes=c<0x80 ? 1 : c<0xe0 ? 2 : c<0xf0 ? 3 : 4;
            if(length+bytes>=SDL_TEXTINPUTEVENT_TEXT_SIZE) break;
            length+=bytes;
        }
        SDL_Event event={0};event.type=SDL_TEXTINPUT;
        event.text.timestamp=SDL_GetTicks();
        SDL_Window *window=SDL_GetKeyboardFocus();
        event.text.windowID=window ? SDL_GetWindowID(window) : 0;
        memcpy(event.text.text,p,length);
        SDL_PushEvent(&event);p+=length;
    }
}
