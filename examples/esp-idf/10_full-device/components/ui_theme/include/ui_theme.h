/*
 * SmartRing-Plus — UI 主题与布局常量（对齐 Figma「Home / 360 Round」）
 * 设计稿: https://www.figma.com/design/p9yDZSZgcBbLskqSQd4pku
 */
#pragma once

#include "lvgl.h"
#include "bsp/board_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 圆屏中心与安全区（内接正方形 ~255×255，圆心 180,180） */
#define UI_CENTER_X     (BOARD_LCD_H_RES / 2)
#define UI_CENTER_Y     (BOARD_LCD_V_RES / 2)
#define UI_SAFE_W       255
#define UI_SAFE_H       255
#define UI_SAFE_HALF    (UI_SAFE_W / 2)

/* Obsidian Tide 暗色主题（Figma tokens） */
#define UI_COLOR_BG         lv_color_hex(0x0A1016)
#define UI_COLOR_SURFACE    lv_color_hex(0x172029)
#define UI_COLOR_SURFACE_HI lv_color_hex(0x223041)
#define UI_COLOR_TEXT       lv_color_hex(0xF2F5F7)
#define UI_COLOR_ACCENT     lv_color_hex(0x3DDBC8)
#define UI_COLOR_WARN       lv_color_hex(0xE85D4C)
#define UI_COLOR_OK         lv_color_hex(0x43A047)
#define UI_COLOR_MUTED      lv_color_hex(0x8A9AAB)

/* 应用磁贴着色（Figma AppTile discs） */
#define UI_TILE_CLOCK       lv_color_hex(0x2F8683)
#define UI_TILE_WEATHER     lv_color_hex(0x4A7499)
#define UI_TILE_RECORD      lv_color_hex(0x854646)
#define UI_TILE_MUSIC       lv_color_hex(0x8A6744)
#define UI_TILE_FILES       lv_color_hex(0x577159)
#define UI_TILE_ALBUM       lv_color_hex(0x8A5266)
#define UI_TILE_SETTINGS    lv_color_hex(0x556575)

/* 字体 */
#define UI_FONT_SM          (&lv_font_montserrat_14)
#define UI_FONT_MD          (&lv_font_montserrat_16)
#define UI_FONT_LG          (&lv_font_montserrat_20)
#define UI_FONT_XL          (&lv_font_montserrat_28)
#define UI_FONT_HUGE        (&lv_font_montserrat_48)

/* 低电量阈值（%） */
#define UI_BATTERY_LOW_PCT  15

/** 将对象限制在安全区内居中 */
static inline void ui_theme_place_safe(lv_obj_t *obj, lv_coord_t w, lv_coord_t h, lv_coord_t y_ofs)
{
    lv_obj_set_size(obj, w, h);
    lv_obj_set_pos(obj, UI_CENTER_X - w / 2, UI_CENTER_Y - h / 2 + y_ofs);
}

/** 屏幕通用背景 */
static inline void ui_theme_apply_screen(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

#ifdef __cplusplus
}
#endif
