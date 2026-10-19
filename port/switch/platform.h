#ifndef OPENREALM_SWITCH_PLATFORM_H
#define OPENREALM_SWITCH_PLATFORM_H
/* The platform adapter has no engine/game state; it emits ordinary SDL input. */
void NX_PumpInput(void);
void NX_SetMenuInput(int enabled);
void NX_GetCameraAxes(float *x,float *y);
void NX_StartTextInput(void);
void NX_ShowKeyboard(void);
#endif
