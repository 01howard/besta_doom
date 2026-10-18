#include <stdint.h>
#include <mutekix/time.h>

typedef enum
{
    DOOM_KEY_UNKNOWN = -1,
    DOOM_KEY_TAB = 9,
    DOOM_KEY_ENTER = 13,
    DOOM_KEY_ESCAPE = 27,
    DOOM_KEY_SPACE = 32,
    DOOM_KEY_APOSTROPHE = '\'',
    DOOM_KEY_MULTIPLY = '*',
    DOOM_KEY_COMMA = ',',
    DOOM_KEY_MINUS = 0x2d,
    DOOM_KEY_PERIOD = '.',
    DOOM_KEY_SLASH = '/',
    DOOM_KEY_0 = '0',
    DOOM_KEY_1 = '1',
    DOOM_KEY_2 = '2',
    DOOM_KEY_3 = '3',
    DOOM_KEY_4 = '4',
    DOOM_KEY_5 = '5',
    DOOM_KEY_6 = '6',
    DOOM_KEY_7 = '7',
    DOOM_KEY_8 = '8',
    DOOM_KEY_9 = '9',
    DOOM_KEY_SEMICOLON = ';',
    DOOM_KEY_EQUALS = 0x3d,
    DOOM_KEY_LEFT_BRACKET = '[',
    DOOM_KEY_RIGHT_BRACKET = ']',
    DOOM_KEY_A = 'a',
    DOOM_KEY_B = 'b',
    DOOM_KEY_C = 'c',
    DOOM_KEY_D = 'd',
    DOOM_KEY_E = 'e',
    DOOM_KEY_F = 'f',
    DOOM_KEY_G = 'g',
    DOOM_KEY_H = 'h',
    DOOM_KEY_I = 'i',
    DOOM_KEY_J = 'j',
    DOOM_KEY_K = 'k',
    DOOM_KEY_L = 'l',
    DOOM_KEY_M = 'm',
    DOOM_KEY_N = 'n',
    DOOM_KEY_O = 'o',
    DOOM_KEY_P = 'p',
    DOOM_KEY_Q = 'q',
    DOOM_KEY_R = 'r',
    DOOM_KEY_S = 's',
    DOOM_KEY_T = 't',
    DOOM_KEY_U = 'u',
    DOOM_KEY_V = 'v',
    DOOM_KEY_W = 'w',
    DOOM_KEY_X = 'x',
    DOOM_KEY_Y = 'y',
    DOOM_KEY_Z = 'z',
    DOOM_KEY_BACKSPACE = 127,
    DOOM_KEY_CTRL = (0x80 + 0x1d), // Both left and right
    DOOM_KEY_LEFT_ARROW = 0xac,
    DOOM_KEY_UP_ARROW = 0xad,
    DOOM_KEY_RIGHT_ARROW = 0xae,
    DOOM_KEY_DOWN_ARROW = 0xaf,
    DOOM_KEY_SHIFT = (0x80 + 0x36), // Both left and right
    DOOM_KEY_ALT = (0x80 + 0x38), // Both left and right
    DOOM_KEY_F1 = (0x80 + 0x3b),
    DOOM_KEY_F2 = (0x80 + 0x3c),
    DOOM_KEY_F3 = (0x80 + 0x3d),
    DOOM_KEY_F4 = (0x80 + 0x3e),
    DOOM_KEY_F5 = (0x80 + 0x3f),
    DOOM_KEY_F6 = (0x80 + 0x40),
    DOOM_KEY_F7 = (0x80 + 0x41),
    DOOM_KEY_F8 = (0x80 + 0x42),
    DOOM_KEY_F9 = (0x80 + 0x43),
    DOOM_KEY_F10 = (0x80 + 0x44),
    DOOM_KEY_F11 = (0x80 + 0x57),
    DOOM_KEY_F12 = (0x80 + 0x58),
    DOOM_KEY_PAUSE = 0xff
} doom_key_t;


#define KEY_HOLD_TIMEOUT_MS  150
#define MAX_HELD_KEYS        16

typedef struct {
    doom_key_t  key;         
    uint64_t    tick;        
} held_key_t;

static held_key_t  g_held[MAX_HELD_KEYS];
static uint64_t    g_next_expire;  

extern void doom_key_down(doom_key_t key);
extern void doom_key_up(doom_key_t key); 

void add_key(doom_key_t k)
{
    if (k == DOOM_KEY_UNKNOWN) return;
    const uint64_t now = mutekix_time_get_usecs();

    int empty  = -1;
    int oldest = -1;

    for (int i = 0; i < MAX_HELD_KEYS; i++) {
        const held_key_t *h = &g_held[i];
        if (h->key == k) {
            g_held[i].tick = now;                 
            if (now + KEY_HOLD_TIMEOUT_MS < g_next_expire || g_next_expire == 0)
                g_next_expire = now + KEY_HOLD_TIMEOUT_MS;
            return;
        }
        if (empty < 0 && h->key == DOOM_KEY_UNKNOWN) {
            empty = i;
        }
        if (oldest < 0 || (h->key != DOOM_KEY_UNKNOWN && h->tick < g_held[oldest].tick))
            oldest = i;
    }

    if (empty < 0) {                              
        doom_key_up(g_held[oldest].key);
        empty = oldest;
    }

    g_held[empty].key  = k;
    g_held[empty].tick = now;
    doom_key_down(k);

    const uint64_t exp = now + KEY_HOLD_TIMEOUT_MS;
    if (g_next_expire == 0 || exp < g_next_expire)
        g_next_expire = exp;
}

void tick_key(void)
{
    const uint64_t now = mutekix_time_get_usecs();
    if (g_next_expire == 0 || now < g_next_expire)   
        return;

    uint64_t next = 0;
    for (int i = 0; i < MAX_HELD_KEYS; i++) {
        held_key_t *h = &g_held[i];
        if (h->key == DOOM_KEY_UNKNOWN) continue;

        if (now - h->tick >= KEY_HOLD_TIMEOUT_MS) {
            doom_key_up(h->key);
            h->key = DOOM_KEY_UNKNOWN;
        } else {
            const uint64_t exp = h->tick + KEY_HOLD_TIMEOUT_MS;
            if (next == 0 || exp < next) next = exp;
        }
    }
    g_next_expire = next;
}