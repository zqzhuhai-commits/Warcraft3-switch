#include "switch.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../platform.h"
extern void __wrap_SWITCH_StartTextInput(void *device);
int main(void) {
    assert(SDL_Init(SDL_INIT_EVENTS)==0);
    SDL_StopTextInput();
    NX_ShowKeyboard();assert(test_keyboard_shows==0);
    __wrap_SWITCH_StartTextInput(NULL);assert(test_keyboard_shows==0);
    NX_StartTextInput();assert(SDL_IsTextInputActive());assert(test_keyboard_shows==0);
    NX_StartTextInput();assert(test_keyboard_shows==0);
    /* The OS keyboard opens only for an explicit request, never menu activation. */
    test_keyboard_text="魔兽争霸冰封王座输入文字测试长段落abcdef";
    NX_ShowKeyboard();assert(test_keyboard_shows==1);assert(test_keyboard_closes==1);
    char result[256]={0};SDL_Event e;int chunks=0;
    while(SDL_PollEvent(&e))if(e.type==SDL_TEXTINPUT){
        assert(((unsigned char)e.text.text[0]&0xc0)!=0x80);
        assert(strlen(e.text.text)<sizeof(e.text.text));strcat(result,e.text.text);chunks++;
    }
    assert(chunks>1);assert(strcmp(result,test_keyboard_text)==0);
    test_keyboard_show_result=-1;NX_ShowKeyboard();
    while(SDL_PollEvent(&e))assert(e.type!=SDL_TEXTINPUT);
    assert(test_keyboard_closes==2);
    test_keyboard_create_result=-1;NX_ShowKeyboard();assert(test_keyboard_closes==2);
    SDL_StopTextInput();assert(!SDL_IsTextInputActive());NX_ShowKeyboard();assert(test_keyboard_shows==2);
    SDL_Quit();puts("Switch keyboard: silent activation, explicit dialog, UTF-8 chunks, cancellation and cleanup passed");
}
