#include <muteki.h>
#include <mutekix/time.h>
#include <stdlib.h>
#include <string.h>

#define WIN32 //不要問為甚麼，但是不寫會炸

#define DOOM_IMPLEMENTATION
#define DOOM_IMPLEMENT_MALLOC
#define DOOM_IMPLEMENT_FILE_IO
#include "PureDOOM_modified.h" //修正檔案讀取

// See press_table.c.
extern void add_key(doom_key_t k);
extern void tick_key(void);

key_press_event_config_t old_cfg;

static void _quit_process_(int code) { 
    
    SetSysKeyState(&old_cfg);
    mutekix_time_fini();
    exit(code);
}

static void _get_time_(int *sec, int *usec) {
    unsigned long long m = mutekix_time_get_usecs();
    unsigned int s = (unsigned int)(m / 1000000ULL);   
    *sec  = (int)s;
    *usec = (int)((unsigned int)m - s * 1000000u);    
}

static bool has_doomrc_file(void) {
    short attr = _wfgetattr(_BUL(".doomrc"));
    if (attr < 0) return false;                    
    if (attr & BXC_FS_ATTR_DIR) return false;      
    return true;
}

#define KEY_QUIT KEY_MENU

static doom_key_t map_key(int v) {
    switch (v) {
        case KEY_UP:     return DOOM_KEY_UP_ARROW;
        case KEY_DOWN:   return DOOM_KEY_DOWN_ARROW;
        case KEY_LEFT:   return DOOM_KEY_LEFT_ARROW;
        case KEY_RIGHT:  return DOOM_KEY_RIGHT_ARROW;
        case KEY_ENTER:  return DOOM_KEY_ENTER;
        case KEY_ESC:    return DOOM_KEY_ESCAPE;
        case KEY_SPACE:  return DOOM_KEY_SPACE;   /* 開門/使用 */
        case KEY_F:      return DOOM_KEY_CTRL;    /* 開火 */
        case KEY_SHIFT:  return DOOM_KEY_SHIFT;   /* 跑步 */
        case KEY_TAB:    return DOOM_KEY_TAB;     /* 地圖 */
        case KEY_1:      return DOOM_KEY_1;
        case KEY_2:      return DOOM_KEY_2;
        case KEY_3:      return DOOM_KEY_3;
        case KEY_4:      return DOOM_KEY_4;
        case KEY_5:      return DOOM_KEY_5;
        case KEY_6:      return DOOM_KEY_6;
        case KEY_7:      return DOOM_KEY_7;
        case KEY_Y:      return DOOM_KEY_Y;       /* 選單「是」 */
        case KEY_N:      return DOOM_KEY_N;       /* 選單「否」 */
        default:         return DOOM_KEY_UNKNOWN;
    }
}

static int WndProc(ui_component_t*,unsigned int msg,short value,void*) {
    switch (msg) {
    case UI_EVENT_TYPE_KEY:
        if (value == KEY_QUIT) return 1;
        add_key(map_key((int)value));
        break;

    case UI_EVENT_TYPE_TOUCH_BEGIN:
        doom_button_down(DOOM_LEFT_BUTTON);
        break;

    case UI_EVENT_TYPE_TOUCH_END:
        doom_button_up(DOOM_LEFT_BUTTON);
        break;

    default:
        break;
    }
    return 0;
}

static void blit32(lcd_surface_t *sf, const int *restrict src) {
    const int w = sf->width, h = sf->height;
    const int stride = sf->xsize;              /* byte */
    uint8_t *dst_row = (uint8_t*)sf->buffer;

    for (int y = 0; y < h; y++) {
        uint32_t *restrict d = (uint32_t*)dst_row;
        for (int x = 0; x < w; x++) {
            uint32_t p = *src++;               /* A B G R 的 byte 順序: R G B A */
            *d++ = ((p & 0x000000FFu) << 16)   /* R -> 第3個byte */
                 |  (p & 0xFF00FF00u & ~0x00FF0000u & 0xFF00FF00u)
                 | ((p >> 16) & 0x000000FFu);  /* B -> 第1個byte */
        }
        dst_row += stride;
    }
}

static void blit24(lcd_surface_t *sf, const char *restrict s) {
    const int w = sf->width, h = sf->height;
    const int stride = sf->xsize;
    uint8_t *dst_row = (uint8_t*)sf->buffer;

    for (int y = 0; y < h; y++) {
        uint32_t *restrict d = (uint32_t*)dst_row;
        int x = 0;
        for (; x + 4 <= w; x += 4, s += 12) {
            *d++ = s[2]  | (s[1]  << 8) | (s[0]  << 16) | ((uint32_t)s[5]  << 24);
            *d++ = s[4]  | (s[3]  << 8) | (s[8]  << 16) | ((uint32_t)s[7]  << 24);
            *d++ = s[6]  | (s[11] << 8) | (s[10] << 16) | ((uint32_t)s[9]  << 24);
        }
        uint8_t *dt = (uint8_t*)d;
        for (; x < w; x++, s += 3, dt += 3) {
            dt[0] = s[2]; dt[1] = s[1]; dt[2] = s[0];
        }
        dst_row += stride;
    }
}

int main() {
    if(has_doomrc_file()){
        if(MessageBox(_BUL("見牘 .doomrc，將致舉程盡僵；當歸去而斬之。"),UI_MB_ICON_ERROR | UI_MB_BUTTON_OK | UI_MB_BUTTON_CANCEL) == UI_MB_RESULT_OK) return 0;
    }
    GetSysKeyState(&old_cfg);
    doom_set_exit(_quit_process_);
    doom_set_gettime(_get_time_);

    lcd_surface_t *sf = GetActiveVRamAddress();
    if (sf->encoding != LCD_SURFACE_ENCODING_RAW) {
        MessageBox(_BUL("方見HCA之式，乃大驚"), UI_MB_ICON_ERROR);
        _quit_process_(0);
    }

    int bpp = 0;
    switch (sf->depth) {
        case 24: bpp = 3; break;
        case 32: bpp = 4; break;
        default:
            MessageBox(_BUL("故仙遺跡，其秘之奧，不得斯解"), UI_MB_ICON_ERROR);
            _quit_process_(0);
    }

    doom_set_resolution(sf->width, sf->height);

    key_press_event_config_t cfg;
    cfg = old_cfg;
    cfg.long_press_delay = 1;        
    cfg.long_press_repeat_rate = 1;  
    SetSysKeyState(&cfg);

    mutekix_time_init();

    char* args[] = {"DOOM",NULL}; 
    doom_init(1, args, DOOM_FLAG_HIDE_MOUSE_OPTIONS);

    ui_event_sys_t sev;
    int n = 0;
    while (true) {
        while (n++ < 8 && GetPenEvent(&sev)) {   
            if (WndProc(sev.recipient, sev.event_type,
                        sev.key_code0, sev.user_data))
                goto out;
        }
        n = 0;
        
        tick_key();
        doom_update();
        if (bpp == 3){
            blit24(sf, doom_get_framebuffer(bpp));
        }else{
            blit32(sf, (int*)doom_get_framebuffer(bpp));
        }
        OSSleep(0);
    }
out:
    _quit_process_(0);
    return 0;
}