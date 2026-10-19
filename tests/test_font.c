#include "test.h"
#include "renderer/r_local.h"
/* Exercise the real renderer layout/draw pass with fixed glyph metrics, no GPU. */
refImport_t ri;
#include "renderer/r_font.c"
static rect_t drawn_glyphs[128];
static int drawn_count;
static void *font_alloc(long n) { return calloc(1,n); }
texture_t *R_AllocateTexture(uint32_t w,uint32_t h) {(void)w;(void)h;abort();}
void R_LoadTextureMipLevel(texture_t *t,texMip_t const *m) {(void)t;(void)m;abort();}
void R_DrawImageEx(drawImage_t const *p) {(void)p;}
void R_DrawImageBatch(texture_t const *t,SHADERTYPE s,BLEND_MODE a,float g,float r,bool c,rect_t const *clip,vertex_t const *v,uint32_t n,bool repeat) {
    (void)t;(void)s;(void)a;(void)g;(void)r;(void)c;(void)clip;(void)v;(void)n;(void)repeat;
}
vertex_t *R_AddQuad(vertex_t *v,rect_t const *screen,rect_t const *uv,color32_t color,float z) {
    (void)uv;(void)color;(void)z;
    if(drawn_count<128)drawn_glyphs[drawn_count++]=*screen;
    return v+6;
}
static font_t fixed_font;
static glyphSet_t fixed_set;
static texture_t fixed_image;
static drawText_t text_args(char const *text,int columns) {
    memset(&fixed_font,0,sizeof(fixed_font));memset(&fixed_set,0,sizeof(fixed_set));
    ri.MemAlloc=font_alloc;ri.MemFree=free;
    fixed_font.size=20;fixed_font.height=10;fixed_image.width=fixed_image.height=128;
    fixed_set.image=&fixed_image;
    for(int i=0;i<256;i++) {
        fixed_set.glyphs[i].xadvance=20;fixed_set.glyphs[i].x1=10;fixed_set.glyphs[i].y1=10;
        fixed_font.sets[i]=&fixed_set;
    }
    drawn_count=0;
    return (drawText_t){.font=&fixed_font,.text=text,.textWidth=columns*INV_SCALE_X(20),
        .rect={0,0,1,1},.halign=FONT_JUSTIFYLEFT,.valign=FONT_JUSTIFYTOP,.flags=DRAW_WORD_WRAP};
}
TEST(font, cjk_paragraph_fills_horizontal_lines) {
    drawText_t a=text_args("虽然恶魔首领阿克蒙德和燃烧军团",6);
    vec2_t size=R_GetTextSize(&a);
    T_ASSERT(fabsf(size.y-3*R_GetFontHeight(&fixed_font))<0.00001f);
    R_DrawText(&a);T_EQ(drawn_count,15);
    for(int i=0;i<15;i++) {
        T_ASSERT(fabsf(drawn_glyphs[i].x-(i%6)*INV_SCALE_X(20))<0.00001f);
        T_ASSERT(fabsf(drawn_glyphs[i].y-(i/6)*R_GetFontHeight(&fixed_font))<0.00001f);
    }
}
TEST(font, latin_words_stay_together_and_long_tokens_split) {
    drawText_t a=text_args("ab cdef",6);R_DrawText(&a);
    T_ASSERT(drawn_glyphs[3].x==0);
    T_ASSERT(drawn_glyphs[3].y>drawn_glyphs[0].y);
    a=text_args("abcdefghijklm",6);R_DrawText(&a);
    T_ASSERT(drawn_glyphs[5].y==drawn_glyphs[0].y);
    T_ASSERT(drawn_glyphs[6].x==0);
    T_ASSERT(drawn_glyphs[12].y>drawn_glyphs[6].y);
}
TEST(font, cjk_color_controls_and_explicit_newlines) {
    drawText_t a=text_args("|cffff0000甲乙|r丙丁|n戊己\n庚辛",4);R_DrawText(&a);
    T_EQ(drawn_count,8);
    T_ASSERT(drawn_glyphs[3].y==drawn_glyphs[0].y);
    T_ASSERT(drawn_glyphs[4].y>drawn_glyphs[3].y);
    T_ASSERT(drawn_glyphs[6].y>drawn_glyphs[4].y);
    a=text_args("甲乙丙丁",1);R_DrawText(&a);
    T_ASSERT(drawn_glyphs[1].y>drawn_glyphs[0].y);
    a=text_args("甲乙丙丁",1);a.flags=0;R_DrawText(&a);
    T_ASSERT(drawn_glyphs[3].y==drawn_glyphs[0].y);
}
