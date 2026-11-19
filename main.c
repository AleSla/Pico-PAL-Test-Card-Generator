#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/pll.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "video.pio.h"

// Hardware Pin Definitions
#define DAC_BASE_PIN 0
#define DAC_AUDIO_PIN 15
#define EXT_CLOCK_PIN 20
#define DEBUG_LED_PIN 25
#define BUTTON_PIN 14

// Digital voltage levels
#define SYNC_TIP 70
#define BLACK_LEVEL 116
#define GRAY_LEVEL 170
#define DARK_GRAY_LEVEL 143
#define WHITE_LEVEL 224

// Buffers 32-bit aligned to enable hyper-fast DMA memory composition
__attribute__((aligned(4))) uint8_t buf_blank_eq[1136];
__attribute__((aligned(4))) uint8_t buf_eq_blank[1136];
__attribute__((aligned(4))) uint8_t buf_vsync[1136];
__attribute__((aligned(4))) uint8_t buf_eq[1136];
__attribute__((aligned(4))) uint8_t buf_sync_eq[1136];
__attribute__((aligned(4))) uint8_t buf_eq_sync[1136];

__attribute__((aligned(4))) uint8_t buf_blank[4][1136];
__attribute__((aligned(4))) uint8_t buf_top_bot[4][1136];

__attribute__((aligned(4))) uint8_t buf_bg_blank_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_blank_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_z1_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_z1_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_z2_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_z2_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_z3_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_z3_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_z4_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_bg_z4_cast1[4][1136];

__attribute__((aligned(4))) uint8_t buf_grid_blank_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_blank_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_z1_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_z1_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_z2_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_z2_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_z3_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_z3_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_z4_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_grid_z4_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_bars_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_bars_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_bars_lower_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_bars_lower_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_gratings_upper_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_gratings_upper_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_cross_top[4][1136];
__attribute__((aligned(4))) uint8_t buf_cross_mid[4][1136];
__attribute__((aligned(4))) uint8_t buf_gratings_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_gratings_cast1[4][1136];

__attribute__((aligned(4))) uint8_t buf_sqwave_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_sqwave_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_gray_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_gray_cast1[4][1136];

__attribute__((aligned(4))) uint8_t buf_top_ref_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_top_ref_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_bot_ref_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_bot_ref_cast1[4][1136];

__attribute__((aligned(4))) uint8_t buf_top_lf_white_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_top_lf_white_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_top_lf_black_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_top_lf_black_cast1[4][1136];
__attribute__((aligned(4))) uint8_t buf_yc_cast0[4][1136];
__attribute__((aligned(4))) uint8_t buf_yc_cast1[4][1136];

__attribute__((aligned(4))) uint8_t buf_vbi_ttx_hdr[4][1136];
__attribute__((aligned(4))) uint8_t buf_vbi_ttx_row1[4][1136];
__attribute__((aligned(4))) uint8_t buf_vbi_ttx_row2[4][1136];

// The 4-Frame Phase Tracking Arrays (10,000 bytes each)
uint8_t* base_table[2500];
uint8_t* special_table[2500];

uint16_t audio_lut[64];
uint16_t *audio_ptr = audio_lut;

// Dedicated SRAM4 & SRAM5 banks: zero bus contention with main SRAM0-3 buffers
__scratch_x("video_ping") __attribute__((aligned(4))) uint8_t ping_buf[1136];
__scratch_y("video_pong") __attribute__((aligned(4))) uint8_t pong_buf[1136];

__attribute__((aligned(4))) int center_mask_dx[576];
__attribute__((aligned(4))) int center_copy_dx[576];
volatile bool enable_center_outline = false;
volatile bool switching_card = false;
volatile bool video_blanked = false;

PIO video_pio = pio0;
uint video_sm = 0;

int dma_chan_ping;
int dma_chan_pong;
int dma_chan_compose;
dma_channel_config c_compose;
volatile int current_line = 0;
volatile int current_frame_line = 0;

// Exact -90 deg/line rotation matching a true 1135-sample (283.75-cycle) PAL line
const int phase_offset[4] = {0, 3, 2, 1};
const bool v_inverted[4]  = {false, true, false, true};

// Shifts a 1135-sample line left by s_mod bytes and pads the tail with the next line's SYNC_TIP
void pack_line_1135(uint8_t *buf, int s_mod) {
    if (s_mod > 0) {
        memmove(buf, buf + s_mod, 1135 - s_mod);
    }
    for (int i = 1135 - s_mod; i < 1136; i++) {
        buf[i] = SYNC_TIP;
    }
}

#include "pm5544.c"
#include "fubk.c"
#include "vg1001_bnt.c"
#include "ueit.c"
#include "simple_patterns.c"
#include "teletext.c"

void init_compose_dma() {
    dma_chan_compose = dma_claim_unused_channel(true);
    c_compose = dma_channel_get_default_config(dma_chan_compose);
    channel_config_set_transfer_data_size(&c_compose, DMA_SIZE_32);
    channel_config_set_read_increment(&c_compose, true);
    channel_config_set_write_increment(&c_compose, true);
    channel_config_set_high_priority(&c_compose, false);
    dma_channel_set_config(dma_chan_compose, &c_compose, false);
}

static inline void __not_in_flash_func(fast_dma_start)(uint32_t *dst, const uint32_t *src, uint32_t count) {
    dma_channel_set_read_addr(dma_chan_compose, src, false);
    dma_channel_set_write_addr(dma_chan_compose, dst, false);
    dma_channel_set_trans_count(dma_chan_compose, count, true);
}

static inline void __not_in_flash_func(fast_dma_copy)(uint32_t *dst, const uint32_t *src, uint32_t count) {
    fast_dma_start(dst, src, count);
    dma_channel_wait_for_finish_blocking(dma_chan_compose);
}

// Synthesizes a valid 1135-sample monochrome black line directly in SCRATCH RAM (zero main-RAM reads)
static inline void __not_in_flash_func(fill_curtain_line)(uint32_t *dst32, uint32_t words, int s_mod) {
    uint32_t s32 = SYNC_TIP | (SYNC_TIP << 8) | (SYNC_TIP << 16) | (SYNC_TIP << 24);
    uint32_t b32 = BLACK_LEVEL | (BLACK_LEVEL << 8) | (BLACK_LEVEL << 16) | (BLACK_LEVEL << 24);
    for (uint32_t w = 0; w < 20; w++) dst32[w] = s32;
    for (uint32_t w = 21; w < words - 1; w++) dst32[w] = b32;
    uint8_t *dst8 = (uint8_t *)dst32;
    int sync_end = 84 - s_mod;
    for (int i = 80; i < 84; i++) dst8[i] = (i < sync_end) ? SYNC_TIP : BLACK_LEVEL;
    int tail_start = 1135 - s_mod;
    int max_x = (int)(words << 2);
    for (int i = max_x - 4; i < max_x; i++) dst8[i] = (i < tail_start) ? BLACK_LEVEL : SYNC_TIP;
}

// --- CORE 1 HYPER-FAST HARDWARE DMA COMPOSITOR ---
void __not_in_flash_func(compose_line)(uint32_t *dst32, int line_idx, int frame_line) {
    int s_mod = line_idx & 3;
    uint32_t words = (s_mod == 3) ? 283 : 284;
    int line = frame_line + 1;

    // Safe blanking curtain while Core 0 rebuilds pattern buffers
    if (switching_card) {
        video_blanked = true;
        if (line <= 5 || (line >= 311 && line <= 318) || line >= 623) {
            uint32_t *sync32;
            if (line == 624 || line == 625 || line == 4 || line == 5 || line == 311 || line == 312 || line == 316 || line == 317) {
                sync32 = (uint32_t *)buf_eq;
            } else if (line == 1 || line == 2 || line == 314 || line == 315) {
                sync32 = (uint32_t *)buf_vsync;
            } else if (line == 3) {
                sync32 = (uint32_t *)buf_sync_eq;
            } else if (line == 313) {
                sync32 = (uint32_t *)buf_eq_sync;
            } else if (line == 623) {
                sync32 = (uint32_t *)buf_blank_eq;
            } else {
                sync32 = (uint32_t *)buf_eq_blank;
            }
            fast_dma_copy(dst32, sync32, words);
            uint8_t *dst8 = (uint8_t *)dst32;
            int max_x = (int)(words << 2);
            for (int i = 1135 - s_mod; i < max_x; i++) dst8[i] = SYNC_TIP;
        } else {
            fill_curtain_line(dst32, words, s_mod);
        }
        return;
    }
    video_blanked = false;

    uint32_t *base32 = (uint32_t *)base_table[line_idx];
    uint32_t *spec32 = (uint32_t *)special_table[line_idx];

    if (line == 16 || line == 329) {
        base32 = (uint32_t *)buf_vbi_ttx_hdr[s_mod];
        spec32 = base32;
    } else if (line == 17 || line == 330) {
        base32 = (uint32_t *)buf_vbi_ttx_row1[s_mod];
        spec32 = base32;
    } else if (line == 18 || line == 331) {
        base32 = (uint32_t *)buf_vbi_ttx_row2[s_mod];
        spec32 = base32;
    }

    if (line < 23 || (line > 310 && line < 336) || line > 623) {
        fast_dma_copy(dst32, base32, words);
        if (line <= 5 || (line >= 311 && line <= 318) || line >= 623) {
            uint8_t *dst8 = (uint8_t *)dst32;
            int max_x = (int)(words << 2);
            for (int i = 1135 - s_mod; i < max_x; i++) {
                dst8[i] = SYNC_TIP;
            }
        }
        return;
    }

    int frame_y = (line < 313) ? ((line - 23) << 1) : (((line - 336) << 1) + 1);
    if (frame_y < 0 || frame_y >= 576) {
        fast_dma_copy(dst32, base32, words);
        return;
    }

    int copy_dx = center_copy_dx[frame_y];
    int outline_dx = center_mask_dx[frame_y];
    int center_x = 645 - s_mod;
    int max_x = (int)(words << 2);
    uint8_t *dst8 = (uint8_t *)dst32;

    int l_start = 0, l_end = -1, r_start = 0, r_end = -1;
    bool draw_outline = (outline_dx > 0 && enable_center_outline);

    if (copy_dx > 0 && base32 != spec32) {
        int left_px  = center_x - copy_dx;
        int right_px = center_x + copy_dx;
        int min_px   = 186 - s_mod;
        if (left_px < min_px) left_px = min_px;
        if (right_px > max_x) right_px = max_x;

        int left_word  = left_px >> 2;
        int right_word = (right_px + 3) >> 2;
        if (right_word > (int)words) right_word = (int)words;

        int center_len = right_word - left_word;
        int right_len  = (int)words - right_word;

        if (left_word > 0) fast_dma_start(dst32, base32, (uint32_t)left_word);
        if (draw_outline) {
            int prev_dx = (frame_y > 0) ? center_mask_dx[frame_y - 1] : 0;
            int next_dx = (frame_y < 575) ? center_mask_dx[frame_y + 1] : 0;
            int inner_dx = outline_dx;
            if (prev_dx < inner_dx) inner_dx = prev_dx;
            if (next_dx < inner_dx) inner_dx = next_dx;
            if (outline_dx - inner_dx < 3) inner_dx = outline_dx - 3;
            l_start = center_x - outline_dx;
            l_end   = center_x - inner_dx;
            r_start = center_x + inner_dx;
            r_end   = center_x + outline_dx;
        }
        if (left_word > 0) dma_channel_wait_for_finish_blocking(dma_chan_compose);

        if (center_len > 0) fast_dma_copy(dst32 + left_word, spec32 + left_word, (uint32_t)center_len);

        if (right_len > 0) fast_dma_start(dst32 + right_word, base32 + right_word, (uint32_t)right_len);
        uint8_t *base8 = (uint8_t *)base32;
        for (int x = left_word << 2; x < left_px; x++) dst8[x] = base8[x];
        if (right_len > 0) dma_channel_wait_for_finish_blocking(dma_chan_compose);
        for (int x = right_px; x < (right_word << 2); x++) dst8[x] = base8[x];
    } else {
        fast_dma_start(dst32, base32, words);
        if (draw_outline) {
            int prev_dx = (frame_y > 0) ? center_mask_dx[frame_y - 1] : 0;
            int next_dx = (frame_y < 575) ? center_mask_dx[frame_y + 1] : 0;
            int inner_dx = outline_dx;
            if (prev_dx < inner_dx) inner_dx = prev_dx;
            if (next_dx < inner_dx) inner_dx = next_dx;
            if (outline_dx - inner_dx < 3) inner_dx = outline_dx - 3;
            l_start = center_x - outline_dx;
            l_end   = center_x - inner_dx;
            r_start = center_x + inner_dx;
            r_end   = center_x + outline_dx;
        }
        dma_channel_wait_for_finish_blocking(dma_chan_compose);
    }

    if (copy_dx == 420 && frame_y >= 260 && frame_y <= 315) {
        if (frame_y != 287 && frame_y != 288) {
            int b_start = 225 - s_mod;
            int y_offset = frame_y - 260;
            int x_rel = 419 - ((y_offset * 1950) >> 8);
            for (int w = -1; w <= 1; w++) {
                int px_left = b_start + x_rel + w;
                int px_right = b_start + 420 + x_rel + w;
                if (px_left >= b_start && px_left < b_start + 420) dst8[px_left] = WHITE_LEVEL;
                if (px_right >= b_start + 420 && px_right < b_start + 840) dst8[px_right] = BLACK_LEVEL;
            }
            dst8[center_x - 1] = BLACK_LEVEL; dst8[center_x] = WHITE_LEVEL;
        }
    }

    if (draw_outline) {
        uint32_t w32 = WHITE_LEVEL | (WHITE_LEVEL << 8) | (WHITE_LEVEL << 16) | (WHITE_LEVEL << 24);
        int min_x = 186 - s_mod;

        if (l_start > min_x && l_end < max_x && l_end >= l_start) {
            while (l_start <= l_end && (l_start & 3) != 0) dst8[l_start++] = WHITE_LEVEL;
            while (l_start + 3 <= l_end) { *(uint32_t*)(dst8 + l_start) = w32; l_start += 4; }
            while (l_start <= l_end) dst8[l_start++] = WHITE_LEVEL;
        }

        if (r_start > min_x && r_end < max_x && r_end >= r_start) {
            while (r_start <= r_end && (r_start & 3) != 0) dst8[r_start++] = WHITE_LEVEL;
            while (r_start + 3 <= r_end) { *(uint32_t*)(dst8 + r_start) = w32; r_start += 4; }
            while (r_start <= r_end) dst8[r_start++] = WHITE_LEVEL;
        }
    }
}

void build_audio_lut() {
    for (int i = 0; i < 64; i++) {
        audio_lut[i] = (uint16_t)(138.0f + 138.0f * sinf(2.0f * 3.14159265f * (float)i / 64.0f));
    }
}

// --- CORE 1 ISR: Hardware Compositor driven strictly by PIO completion ---
void __not_in_flash_func(dma_isr)() {
    if (dma_hw->ints0 & (1u << dma_chan_ping)) {
        dma_hw->ints0 = 1u << dma_chan_ping;
        uint32_t words = ((current_line & 3) == 3) ? 283 : 284;
        dma_channel_set_read_addr(dma_chan_ping, ping_buf, false);
        dma_channel_set_trans_count(dma_chan_ping, words, false);
        compose_line((uint32_t*)ping_buf, current_line, current_frame_line);
        if (++current_line == 2500) current_line = 0;
        if (++current_frame_line == 625) current_frame_line = 0;
    }
    if (dma_hw->ints0 & (1u << dma_chan_pong)) {
        dma_hw->ints0 = 1u << dma_chan_pong;
        uint32_t words = ((current_line & 3) == 3) ? 283 : 284;
        dma_channel_set_read_addr(dma_chan_pong, pong_buf, false);
        dma_channel_set_trans_count(dma_chan_pong, words, false);
        compose_line((uint32_t*)pong_buf, current_line, current_frame_line);
        if (++current_line == 2500) current_line = 0;
        if (++current_frame_line == 625) current_frame_line = 0;
    }
}

// Infinite low-latency RAM sleep loop so Core 1 never touches XIP Flash
void __not_in_flash_func(core1_video_loop)() {
    while (1) {
        __wfi();
    }
}

void core1_video_main() {
    init_compose_dma();

    compose_line((uint32_t*)ping_buf, 0, 0);
    compose_line((uint32_t*)pong_buf, 1, 1);
    current_line = 2;
    current_frame_line = 2;

    dma_chan_ping = dma_claim_unused_channel(true);
    dma_chan_pong = dma_claim_unused_channel(true);

    dma_channel_config c_ping = dma_channel_get_default_config(dma_chan_ping);
    channel_config_set_transfer_data_size(&c_ping, DMA_SIZE_32);
    channel_config_set_read_increment(&c_ping, true);
    channel_config_set_write_increment(&c_ping, false);
    channel_config_set_dreq(&c_ping, pio_get_dreq(video_pio, video_sm, true));
    channel_config_set_chain_to(&c_ping, dma_chan_pong);
    channel_config_set_high_priority(&c_ping, true);

    dma_channel_configure(
        dma_chan_ping, &c_ping,
        &video_pio->txf[video_sm], ping_buf, 284, false
    );

    dma_channel_config c_pong = dma_channel_get_default_config(dma_chan_pong);
    channel_config_set_transfer_data_size(&c_pong, DMA_SIZE_32);
    channel_config_set_read_increment(&c_pong, true);
    channel_config_set_write_increment(&c_pong, false);
    channel_config_set_dreq(&c_pong, pio_get_dreq(video_pio, video_sm, true));
    channel_config_set_chain_to(&c_pong, dma_chan_ping);
    channel_config_set_high_priority(&c_pong, true);

    dma_channel_configure(
        dma_chan_pong, &c_pong,
        &video_pio->txf[video_sm], pong_buf, 284, false
    );

    dma_channel_set_irq0_enabled(dma_chan_ping, true);
    dma_channel_set_irq0_enabled(dma_chan_pong, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_isr);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_start(dma_chan_ping);
    core1_video_loop();
}

int main() {
    gpio_init(DEBUG_LED_PIN);
    gpio_set_dir(DEBUG_LED_PIN, GPIO_OUT);
    gpio_put(DEBUG_LED_PIN, 1);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    build_audio_lut();
    build_pm5544();
    build_ebu_bars();
    build_teletext_vbi();

    // --- AUDIO PWM & DMA ENGINE SETUP ---
    gpio_set_function(DAC_AUDIO_PIN, GPIO_FUNC_PWM);
    uint audio_slice = pwm_gpio_to_slice_num(DAC_AUDIO_PIN);
    uint audio_chan = pwm_gpio_to_channel(DAC_AUDIO_PIN);

    pwm_config audio_cfg = pwm_get_default_config();
    pwm_config_set_wrap(&audio_cfg, 276);
    pwm_init(audio_slice, &audio_cfg, true);

    int dma_audio_data = dma_claim_unused_channel(true);
    int dma_audio_ctrl = dma_claim_unused_channel(true);

    volatile uint16_t *pwm_cc_addr = ((volatile uint16_t *)&pwm_hw->slice[audio_slice].cc) + audio_chan;

    dma_channel_config ac_ctrl = dma_channel_get_default_config(dma_audio_ctrl);
    channel_config_set_transfer_data_size(&ac_ctrl, DMA_SIZE_32);
    channel_config_set_read_increment(&ac_ctrl, false);
    channel_config_set_write_increment(&ac_ctrl, false);

    dma_channel_configure(
        dma_audio_ctrl, &ac_ctrl,
        &dma_hw->ch[dma_audio_data].al3_read_addr_trig,
        &audio_ptr,
        1, false
    );

    dma_channel_config ac_data = dma_channel_get_default_config(dma_audio_data);
    channel_config_set_transfer_data_size(&ac_data, DMA_SIZE_16);
    channel_config_set_read_increment(&ac_data, true);
    channel_config_set_write_increment(&ac_data, false);
    channel_config_set_dreq(&ac_data, pwm_get_dreq(audio_slice));
    channel_config_set_chain_to(&ac_data, dma_audio_ctrl);

    dma_channel_configure(
        dma_audio_data, &ac_data,
        pwm_cc_addr, audio_lut, 64, false
    );

    dma_channel_start(dma_audio_data);

    gpio_set_function(EXT_CLOCK_PIN, GPIO_FUNC_GPCK);
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0,
                    17734475, 17734475);

    uint offset = pio_add_program(video_pio, &video_out_program);
    video_out_program_init(video_pio, video_sm, offset, DAC_BASE_PIN);
    video_pio->sm[video_sm].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;

    // Launch dedicated zero-jitter video compositor on Core 1
    __dmb();
    multicore_launch_core1(core1_video_main);

    int current_card = 0;
    bool last_button_state = true;
    uint32_t last_led_time = time_us_32();
    uint32_t last_debounce_time = time_us_32();
    bool led_state = false;

    while (1) {
        uint32_t current_time = time_us_32();
        bool current_button_state = gpio_get(BUTTON_PIN);

        if (current_button_state == false) {
            gpio_put(DEBUG_LED_PIN, 1);
        } else {
            if (current_time - last_led_time > 500000) {
                led_state = !led_state;
                gpio_put(DEBUG_LED_PIN, led_state);
                last_led_time = current_time;
            }
        }

        if (last_button_state == true && current_button_state == false) {
            if (current_time - last_debounce_time > 250000) {
                current_card++;
                if (current_card > 7) current_card = 0;

                switching_card = true;
                __dmb();
                while (!video_blanked) tight_loop_contents();

                if (current_card == 0) build_ebu_bars();
                else if (current_card == 1) build_crosshatch();
                else if (current_card == 2) build_checkerboard();
                else if (current_card == 3) build_multiburst();
                else if (current_card == 4) build_pm5544();
                else if (current_card == 5) build_fubk();
                else if (current_card == 6) build_vg1001_bnt();
                else if (current_card == 7) build_ueit();

                __dmb();
                switching_card = false;

                last_debounce_time = current_time;
            }
        }

        last_button_state = current_button_state;
    }
}
