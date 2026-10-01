#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/pll.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "video.pio.h"

// Hardware Pin Definitions
#define DAC_BASE_PIN 0
#define DAC_AUDIO_PIN 15
#define EXT_CLOCK_PIN 20
#define DEBUG_LED_PIN 25 // YD-RP2040 Blue LED for heartbeat
#define BUTTON_PIN 14 // Connect a button between GPIO 14 and Ground

// Digital voltage levels
#define SYNC_TIP 70     
#define BLACK_LEVEL 116 
#define GRAY_LEVEL 170  
#define DARK_GRAY_LEVEL 143
#define WHITE_LEVEL 224 

// Buffers 32-bit aligned to enable hyper-fast, starvation-proof DMA memory composition
__attribute__((aligned(4))) uint8_t buf_blank_eq[1136]; 
__attribute__((aligned(4))) uint8_t buf_eq_blank[1136];
__attribute__((aligned(4))) uint8_t buf_vsync[1136];
__attribute__((aligned(4))) uint8_t buf_eq[1136];
__attribute__((aligned(4))) uint8_t buf_sync_eq[1136]; 
__attribute__((aligned(4))) uint8_t buf_eq_sync[1136]; 

__attribute__((aligned(4))) uint8_t buf_blank[4][1136];
__attribute__((aligned(4))) uint8_t buf_top_bot[4][1136];

// Splitting the background into 4 strict vertical zones for the precise sidebars, plus a blank margin
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

uint8_t* base_table[2500];
uint8_t* special_table[2500];

uint16_t audio_lut[64];
uint16_t *audio_ptr = audio_lut; // Global pointer required for the Control DMA

__attribute__((aligned(4))) uint8_t raw_ping_buf[1136];
__attribute__((aligned(4))) uint8_t raw_pong_buf[1144];
uint8_t *ping_buf = raw_ping_buf;
uint8_t *pong_buf = raw_pong_buf + 4;

__attribute__((aligned(4))) int circle_dx[576]; 
__attribute__((aligned(4))) int center_mask_dx[576]; 
__attribute__((aligned(4))) int center_copy_dx[576]; // NEW: Decouples rectangular patterns from the circular outline
bool enable_center_outline = false;

int dma_chan_ping;
int dma_chan_pong;
int dma_chan_compose; // 3rd DMA channel handles circle composition
dma_channel_config c_compose;
volatile int current_line = 0;

const int phase_offset[4] = {0, 0, 0, 0}; 
const bool v_inverted[4]  = {false, true, false, true};

#include "pm5544.c"
#include "fubk.c"
#include "vg1001_bnt.c"
#include "ueit.c"
#include "simple_patterns.c"

void init_compose_dma() {
    dma_chan_compose = dma_claim_unused_channel(true);
    c_compose = dma_channel_get_default_config(dma_chan_compose);
    channel_config_set_transfer_data_size(&c_compose, DMA_SIZE_32); // Move 4 pixels per cycle
    channel_config_set_read_increment(&c_compose, true);
    channel_config_set_write_increment(&c_compose, true);
    channel_config_set_high_priority(&c_compose, false); // Yield to PIO output DMA!
}

// --- HYPER-FAST HARDWARE DMA COMPOSITOR ---
void __not_in_flash_func(compose_line)(uint32_t *dst32, int line_idx) {
    uint32_t *base32 = (uint32_t *)base_table[line_idx];
    uint32_t *spec32 = (uint32_t *)special_table[line_idx];

    // Decode continuous 2500-frame tracking back into standard 625 physical lines
    int line = (line_idx % 625) + 1;
    
    if (line < 23 || (line > 310 && line < 336) || line > 623) {
        dma_channel_configure(dma_chan_compose, &c_compose, dst32, base32, 284, true);
        dma_channel_wait_for_finish_blocking(dma_chan_compose);
        return;
    }

    int frame_y = (line < 313) ? (line - 23) * 2 : (line - 336) * 2 + 1;
    if (frame_y < 0 || frame_y >= 576) {
        dma_channel_configure(dma_chan_compose, &c_compose, dst32, base32, 284, true);
        dma_channel_wait_for_finish_blocking(dma_chan_compose);
        return;
    }

    int copy_dx = center_copy_dx[frame_y];
    int outline_dx = center_mask_dx[frame_y];

    // 1. Copy the inner pattern
    if (copy_dx > 0 && base32 != spec32) {
        int left_word = (645 - copy_dx) / 4;
        int right_word = (645 + copy_dx) / 4 + 1;
        if (left_word < 47) left_word = 47; 
        if (right_word > 284) right_word = 284;
        
        int center_len = right_word - left_word;
        int right_len = 284 - right_word;

        if (left_word > 0) {
            dma_channel_configure(dma_chan_compose, &c_compose, dst32, base32, left_word, true);
            dma_channel_wait_for_finish_blocking(dma_chan_compose);
        }
        if (center_len > 0) {
            dma_channel_configure(dma_chan_compose, &c_compose, dst32 + left_word, spec32 + left_word, center_len, true);
            dma_channel_wait_for_finish_blocking(dma_chan_compose);
        }
        if (right_len > 0) {
            dma_channel_configure(dma_chan_compose, &c_compose, dst32 + right_word, base32 + right_word, right_len, true);
            dma_channel_wait_for_finish_blocking(dma_chan_compose);
        }
    } else {
        dma_channel_configure(dma_chan_compose, &c_compose, dst32, base32, 284, true);
        dma_channel_wait_for_finish_blocking(dma_chan_compose);
    }

    uint8_t *dst8 = (uint8_t *)dst32;

    if (copy_dx == 420 && frame_y >= 260 && frame_y <= 315) {
        if (frame_y != 287 && frame_y != 288) {
            int b_start = 225;
            int y_offset = frame_y - 260; 
            int x_rel = 419 - ((y_offset * 1950) >> 8); 
            
            for (int w = -1; w <= 1; w++) { 
                int px_left = b_start + x_rel + w;
                int px_right = b_start + 420 + x_rel + w;
                if (px_left >= b_start && px_left < b_start + 420) dst8[px_left] = WHITE_LEVEL;
                if (px_right >= b_start + 420 && px_right < b_start + 840) dst8[px_right] = BLACK_LEVEL;
            }
            dst8[644] = BLACK_LEVEL; dst8[645] = WHITE_LEVEL;
        }
    }

    // 2. Draw the continuous, gap-free circular outline
    if (outline_dx > 0 && enable_center_outline) {
        int prev_dx = (frame_y > 0) ? center_mask_dx[frame_y - 1] : 0;
        int next_dx = (frame_y < 575) ? center_mask_dx[frame_y + 1] : 0;
        
        int inner_dx = outline_dx;
        if (prev_dx < inner_dx) inner_dx = prev_dx;
        if (next_dx < inner_dx) inner_dx = next_dx;
        
        if (outline_dx - inner_dx < 3) inner_dx = outline_dx - 3;
        
        int l_start = 645 - outline_dx;
        int l_end   = 645 - inner_dx;
        if (l_start > 186 && l_end < 1136 && l_end >= l_start) {
            for (int x = l_start; x <= l_end; x++) dst8[x] = WHITE_LEVEL;
        }
        
        int r_start = 645 + inner_dx;
        int r_end   = 645 + outline_dx;
        if (r_start > 186 && r_end < 1136 && r_end >= r_start) {
            for (int x = r_start; x <= r_end; x++) dst8[x] = WHITE_LEVEL;
        }
    }
}


void build_audio_lut() {
    for (int i = 0; i < 64; i++) {
        audio_lut[i] = (uint16_t)(138.0f + 138.0f * sinf(2.0f * 3.14159265f * (float)i / 64.0f));
    }
}

// --- ISR: Hardware Compositor driven strictly by PIO completion ---
void __not_in_flash_func(dma_isr)() {
    if (dma_hw->ints0 & (1u << dma_chan_ping)) {
        dma_hw->ints0 = 1u << dma_chan_ping;
        dma_channel_set_read_addr(dma_chan_ping, ping_buf, false);
        compose_line((uint32_t*)ping_buf, current_line); 
        current_line++;
        if (current_line == 2500) current_line = 0; // Wraps smoothly for 4-frame PAL sequence
    }
    if (dma_hw->ints0 & (1u << dma_chan_pong)) {
        dma_hw->ints0 = 1u << dma_chan_pong;
        dma_channel_set_read_addr(dma_chan_pong, pong_buf, false);
        compose_line((uint32_t*)pong_buf, current_line); 
        current_line++;
        if (current_line == 2500) current_line = 0; 
    }
}

int main() {
    // 1. INSTANT DIAGNOSTIC LED 
    gpio_init(DEBUG_LED_PIN);
    gpio_set_dir(DEBUG_LED_PIN, GPIO_OUT);
    gpio_put(DEBUG_LED_PIN, 1);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    build_audio_lut();
    build_pm5544();
    build_ebu_bars();
    
    // --- AUDIO PWM & DMA ENGINE SETUP ---
    gpio_set_function(DAC_AUDIO_PIN, GPIO_FUNC_PWM);
    uint audio_slice = pwm_gpio_to_slice_num(DAC_AUDIO_PIN);
    uint audio_chan = pwm_gpio_to_channel(DAC_AUDIO_PIN); // Figures out if it's channel A or B

    pwm_config audio_cfg = pwm_get_default_config();
    pwm_config_set_wrap(&audio_cfg, 276); // Exactly maps to a 64kHz sample rate
    pwm_init(audio_slice, &audio_cfg, true);

    // Setup Audio DMA (100% Hardware driven infinite loop)
    int dma_audio_data = dma_claim_unused_channel(true);
    int dma_audio_ctrl = dma_claim_unused_channel(true);

    // Pointer magic: Point exactly to the 16-bit half-word of the CC register
    volatile uint16_t *pwm_cc_addr = ((volatile uint16_t *)&pwm_hw->slice[audio_slice].cc) + audio_chan;

    // --- CONTROL CHANNEL ---
    // This channel wakes up, writes the starting memory address of the audio table
    // into the Data channel's trigger register, and goes back to sleep.
    dma_channel_config ac_ctrl = dma_channel_get_default_config(dma_audio_ctrl);
    channel_config_set_transfer_data_size(&ac_ctrl, DMA_SIZE_32); 
    channel_config_set_read_increment(&ac_ctrl, false);
    channel_config_set_write_increment(&ac_ctrl, false);
    
    dma_channel_configure(
        dma_audio_ctrl, &ac_ctrl,
        &dma_hw->ch[dma_audio_data].al3_read_addr_trig, // Writing here triggers the Data channel
        &audio_ptr, // The memory location holding the start address
        1, false
    );

    // --- DATA CHANNEL ---
    // This channel feeds 64 samples to the PWM at 64 kHz. 
    // When it finishes, it chains to the Control Channel to be reset.
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

    // Start the infinite loop
    dma_channel_start(dma_audio_data);
    // ------------------------------------

    gpio_set_function(EXT_CLOCK_PIN, GPIO_FUNC_GPCK);

    clock_configure(clk_sys, 
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX, 
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0, 
                    17734475, 17734475);

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &video_out_program);
    video_out_program_init(pio, sm, offset, DAC_BASE_PIN);

    // CRITICAL FIX FOR 32-BIT MEMORY PACKING:
    // This allows the PIO to pull exactly 32 bits from RAM and shift out 4 perfect 8-bit pixels.
    // Without this, the DMA channel starves and horizontal tearing occurs!
    pio->sm[sm].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;

    // INITIALIZE THE DMA COMPOSITOR
    init_compose_dma();

    // Pre-compose the first two lines into the hardware buffers
    compose_line((uint32_t*)ping_buf, 0);
    compose_line((uint32_t*)pong_buf, 1);
    current_line = 2; 

    dma_chan_ping = dma_claim_unused_channel(true);
    dma_chan_pong = dma_claim_unused_channel(true);

    // Configure Ping Video Output
    dma_channel_config c_ping = dma_channel_get_default_config(dma_chan_ping);
    channel_config_set_transfer_data_size(&c_ping, DMA_SIZE_32);
    channel_config_set_read_increment(&c_ping, true);
    channel_config_set_write_increment(&c_ping, false);
    channel_config_set_dreq(&c_ping, pio_get_dreq(pio, sm, true)); 
    channel_config_set_chain_to(&c_ping, dma_chan_pong);
    channel_config_set_high_priority(&c_ping, true); // Keep video output priority high

    dma_channel_configure(
        dma_chan_ping, &c_ping,
        &pio->txf[sm], ping_buf, 284, false 
    );

    // Configure Pong Video Output
    dma_channel_config c_pong = dma_channel_get_default_config(dma_chan_pong);
    channel_config_set_transfer_data_size(&c_pong, DMA_SIZE_32);
    channel_config_set_read_increment(&c_pong, true);
    channel_config_set_write_increment(&c_pong, false);
    channel_config_set_dreq(&c_pong, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&c_pong, dma_chan_ping);
    channel_config_set_high_priority(&c_pong, true); // Keep video output priority high

    dma_channel_configure(
        dma_chan_pong, &c_pong,
        &pio->txf[sm], pong_buf, 284, false 
    );

    dma_channel_set_irq0_enabled(dma_chan_ping, true);
    dma_channel_set_irq0_enabled(dma_chan_pong, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_isr);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_start(dma_chan_ping);
    
    int current_card = 0;
    bool last_button_state = true; // Assume unpressed (pulled HIGH)
    uint32_t last_led_time = time_us_32();
    uint32_t last_debounce_time = time_us_32();
    bool led_state = false;

    while (1) {
        uint32_t current_time = time_us_32();
        bool current_button_state = gpio_get(BUTTON_PIN); // Read physical pin

        // 1. HARDWARE PROBE: LED turns solid ON when button is physically pressed
        if (current_button_state == false) {
            gpio_put(DEBUG_LED_PIN, 1);
        } else {
            // Normal 500ms heartbeat when button is released
            if (current_time - last_led_time > 500000) {
                led_state = !led_state;
                gpio_put(DEBUG_LED_PIN, led_state);
                last_led_time = current_time;
            }
        }

        // 2. True Edge-Detection Button Polling
        if (last_button_state == true && current_button_state == false) {
            if (current_time - last_debounce_time > 250000) {
                current_card++;
                if (current_card > 7) current_card = 0; // Extended to 8 cards
                
                if (current_card == 0) build_ebu_bars();
                else if (current_card == 1) build_crosshatch();
                else if (current_card == 2) build_checkerboard();
                else if (current_card == 3) build_multiburst();
                else if (current_card == 4) build_pm5544();
                else if (current_card == 5) build_fubk(); 
                else if (current_card == 6) build_vg1001_bnt();
                else if (current_card == 7) build_ueit();
                
                last_debounce_time = current_time; 
            }
        }
        
        last_button_state = current_button_state; 
    }
}
