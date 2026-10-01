// 3-Tap FIR Low-Pass Filter 
void apply_luma_lpf(uint8_t *buf, int passes) {
    uint8_t temp[1136];
    for (int p = 0; p < passes; p++) {
        memcpy(temp, buf, 1136);
        for (int x = 188; x < 1102; x++) {
            buf[x] = (temp[x-1] + (temp[x] << 1) + temp[x+1]) >> 2;
        }
    }
}

void generate_hblank(uint8_t *buffer, int s_mod) {
    int idx = 0;
    memset(&buffer[idx], SYNC_TIP, 84); idx += 84; 
    memset(&buffer[idx], BLACK_LEVEL, 16); idx += 16; 
    
    int phase_off = phase_offset[s_mod];
    bool is_odd = v_inverted[s_mod];
    
    for (int i = 0; i < 40; i++) {
        int x = idx; 
        int phase = (x + phase_off) % 4; 
        int burst_val = 0;
        
        if (is_odd) {
            if (phase == 0) burst_val = -23;
            else if (phase == 1) burst_val = -23;
            else if (phase == 2) burst_val = 23;
            else if (phase == 3) burst_val = 23;
        } else {
            if (phase == 0) burst_val = -23;
            else if (phase == 1) burst_val = 23;
            else if (phase == 2) burst_val = 23;
            else if (phase == 3) burst_val = -23;
        }
        buffer[idx++] = BLACK_LEVEL + burst_val;
    }
    memset(&buffer[idx], BLACK_LEVEL, 186 - idx);
}

void build_template(uint8_t *buffer, int s_mod, bool is_tb_zone, bool is_h_grid, bool cast_y_even) {
    generate_hblank(buffer, s_mod);
    
    for (int x = 186; x < 1136; x++) {
        if (x < 189 || x >= 1101) {
            buffer[x] = BLACK_LEVEL;
            continue;
        }

        int cell_x = (x - 189) / 48;       
        int mod_x = (x - 189) % 48;        
        bool is_v_grid = (mod_x == 0 || mod_x == 47); 

        bool is_left_zone = (cell_x == 0);
        bool is_right_zone = (cell_x == 18);

        if (is_tb_zone) {
            buffer[x] = (cell_x % 2 == 0) ? WHITE_LEVEL : BLACK_LEVEL;
        } else if (is_left_zone || is_right_zone) {
            buffer[x] = cast_y_even ? WHITE_LEVEL : BLACK_LEVEL;
        } else {
            if (is_v_grid || is_h_grid) {
                buffer[x] = WHITE_LEVEL;
            } else {
                buffer[x] = GRAY_LEVEL;
            }
        }
    }
}

// Side color injection mapping inside grid cells
void fill_side_color(uint8_t *buf, int cell_x, int Y, int U, int V, int s_mod, bool over_left, bool over_right) {
    int start_x = 189 + cell_x * 48;
    int phase_off = phase_offset[s_mod];
    bool is_odd = v_inverted[s_mod];
    
    // Expand the loop bounds if the override booleans are true
    int start_i = over_left ? 0 : 1;
    int end_i = over_right ? 48 : 47;

    for (int i = start_i; i < end_i; i++) { 
        int x = start_x + i;
        int phase = (x + phase_off) % 4;
        int chroma = 0;
        
        if (is_odd) {
            if (phase == 0) chroma = U;
            else if (phase == 1) chroma = -V; 
            else if (phase == 2) chroma = -U;
            else if (phase == 3) chroma = V;
        } else {
            if (phase == 0) chroma = U;
            else if (phase == 1) chroma = V;
            else if (phase == 2) chroma = -U;
            else if (phase == 3) chroma = -V;
        }
        
        int pixel_val = Y + chroma;
        if (pixel_val < BLACK_LEVEL) pixel_val = BLACK_LEVEL;
        if (pixel_val > WHITE_LEVEL) pixel_val = WHITE_LEVEL;
        
        buf[x] = (uint8_t)pixel_val;
    }
}

void fill_color_bar(uint8_t *buf, int start, int width, int Y, int U, int V, int s_mod) {
    int phase_off = phase_offset[s_mod];
    bool is_odd = v_inverted[s_mod];
    
    for (int x = start; x < start + width; x++) {
        int phase = (x + phase_off) % 4;
        int chroma = 0;
        
        if (is_odd) {
            if (phase == 0) chroma = U;
            else if (phase == 1) chroma = -V; 
            else if (phase == 2) chroma = -U;
            else if (phase == 3) chroma = V;
        } else {
            if (phase == 0) chroma = U;
            else if (phase == 1) chroma = V;
            else if (phase == 2) chroma = -U;
            else if (phase == 3) chroma = -V;
        }
        buf[x] = Y + chroma;
    }
}

void overlay_center_cross(uint8_t *buf, bool is_mid) {
    for (int x = 357; x <= 933; x++) {
        if (x >= 643 && x <= 646) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 379 && x <= 382) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 427 && x <= 430) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 475 && x <= 478) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 523 && x <= 526) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 571 && x <= 574) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 619 && x <= 622) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 667 && x <= 670) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 715 && x <= 718) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 763 && x <= 766) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 811 && x <= 814) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 859 && x <= 862) {
            buf[x] = WHITE_LEVEL; 
        } else if (x >= 907 && x <= 910) {
            buf[x] = WHITE_LEVEL; 
        } else if (is_mid) {
            buf[x] = WHITE_LEVEL; 
        } else {
            buf[x] = BLACK_LEVEL; 
        }
    }
}

void overlay_sqwave(uint8_t *buf) {
    int bar_start = 357;
    int total_w = 96 * 6; 
    float period_pixels = 17734475.0f / 250000.0f; 

    for (int x = bar_start; x < bar_start + total_w; x++) {
        float phase = fmodf((float)(x - bar_start), period_pixels);
        buf[x] = (phase < (period_pixels / 2.0f)) ? 197 : BLACK_LEVEL; 
    }
}

void overlay_grayscale(uint8_t *buf) {
    int bar_start = 357;
    int block_w = 96;
    int levels[6] = {116, 138, 159, 181, 202, 224}; 
    
    for (int b = 0; b < 6; b++) {
        int start_x = bar_start + (b * block_w);
        for (int i = 0; i < block_w; i++) {
            buf[start_x + i] = levels[b];
        }
    }
}

void overlay_top_reflection(uint8_t *buf) {
    int bar_start = 357;
    int total_w = 96 * 6;  
    int flank_w = 144;     
    int center_w = 288;    

    for (int x = bar_start; x < bar_start + total_w; x++) {
        if (x >= bar_start + flank_w && x < bar_start + flank_w + center_w) {
            if (x >= bar_start + flank_w + 22 && x <= bar_start + flank_w + 25) {
                buf[x] = BLACK_LEVEL; 
            } else {
                buf[x] = WHITE_LEVEL;
            }
        } else {
            buf[x] = BLACK_LEVEL;
        }
    }
}

void overlay_bot_reflection(uint8_t *buf) {
    int bar_start = 357;
    int total_w = 96 * 6;  
    int flank_w = 144;     
    int center_w = 288;    

    for (int x = bar_start; x < bar_start + total_w; x++) {
        if (x >= bar_start + flank_w && x < bar_start + flank_w + center_w) {
            if (x >= bar_start + flank_w + 22 && x <= bar_start + flank_w + 25) {
                buf[x] = WHITE_LEVEL; 
            } else {
                buf[x] = BLACK_LEVEL;
            }
        } else {
            buf[x] = WHITE_LEVEL;
        }
    }
}

void overlay_gratings(uint8_t *buf) {
    float freqs[5] = {0.8f, 1.8f, 2.8f, 3.8f, 4.8f}; 
    int block_w = 96;    
    int bar_start = 357; 
    int total_w = 96 * 6; 
    
    for (int x = bar_start; x < bar_start + total_w; x++) {
        buf[x] = BLACK_LEVEL;
    }

    int centered_start = bar_start + 48;
    
    for (int b = 0; b < 5; b++) {
        float phase_inc = (freqs[b] / 17.734475f) * 2.0f * 3.14159265f;
        int start_x = centered_start + (b * block_w);
        
        for (int i = 0; i < block_w; i++) {
            int x = start_x + i;
            float sine_val = sinf(i * phase_inc); 
            
            int pixel_val = 170 + (int)(sine_val * 54.0f); 
            
            if (pixel_val < BLACK_LEVEL) pixel_val = BLACK_LEVEL;
            if (pixel_val > WHITE_LEVEL) pixel_val = WHITE_LEVEL;
            
            buf[x] = (uint8_t)pixel_val;
        }
    }
}

void overlay_top_lf_white(uint8_t *buf) {
    int bar_start = 357;
    int total_w = 96 * 6; 
    for (int x = bar_start; x < bar_start + total_w; x++) {
        buf[x] = WHITE_LEVEL;
    }
}

void overlay_top_lf_black(uint8_t *buf) {
    int bar_start = 357;
    int total_w = 96 * 6; 
    int margin = 192; 
    for (int x = bar_start; x < bar_start + total_w; x++) {
        if (x >= bar_start + margin && x < bar_start + total_w - margin) {
            buf[x] = BLACK_LEVEL;
        } else {
            buf[x] = WHITE_LEVEL;
        }
    }
}

void overlay_yc_luma(uint8_t *buf) {
    int yc_start = 357;
    int center_red_w = 48; 
    int yellow_w = 264;    
    for (int x = yc_start; x < yc_start + yellow_w; x++) buf[x] = 188; 
    for (int x = yc_start + yellow_w; x < yc_start + yellow_w + center_red_w; x++) buf[x] = 137; 
    for (int x = yc_start + yellow_w + center_red_w; x < yc_start + yellow_w * 2 + center_red_w; x++) buf[x] = 188; 
}

void overlay_center_ext(uint8_t *buf) {
    for (int x = 621; x < 669; x++) {
        if (x >= 643 && x <= 646) {
            buf[x] = WHITE_LEVEL; // White center line
        } else {
            buf[x] = BLACK_LEVEL; // Black bracket background
        }
    }
}

void build_pm5544() {
    enable_center_outline = false;
    for (int y = 0; y < 576; y++) {
        if (y >= 60 && y <= 516) {
            float dy = (y - 288) / 228.0f;
            int val = (int)(288.0f * sqrtf(1.0f - dy * dy));
            center_mask_dx[y] = val;
            center_copy_dx[y] = val; // PM5544 patterns are bound to the circle
        } else {
            center_mask_dx[y] = 0;
            center_copy_dx[y] = 0;
        }
    }
    
    memset(&buf_vsync[0], SYNC_TIP, 484); memset(&buf_vsync[484], BLACK_LEVEL, 84);
    memset(&buf_vsync[568], SYNC_TIP, 484); memset(&buf_vsync[1052], BLACK_LEVEL, 84);

    memset(&buf_eq[0], SYNC_TIP, 42); memset(&buf_eq[42], BLACK_LEVEL, 526);
    memset(&buf_eq[568], SYNC_TIP, 42); memset(&buf_eq[610], BLACK_LEVEL, 526);

    memset(&buf_sync_eq[0], SYNC_TIP, 484); memset(&buf_sync_eq[484], BLACK_LEVEL, 84);
    memset(&buf_sync_eq[568], SYNC_TIP, 42); memset(&buf_sync_eq[610], BLACK_LEVEL, 526);

    memset(&buf_eq_sync[0], SYNC_TIP, 42); memset(&buf_eq_sync[42], BLACK_LEVEL, 526);
    memset(&buf_eq_sync[568], SYNC_TIP, 484); memset(&buf_eq_sync[1052], BLACK_LEVEL, 84);
    // Line 623: H-Sync + Blanking (1st half), Equalizing Pulse (2nd half)
    memset(buf_blank_eq, BLACK_LEVEL, 1136);
    generate_hblank(buf_blank_eq, 0); 
    memset(&buf_blank_eq[568], SYNC_TIP, 42); 

    // Line 318: Equalizing Pulse (1st half), Black Blanking (2nd half)
    memset(buf_eq_blank, BLACK_LEVEL, 1136);
    memset(&buf_eq_blank[0], SYNC_TIP, 42);

    for (int s_mod = 0; s_mod < 4; s_mod++) {
        generate_hblank(buf_blank[s_mod], s_mod); 
        memset(&buf_blank[s_mod][186], BLACK_LEVEL, 1136 - 186);

        build_template(buf_top_bot[s_mod], s_mod, true, false, false);
        
        // 1. Initialize the new zone backgrounds
        build_template(buf_bg_blank_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_bg_blank_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_bg_z1_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_bg_z1_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_bg_z2_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_bg_z2_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_bg_z3_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_bg_z3_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_bg_z4_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_bg_z4_cast1[s_mod], s_mod, false, false, false);
        // 1b. Initialize the grid zone backgrounds (is_h_grid = true)
        build_template(buf_grid_blank_cast0[s_mod], s_mod, false, true, true);
        build_template(buf_grid_blank_cast1[s_mod], s_mod, false, true, false);
        build_template(buf_grid_z1_cast0[s_mod], s_mod, false, true, true);
        build_template(buf_grid_z1_cast1[s_mod], s_mod, false, true, false);
        build_template(buf_grid_z2_cast0[s_mod], s_mod, false, true, true);
        build_template(buf_grid_z2_cast1[s_mod], s_mod, false, true, false);
        build_template(buf_grid_z3_cast0[s_mod], s_mod, false, true, true);
        build_template(buf_grid_z3_cast1[s_mod], s_mod, false, true, false);
        build_template(buf_grid_z4_cast0[s_mod], s_mod, false, true, true);
        build_template(buf_grid_z4_cast1[s_mod], s_mod, false, true, false);

        // 2. Inject the Diagnostic Sidebars into both BG and GRID arrays
        
        // Zone 1: Top G-Y Rects & Top Outer Signals (cell_y = 2, 3)
        // Erase boundary between cell 2 (Right edge) and cell 3 (Left edge)
        fill_side_color(buf_bg_z1_cast0[s_mod], 2, 170, 0, -50, s_mod, false, true);
        fill_side_color(buf_bg_z1_cast1[s_mod], 2, 170, 0, -50, s_mod, false, true);
        fill_side_color(buf_grid_z1_cast0[s_mod], 2, 170, 0, -50, s_mod, false, true);
        fill_side_color(buf_grid_z1_cast1[s_mod], 2, 170, 0, -50, s_mod, false, true);
        
        fill_side_color(buf_bg_z1_cast0[s_mod], 3, 170, 36, -50, s_mod, true, false);
        fill_side_color(buf_bg_z1_cast1[s_mod], 3, 170, 36, -50, s_mod, true, false);
        fill_side_color(buf_grid_z1_cast0[s_mod], 3, 170, 36, -50, s_mod, true, false);
        fill_side_color(buf_grid_z1_cast1[s_mod], 3, 170, 36, -50, s_mod, true, false);
        
        // Erase boundary between cell 15 (Right edge) and cell 16 (Left edge)
        fill_side_color(buf_bg_z1_cast0[s_mod], 15, 170, 36, -50, s_mod, false, true);
        fill_side_color(buf_bg_z1_cast1[s_mod], 15, 170, 36, -50, s_mod, false, true);
        fill_side_color(buf_grid_z1_cast0[s_mod], 15, 170, 36, -50, s_mod, false, true);
        fill_side_color(buf_grid_z1_cast1[s_mod], 15, 170, 36, -50, s_mod, false, true);
        
        fill_side_color(buf_bg_z1_cast0[s_mod], 16, 170, -50, 0, s_mod, true, false);
        fill_side_color(buf_bg_z1_cast1[s_mod], 16, 170, -50, 0, s_mod, true, false);
        fill_side_color(buf_grid_z1_cast0[s_mod], 16, 170, -50, 0, s_mod, true, false);
        fill_side_color(buf_grid_z1_cast1[s_mod], 16, 170, -50, 0, s_mod, true, false);

        // Zone 2: Upper Mid Outer Signals (cell_y = 4, 5, 6, 7-top)
        // No adjacent blocks, preserve all outer vertical grid lines
        fill_side_color(buf_bg_z2_cast0[s_mod], 2, 170, 0, -50, s_mod, false, false);
        fill_side_color(buf_bg_z2_cast1[s_mod], 2, 170, 0, -50, s_mod, false, false);
        fill_side_color(buf_grid_z2_cast0[s_mod], 2, 170, 0, -50, s_mod, false, false);
        fill_side_color(buf_grid_z2_cast1[s_mod], 2, 170, 0, -50, s_mod, false, false);
        
        fill_side_color(buf_bg_z2_cast0[s_mod], 16, 170, -50, 0, s_mod, false, false);
        fill_side_color(buf_bg_z2_cast1[s_mod], 16, 170, -50, 0, s_mod, false, false);
        fill_side_color(buf_grid_z2_cast0[s_mod], 16, 170, -50, 0, s_mod, false, false);
        fill_side_color(buf_grid_z2_cast1[s_mod], 16, 170, -50, 0, s_mod, false, false);

        // Zone 3: Lower Mid Outer Signals (cell_y = 7-bot, 8, 9, 10)
        // No adjacent blocks, preserve all outer vertical grid lines
        fill_side_color(buf_bg_z3_cast0[s_mod], 2, 170, 0, 50, s_mod, false, false);
        fill_side_color(buf_bg_z3_cast1[s_mod], 2, 170, 0, 50, s_mod, false, false);
        fill_side_color(buf_grid_z3_cast0[s_mod], 2, 170, 0, 50, s_mod, false, false);
        fill_side_color(buf_grid_z3_cast1[s_mod], 2, 170, 0, 50, s_mod, false, false);
        
        fill_side_color(buf_bg_z3_cast0[s_mod], 16, 170, 50, 0, s_mod, false, false);
        fill_side_color(buf_bg_z3_cast1[s_mod], 16, 170, 50, 0, s_mod, false, false);
        fill_side_color(buf_grid_z3_cast0[s_mod], 16, 170, 50, 0, s_mod, false, false);
        fill_side_color(buf_grid_z3_cast1[s_mod], 16, 170, 50, 0, s_mod, false, false);

        // Zone 4: Bottom G-Y Rects & Bot Outer Signals (cell_y = 11, 12)
        // Erase boundary between cell 2 (Right edge) and cell 3 (Left edge)
        fill_side_color(buf_bg_z4_cast0[s_mod], 2, 170, 0, 50, s_mod, false, true);
        fill_side_color(buf_bg_z4_cast1[s_mod], 2, 170, 0, 50, s_mod, false, true);
        fill_side_color(buf_grid_z4_cast0[s_mod], 2, 170, 0, 50, s_mod, false, true);
        fill_side_color(buf_grid_z4_cast1[s_mod], 2, 170, 0, 50, s_mod, false, true);
        
        fill_side_color(buf_bg_z4_cast0[s_mod], 3, 170, -36, 50, s_mod, true, false);
        fill_side_color(buf_bg_z4_cast1[s_mod], 3, 170, -36, 50, s_mod, true, false);
        fill_side_color(buf_grid_z4_cast0[s_mod], 3, 170, -36, 50, s_mod, true, false);
        fill_side_color(buf_grid_z4_cast1[s_mod], 3, 170, -36, 50, s_mod, true, false);
        
        // Erase boundary between cell 15 (Right edge) and cell 16 (Left edge)
        fill_side_color(buf_bg_z4_cast0[s_mod], 15, 170, -36, 50, s_mod, false, true);
        fill_side_color(buf_bg_z4_cast1[s_mod], 15, 170, -36, 50, s_mod, false, true);
        fill_side_color(buf_grid_z4_cast0[s_mod], 15, 170, -36, 50, s_mod, false, true);
        fill_side_color(buf_grid_z4_cast1[s_mod], 15, 170, -36, 50, s_mod, false, true);
        
        fill_side_color(buf_bg_z4_cast0[s_mod], 16, 170, 50, 0, s_mod, true, false);
        fill_side_color(buf_bg_z4_cast1[s_mod], 16, 170, 50, 0, s_mod, true, false);
        fill_side_color(buf_grid_z4_cast0[s_mod], 16, 170, 50, 0, s_mod, true, false);
        fill_side_color(buf_grid_z4_cast1[s_mod], 16, 170, 50, 0, s_mod, true, false);
        
        build_template(buf_bars_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_bars_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_gratings_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_gratings_cast1[s_mod], s_mod, false, false, false);

        build_template(buf_cross_top[s_mod], s_mod, false, false, false);
        build_template(buf_cross_mid[s_mod], s_mod, false, false, false);
        build_template(buf_sqwave_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_sqwave_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_gray_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_gray_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_top_ref_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_top_ref_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_bot_ref_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_bot_ref_cast1[s_mod], s_mod, false, false, false);

        build_template(buf_top_lf_white_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_top_lf_white_cast1[s_mod], s_mod, false, false, false);
        build_template(buf_top_lf_black_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_top_lf_black_cast1[s_mod], s_mod, false, false, false);
        
        build_template(buf_yc_cast0[s_mod], s_mod, false, false, true);
        build_template(buf_yc_cast1[s_mod], s_mod, false, false, false);

        overlay_center_cross(buf_cross_top[s_mod], false);
        overlay_center_cross(buf_cross_mid[s_mod], true);
        overlay_sqwave(buf_sqwave_cast0[s_mod]);
        overlay_sqwave(buf_sqwave_cast1[s_mod]);
        overlay_grayscale(buf_gray_cast0[s_mod]);
        overlay_grayscale(buf_gray_cast1[s_mod]);
        overlay_top_reflection(buf_top_ref_cast0[s_mod]);
        overlay_top_reflection(buf_top_ref_cast1[s_mod]);
        overlay_bot_reflection(buf_bot_ref_cast0[s_mod]);
        overlay_bot_reflection(buf_bot_ref_cast1[s_mod]);

        overlay_top_lf_white(buf_top_lf_white_cast0[s_mod]);
        overlay_top_lf_white(buf_top_lf_white_cast1[s_mod]);
        overlay_top_lf_black(buf_top_lf_black_cast0[s_mod]);
        overlay_top_lf_black(buf_top_lf_black_cast1[s_mod]);

        overlay_yc_luma(buf_yc_cast0[s_mod]);
        overlay_yc_luma(buf_yc_cast1[s_mod]);

        // APPLY FIR LOW-PASS FILTER to the newly split background arrays
        apply_luma_lpf(buf_top_bot[s_mod], 2);
        apply_luma_lpf(buf_bg_blank_cast0[s_mod], 2);
        apply_luma_lpf(buf_bg_blank_cast1[s_mod], 2);
        apply_luma_lpf(buf_bg_z1_cast0[s_mod], 2);
        apply_luma_lpf(buf_bg_z1_cast1[s_mod], 2);
        apply_luma_lpf(buf_bg_z2_cast0[s_mod], 2);
        apply_luma_lpf(buf_bg_z2_cast1[s_mod], 2);
        apply_luma_lpf(buf_bg_z3_cast0[s_mod], 2);
        apply_luma_lpf(buf_bg_z3_cast1[s_mod], 2);
        apply_luma_lpf(buf_bg_z4_cast0[s_mod], 2);
        apply_luma_lpf(buf_bg_z4_cast1[s_mod], 2);
        apply_luma_lpf(buf_grid_blank_cast0[s_mod], 2);
        apply_luma_lpf(buf_grid_blank_cast1[s_mod], 2);
        apply_luma_lpf(buf_grid_z1_cast0[s_mod], 2);
        apply_luma_lpf(buf_grid_z1_cast1[s_mod], 2);
        apply_luma_lpf(buf_grid_z2_cast0[s_mod], 2);
        apply_luma_lpf(buf_grid_z2_cast1[s_mod], 2);
        apply_luma_lpf(buf_grid_z3_cast0[s_mod], 2);
        apply_luma_lpf(buf_grid_z3_cast1[s_mod], 2);
        apply_luma_lpf(buf_grid_z4_cast0[s_mod], 2);
        apply_luma_lpf(buf_grid_z4_cast1[s_mod], 2);
        apply_luma_lpf(buf_bars_cast0[s_mod], 2); 
        apply_luma_lpf(buf_bars_cast1[s_mod], 2);
        apply_luma_lpf(buf_cross_top[s_mod], 2);
        apply_luma_lpf(buf_cross_mid[s_mod], 2);
        apply_luma_lpf(buf_sqwave_cast0[s_mod], 2);
        apply_luma_lpf(buf_sqwave_cast1[s_mod], 2);
        apply_luma_lpf(buf_gray_cast0[s_mod], 2);
        apply_luma_lpf(buf_gray_cast1[s_mod], 2);
        apply_luma_lpf(buf_top_ref_cast0[s_mod], 2);
        apply_luma_lpf(buf_top_ref_cast1[s_mod], 2);
        apply_luma_lpf(buf_bot_ref_cast0[s_mod], 2);
        apply_luma_lpf(buf_bot_ref_cast1[s_mod], 2);
        apply_luma_lpf(buf_gratings_cast0[s_mod], 2);
        apply_luma_lpf(buf_gratings_cast1[s_mod], 2);
        apply_luma_lpf(buf_top_lf_white_cast0[s_mod], 2);
        apply_luma_lpf(buf_top_lf_white_cast1[s_mod], 2);
        apply_luma_lpf(buf_top_lf_black_cast0[s_mod], 2);
        apply_luma_lpf(buf_top_lf_black_cast1[s_mod], 2);
        apply_luma_lpf(buf_yc_cast0[s_mod], 2);
        apply_luma_lpf(buf_yc_cast1[s_mod], 2);

        // INJECT HIGH-FREQUENCY MATHEMATICAL ELEMENTS 
        overlay_gratings(buf_gratings_cast0[s_mod]);
        overlay_gratings(buf_gratings_cast1[s_mod]);

        int bar_start = 357; 
        int bar_w = 96; 
        
        fill_color_bar(buf_bars_cast0[s_mod], bar_start + 0*bar_w, bar_w, 188, -35, 8, s_mod);   
        fill_color_bar(buf_bars_cast0[s_mod], bar_start + 1*bar_w, bar_w, 173, 12, -50, s_mod);  
        fill_color_bar(buf_bars_cast0[s_mod], bar_start + 2*bar_w, bar_w, 163, -23, -42, s_mod); 
        fill_color_bar(buf_bars_cast0[s_mod], bar_start + 3*bar_w, bar_w, 150, 23, 42, s_mod);   
        fill_color_bar(buf_bars_cast0[s_mod], bar_start + 4*bar_w, bar_w, 140, -12, 50, s_mod);  
        fill_color_bar(buf_bars_cast0[s_mod], bar_start + 5*bar_w, bar_w, 125, 35, -8, s_mod);   
        
        fill_color_bar(buf_bars_cast1[s_mod], bar_start + 0*bar_w, bar_w, 188, -35, 8, s_mod);   
        fill_color_bar(buf_bars_cast1[s_mod], bar_start + 1*bar_w, bar_w, 173, 12, -50, s_mod);  
        fill_color_bar(buf_bars_cast1[s_mod], bar_start + 2*bar_w, bar_w, 163, -23, -42, s_mod); 
        fill_color_bar(buf_bars_cast1[s_mod], bar_start + 3*bar_w, bar_w, 150, 23, 42, s_mod);   
        fill_color_bar(buf_bars_cast1[s_mod], bar_start + 4*bar_w, bar_w, 140, -12, 50, s_mod);  
        fill_color_bar(buf_bars_cast1[s_mod], bar_start + 5*bar_w, bar_w, 125, 35, -8, s_mod);   

        int center_red_w = 48; 
        int yellow_w = 264; 
        
        fill_color_bar(buf_yc_cast0[s_mod], bar_start, yellow_w, 188, -35, 8, s_mod);
        fill_color_bar(buf_yc_cast0[s_mod], bar_start + yellow_w, center_red_w, 140, -12, 50, s_mod);
        fill_color_bar(buf_yc_cast0[s_mod], bar_start + yellow_w + center_red_w, yellow_w, 188, -35, 8, s_mod);

        fill_color_bar(buf_yc_cast1[s_mod], bar_start, yellow_w, 188, -35, 8, s_mod);
        fill_color_bar(buf_yc_cast1[s_mod], bar_start + yellow_w, center_red_w, 140, -12, 50, s_mod);
        fill_color_bar(buf_yc_cast1[s_mod], bar_start + yellow_w + center_red_w, yellow_w, 188, -35, 8, s_mod);
        
        // --- CREATE CENTER CROSS VERTICAL EXTENSIONS ---
        // Copy the completed bars/gratings to the new variation buffers
        memcpy(buf_bars_lower_cast0[s_mod], buf_bars_cast0[s_mod], 1136);
        memcpy(buf_bars_lower_cast1[s_mod], buf_bars_cast1[s_mod], 1136);
        memcpy(buf_gratings_upper_cast0[s_mod], buf_gratings_cast0[s_mod], 1136);
        memcpy(buf_gratings_upper_cast1[s_mod], buf_gratings_cast1[s_mod], 1136);
        
        // Overlay the black box and central white line onto the extensions
        overlay_center_ext(buf_bars_lower_cast0[s_mod]);
        overlay_center_ext(buf_bars_lower_cast1[s_mod]);
        overlay_center_ext(buf_gratings_upper_cast0[s_mod]);
        overlay_center_ext(buf_gratings_upper_cast1[s_mod]);
    }

    // MAP BASE AND SPECIAL RENDER TABLES
    for (int i = 0; i < 2500; i++) {
        int line = (i % 625) + 1;
        int s_mod = i % 4; 

        if (line == 624 || line == 625 || line == 4 || line == 5 || line == 311 || line == 312 || line == 316 || line == 317) {
            base_table[i] = buf_eq;
            special_table[i] = buf_eq;
        } else if (line == 1 || line == 2 || line == 314 || line == 315) {
            base_table[i] = buf_vsync;
            special_table[i] = buf_vsync;
        } else if (line == 3) {
            base_table[i] = buf_sync_eq;
            special_table[i] = buf_sync_eq;
        } else if (line == 313) {
            base_table[i] = buf_eq_sync;
            special_table[i] = buf_eq_sync;
        } else if (line == 623) {
            base_table[i] = buf_blank_eq;
            special_table[i] = buf_blank_eq;
        } else if (line == 318) {
            base_table[i] = buf_eq_blank;
            special_table[i] = buf_eq_blank;
        } else if (line < 23 || (line > 310 && line < 336) || line > 623) {
            base_table[i] = buf_blank[s_mod];
            special_table[i] = buf_blank[s_mod];
        } else {
            int frame_y = (line < 313) ? (line - 23) * 2 : (line - 336) * 2 + 1;

            if (frame_y < 3 || frame_y >= 573) {
                base_table[i] = buf_blank[s_mod];
                special_table[i] = buf_blank[s_mod];
                continue;
            }

            int cell_y = (frame_y - 3) / 38;      
            int mod_y = (frame_y - 3) % 38;       
            
            bool is_tb_zone = (cell_y == 0 || cell_y == 14); 
            bool is_h_grid = (mod_y == 0 || mod_y == 37);    
            bool cast_y_even = (cell_y % 2 == 0);

            // Establish the primary background base mapping to the strict vertical zones
            if (is_tb_zone) {
                base_table[i] = buf_top_bot[s_mod];
            } else {
                if (cell_y == 1 || cell_y == 13) {
                    if (is_h_grid) base_table[i] = cast_y_even ? buf_grid_blank_cast0[s_mod] : buf_grid_blank_cast1[s_mod];
                    else base_table[i] = cast_y_even ? buf_bg_blank_cast0[s_mod] : buf_bg_blank_cast1[s_mod];
                } else if (cell_y == 2 || cell_y == 3) {
                    if (is_h_grid) base_table[i] = cast_y_even ? buf_grid_z1_cast0[s_mod] : buf_grid_z1_cast1[s_mod];
                    else base_table[i] = cast_y_even ? buf_bg_z1_cast0[s_mod] : buf_bg_z1_cast1[s_mod];
                } else if (cell_y >= 4 && cell_y <= 7) {
                    // Splitting cell_y 7 exactly in half for 5.5 cells
                    if (cell_y == 7 && mod_y >= 19) {
                        if (is_h_grid) base_table[i] = cast_y_even ? buf_grid_z3_cast0[s_mod] : buf_grid_z3_cast1[s_mod];
                        else base_table[i] = cast_y_even ? buf_bg_z3_cast0[s_mod] : buf_bg_z3_cast1[s_mod];
                    } else {
                        if (is_h_grid) base_table[i] = cast_y_even ? buf_grid_z2_cast0[s_mod] : buf_grid_z2_cast1[s_mod];
                        else base_table[i] = cast_y_even ? buf_bg_z2_cast0[s_mod] : buf_bg_z2_cast1[s_mod];
                    }
                } else if (cell_y >= 8 && cell_y <= 10) {
                    if (is_h_grid) base_table[i] = cast_y_even ? buf_grid_z3_cast0[s_mod] : buf_grid_z3_cast1[s_mod];
                    else base_table[i] = cast_y_even ? buf_bg_z3_cast0[s_mod] : buf_bg_z3_cast1[s_mod];
                } else if (cell_y == 11 || cell_y == 12) {
                    if (is_h_grid) base_table[i] = cast_y_even ? buf_grid_z4_cast0[s_mod] : buf_grid_z4_cast1[s_mod];
                    else base_table[i] = cast_y_even ? buf_bg_z4_cast0[s_mod] : buf_bg_z4_cast1[s_mod];
                }
            }

            bool is_top_lf_white = (cell_y == 1 && mod_y >= 19);
            bool is_top_lf_black = (cell_y == 2);
            bool is_top_ref = (cell_y == 3);
            bool is_sqwave = (cell_y == 4);               
            bool is_color_bars_top = (cell_y == 5);  
            bool is_color_bars_bot = (cell_y == 6);
            bool is_center_cross = (cell_y == 7);
            bool is_gratings_top = (cell_y == 8);    
            bool is_gratings_bot = (cell_y == 9);
            bool is_grayscale = (cell_y == 10);           
            bool is_bot_ref = (cell_y == 11);
            bool is_yc_check = (cell_y == 12) || (cell_y == 13 && mod_y < 19);

            if (is_tb_zone) {
                special_table[i] = base_table[i];
            } else if (is_top_lf_white) {
                special_table[i] = cast_y_even ? buf_top_lf_white_cast0[s_mod] : buf_top_lf_white_cast1[s_mod];
            } else if (is_top_lf_black) {
                special_table[i] = cast_y_even ? buf_top_lf_black_cast0[s_mod] : buf_top_lf_black_cast1[s_mod];
            } else if (is_top_ref) {
                special_table[i] = cast_y_even ? buf_top_ref_cast0[s_mod] : buf_top_ref_cast1[s_mod];
            } else if (is_sqwave) {
                special_table[i] = cast_y_even ? buf_sqwave_cast0[s_mod] : buf_sqwave_cast1[s_mod];
            } else if (is_color_bars_top) { 
                special_table[i] = cast_y_even ? buf_bars_cast0[s_mod] : buf_bars_cast1[s_mod];
            } else if (is_color_bars_bot) {
                special_table[i] = cast_y_even ? buf_bars_lower_cast0[s_mod] : buf_bars_lower_cast1[s_mod];
            } else if (is_center_cross) {
                if (mod_y >= 17 && mod_y <= 20) special_table[i] = buf_cross_mid[s_mod];
                else special_table[i] = buf_cross_top[s_mod];
            } else if (is_gratings_top) {
                special_table[i] = cast_y_even ? buf_gratings_upper_cast0[s_mod] : buf_gratings_upper_cast1[s_mod];
            } else if (is_gratings_bot) {
                special_table[i] = cast_y_even ? buf_gratings_cast0[s_mod] : buf_gratings_cast1[s_mod];
            } else if (is_grayscale) {
                special_table[i] = cast_y_even ? buf_gray_cast0[s_mod] : buf_gray_cast1[s_mod];
            } else if (is_bot_ref) {
                special_table[i] = cast_y_even ? buf_bot_ref_cast0[s_mod] : buf_bot_ref_cast1[s_mod];
            } else if (is_yc_check) {
                special_table[i] = cast_y_even ? buf_yc_cast0[s_mod] : buf_yc_cast1[s_mod];
            } else {
                special_table[i] = base_table[i];
            }
        }
    }
}
