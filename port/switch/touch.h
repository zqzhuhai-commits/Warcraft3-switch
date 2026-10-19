#ifndef OPENREALM_SWITCH_TOUCH_H
#define OPENREALM_SWITCH_TOUCH_H
#include <SDL2/SDL.h>
#include <stdbool.h>
/* Private ID is translated by our event filter into SDL_TOUCH_MOUSEID. */
#define NX_TOUCH_MOUSEID 0x4e58544fu
void NX_InitInput(void);
void NX_TouchFrame(SDL_Window *window, Uint32 now, int width, int height);
void NX_TouchCancel(void);
void NX_TouchPointer(int x,int y);
void NX_MouseSource(unsigned source, int button, bool down, int x, int y);
#endif
