#ifndef TEST_SWITCH_H
#define TEST_SWITCH_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
typedef uint64_t u64;
typedef struct { unsigned finger_id,x,y; } HidTouchState;
typedef struct { int count; HidTouchState touches[16]; } HidTouchScreenState;
extern HidTouchScreenState test_touch;
extern bool test_touch_available;
static inline void hidInitializeTouchScreen(void) {}
static inline size_t hidGetTouchScreenStates(HidTouchScreenState *s,size_t n) {
    (void)n;if(!test_touch_available)return 0;*s=test_touch;return 1;
}
typedef struct { int unused; } PadState;
typedef struct { int x,y; } HidAnalogStickState;
enum {HidNpadButton_A=1ULL<<0,HidNpadButton_B=1ULL<<1,HidNpadButton_X=1ULL<<2,HidNpadButton_Y=1ULL<<3,
HidNpadButton_L=1ULL<<4,HidNpadButton_R=1ULL<<5,HidNpadButton_ZL=1ULL<<6,HidNpadButton_ZR=1ULL<<7,
HidNpadButton_Plus=1ULL<<8,HidNpadButton_Minus=1ULL<<9,HidNpadButton_Up=1ULL<<10,HidNpadButton_Down=1ULL<<11,
HidNpadButton_Left=1ULL<<12,HidNpadButton_Right=1ULL<<13,HidNpadButton_StickL=1ULL<<14,HidNpadButton_StickR=1ULL<<15};
enum {HidNpadStyleSet_NpadStandard,AppletType_Application,AppletType_SystemApplication};
extern u64 test_buttons;
extern HidAnalogStickState test_sticks[2];
extern bool test_running;
static inline bool appletMainLoop(void){return test_running;}
static inline void padUpdate(PadState *p){(void)p;}
static inline u64 padGetButtons(PadState *p){(void)p;return test_buttons;}
static inline u64 padGetButtonsDown(PadState *p){return padGetButtons(p);}
static inline HidAnalogStickState padGetStickPos(PadState *p,int i){(void)p;return test_sticks[i];}
static inline void padConfigureInput(int n,int s){(void)n;(void)s;}
static inline void padInitializeDefault(PadState *p){(void)p;}
static inline int appletGetAppletType(void){return AppletType_Application;}
static inline int romfsInit(void){return 0;}
static inline void romfsExit(void){}
static inline void randomGet(void *p,size_t n){(void)p;(void)n;}
static inline void consoleInit(void *p){(void)p;}
static inline void consoleExit(void *p){(void)p;}
static inline void consoleUpdate(void *p){(void)p;}
typedef int Result;
typedef struct {int unused;} SwkbdConfig;
extern int test_keyboard_shows,test_keyboard_closes,test_keyboard_create_result,test_keyboard_show_result;
extern char const *test_keyboard_text;
static inline Result swkbdCreate(SwkbdConfig *p,size_t n){(void)p;(void)n;return test_keyboard_create_result;}
static inline void swkbdConfigMakePresetDefault(SwkbdConfig *p){(void)p;}
static inline void swkbdConfigSetStringLenMax(SwkbdConfig *p,unsigned n){(void)p;(void)n;}
static inline Result swkbdShow(SwkbdConfig *p,char *s,size_t n){(void)p;test_keyboard_shows++;snprintf(s,n,"%s",test_keyboard_text);return test_keyboard_show_result;}
static inline void swkbdClose(SwkbdConfig *p){(void)p;test_keyboard_closes++;}
#define R_FAILED(x) ((x)!=0)
#endif
