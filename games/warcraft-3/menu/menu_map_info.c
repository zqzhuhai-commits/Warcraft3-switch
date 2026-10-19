/*
 * menu_map_info.c - Warcraft III map info pane runtime layout.
 */

#include "menu_local.h"
#include "generated/map_info_pane.h"

static void UI_SetSizeIfPresent(frameDef_t *frame, float width, float height) {
    if (frame) {
        UI_SetSize(frame, width, height);
    }
}

static void UI_SetPointIfPresent(frameDef_t *frame,
                                 UIFRAMEPOINT point,
                                 frameDef_t const *relative,
                                 UIFRAMEPOINT relative_point,
                                 float x,
                                 float y)
{
    if (frame && relative) {
        frame->AnyPointsSet=false;
        UI_SetPoint(frame, point, relative, relative_point, x, y);
    }
}

static void UI_SetHiddenIfPresent(frameDef_t *frame, bool hidden) {
    if (frame) {
        UI_SetHidden(frame, hidden);
    }
}

/* A long custom-map description owns a finite preview panel, not the buttons below it. */
static void UI_DrawMapDescription(frameDef_t const *frame,rect_t const *rect) {
    refExport_t *renderer=mi.GetRenderer();
    if(!renderer || !renderer->LoadFont || !renderer->DrawText || !frame->Text)return;
    cstring_t path=Theme_String(frame->Font.Name[0] ? frame->Font.Name : "MasterFont","Default");
    uint32_t pixels=frame->Font.Size>0 ? (uint32_t)(frame->Font.Size*1000+0.5f) : 13;
    font_t const *font=renderer->LoadFont(path,pixels);
    if(!font)return;
    renderer->DrawText(&(drawText_t){.font=font,.text=frame->Text,.rect=*rect,.textWidth=rect->w,
        .color=frame->Font.Color,.halign=FONT_JUSTIFYLEFT,.valign=FONT_JUSTIFYTOP,
        .flags=DRAW_WORD_WRAP|DRAW_CLIP,.clip=*rect});
}

void UI_LayoutMapInfoPane(frameDef_t *frame) {
    MapInfoPane_t pane;
    float height, row_top, map_top = 0.035f;
    bool compact;

    if (!frame) {
        return;
    }
    if (!MapInfoPane_Bind(&pane, frame)) {
        return;
    }
    height = frame->Height;
    if (height <= 0.0f && frame->Parent) {
        height = frame->Parent->Height;
    }
    float width=frame->Width;
    if(width<=0.0f && frame->Parent) width=frame->Parent->Width;
    if(width<=0.024f) width=0.271875f;
    float content=width-0.024f;
    compact = height > 0.0f && height < 0.25f;
    row_top = compact ? 0.163f : 0.195f;

    UI_SetPointIfPresent(pane.MaxPlayersIcon, FRAMEPOINT_TOPLEFT, frame, FRAMEPOINT_TOPLEFT, 0.012f, -0.004f);
    UI_SetSizeIfPresent(pane.MapNameValue, MAX(0.01f,content-0.023f), 0.020f);
    UI_SetPointIfPresent(pane.MapNameValue, FRAMEPOINT_LEFT, pane.MaxPlayersIcon, FRAMEPOINT_RIGHT, 0.0025f, 0.0f);
    UI_SetHiddenIfPresent(pane.AuthIcon, true);

    /* Retail leaves preview placement to the native pane. Fit its authored image/border
     * proportions into the compact preview slot; moving only the rows overlapped the border. */
    if (compact && pane.MinimapImage && pane.MinimapImageBackdrop) {
        float scale = MIN(1.0f, (row_top - map_top - 0.002f) / pane.MinimapImageBackdrop->Height);
        UI_SetSize(pane.MinimapImage, pane.MinimapImage->Width * scale, pane.MinimapImage->Height * scale);
        UI_SetSize(pane.MinimapImageBackdrop, pane.MinimapImageBackdrop->Width * scale, pane.MinimapImageBackdrop->Height * scale);
        map_top += (pane.MinimapImageBackdrop->Height - pane.MinimapImage->Height) * 0.5f;
    }
    UI_SetPointIfPresent(pane.MinimapImage, FRAMEPOINT_TOP, frame, FRAMEPOINT_TOP, 0.0f, -map_top);

    UI_SetSizeIfPresent(pane.SuggestedPlayersLabel, content*0.46f, 0.016f);
    UI_SetPointIfPresent(pane.SuggestedPlayersLabel,
                         FRAMEPOINT_TOPLEFT,
                         frame,
                         FRAMEPOINT_TOPLEFT,
                         0.012f,
                         -row_top);
    UI_SetSizeIfPresent(pane.SuggestedPlayersValue, content*0.54f, 0.016f);
    UI_SetPointIfPresent(pane.SuggestedPlayersValue,
                         FRAMEPOINT_TOPRIGHT,
                         frame,
                         FRAMEPOINT_TOPRIGHT,
                         -0.012f,
                         -row_top);

    UI_SetSizeIfPresent(pane.MapSizeLabel, content*0.46f, 0.016f);
    UI_SetPointIfPresent(pane.MapSizeLabel, FRAMEPOINT_TOPLEFT, pane.SuggestedPlayersLabel, FRAMEPOINT_BOTTOMLEFT, 0.0f, -0.002f);
    UI_SetSizeIfPresent(pane.MapSizeValue, content*0.54f, 0.016f);
    UI_SetPointIfPresent(pane.MapSizeValue, FRAMEPOINT_TOPRIGHT, pane.SuggestedPlayersValue, FRAMEPOINT_BOTTOMRIGHT, 0.0f, -0.002f);

    UI_SetSizeIfPresent(pane.MapTilesetLabel, content*0.46f, 0.016f);
    UI_SetPointIfPresent(pane.MapTilesetLabel, FRAMEPOINT_TOPLEFT, pane.MapSizeLabel, FRAMEPOINT_BOTTOMLEFT, 0.0f, -0.002f);
    UI_SetSizeIfPresent(pane.MapTilesetValue, content*0.54f, 0.016f);
    UI_SetPointIfPresent(pane.MapTilesetValue, FRAMEPOINT_TOPRIGHT, pane.MapSizeValue, FRAMEPOINT_BOTTOMRIGHT, 0.0f, -0.002f);

    if(pane.MapDescValue) {pane.MapDescValue->Type=FT_FRAME;pane.MapDescValue->draw=UI_DrawMapDescription;}
    UI_SetHiddenIfPresent(pane.MapDescLabel, compact);
    UI_SetHiddenIfPresent(pane.MapDescValue, compact);
    if (!compact) {
        UI_SetSizeIfPresent(pane.MapDescLabel, content, 0.016f);
        UI_SetPointIfPresent(pane.MapDescLabel, FRAMEPOINT_TOPLEFT, pane.MapTilesetLabel, FRAMEPOINT_BOTTOMLEFT, 0.0f, -0.012f);
        UI_SetSizeIfPresent(pane.MapDescValue, content, MAX(0.016f,height-row_top-0.082f));
        UI_SetPointIfPresent(pane.MapDescValue, FRAMEPOINT_TOPLEFT, pane.MapDescLabel, FRAMEPOINT_BOTTOMLEFT, 0.0f, -0.002f);
    }
}
