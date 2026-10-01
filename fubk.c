// fubk.c

void draw_fubk_grid(uint8_t *buffer, int s_mod, bool is_h_grid) {
    generate_hblank(buffer, s_mod);
    for (int x = 186; x < 1136; x++) {
        if (x < 189 || x >= 1101) {
            buffer[x] = BLACK_LEVEL;
        } else {
            int shift_x = x - 189 + 24; 
            int mod_x = shift_x % 48;
            if (is_h_grid || mod_x == 0 || mod_x == 47) buffer[x] = WHITE_LEVEL;
            else buffer[x] = DARK_GRAY_LEVEL; 
        }
    }
}

void fill_fubk_grating(uint8_t *buf, int start_x, int width, float mhz) {
    float phase_inc = (mhz / 17.734475f) * 2.0f * 3.14159265f;
    for (int i = 0; i < width; i++) {
        float sine_val = sinf(i * phase_inc); 
        int pixel_val = 170 + (int)(sine_val * 54.0f); 
        if (pixel_val < BLACK_LEVEL) pixel_val = BLACK_LEVEL;
        if (pixel_val > WHITE_LEVEL) pixel_val = WHITE_LEVEL;
        buf[start_x + i] = (uint8_t)pixel_val;
    }
}

void fill_color_gradient(uint8_t *buf, int start, int width, float Y_start, float U_start, float V_start, int s_mod) {
    int phase_off = phase_offset[s_mod];
    bool is_odd = v_inverted[s_mod];
    
    for (int x = start; x < start + width; x++) {
        float progress = (float)(x - start) / (float)(width - 1); 
        
        float Y = Y_start - ((Y_start - (float)BLACK_LEVEL) * progress);
        float U = U_start - (U_start * progress);
        float V = V_start - (V_start * progress);
        
        int phase = (x + phase_off) % 4;
        float chroma = 0;
        
        if (is_odd) {
            if (phase == 0) chroma = U; else if (phase == 1) chroma = -V; 
            else if (phase == 2) chroma = -U; else if (phase == 3) chroma = V;
        } else {
            if (phase == 0) chroma = U; else if (phase == 1) chroma = V;
            else if (phase == 2) chroma = -U; else if (phase == 3) chroma = -V;
        }
        
        int pixel_val = (int)(Y + chroma);
        if (pixel_val < 0) pixel_val = 0;
        if (pixel_val > 255) pixel_val = 255;
        buf[x] = (uint8_t)pixel_val;
    }
}

void fill_pal_test_sector(uint8_t *buf, int start, int width, int Y, int amp, bool is_plus_v, int s_mod) {
    int phase_off = phase_offset[s_mod];
    bool is_odd = v_inverted[s_mod];
    
    for (int x = start; x < start + width; x++) {
        int phase = (x + phase_off) % 4;
        int chroma = 0;
        
        if (is_plus_v) {
            if (phase == 1) chroma = amp;
            else if (phase == 3) chroma = -amp;
        } else {
            if (is_odd) {
                if (phase == 0) chroma = -amp;
                else if (phase == 2) chroma = amp;
            } else {
                if (phase == 0) chroma = amp;
                else if (phase == 2) chroma = -amp;
            }
        }
        
        int pixel_val = Y + chroma;
        if (pixel_val < BLACK_LEVEL) pixel_val = BLACK_LEVEL;
        if (pixel_val > WHITE_LEVEL) pixel_val = WHITE_LEVEL;
        buf[x] = (uint8_t)pixel_val;
    }
}

void build_fubk() {
    enable_center_outline = true;
    
    uint8_t (*tri_bufs[13])[1136] = {
        buf_bg_z4_cast0, buf_bg_z4_cast1, buf_grid_z4_cast0, buf_grid_z4_cast1, 
        buf_bars_cast1, buf_bars_lower_cast0, buf_bars_lower_cast1, 
        buf_gratings_upper_cast0, buf_gratings_upper_cast1, buf_cross_top,
        buf_gratings_cast1, buf_sqwave_cast0, buf_sqwave_cast1
    };
    
    for (int y = 0; y < 576; y++) {
        float rad_x = 320.0f;
        float rad_y = 256.0f;
        float dy = (y - 288) / rad_y;
        if (dy >= -1.0f && dy <= 1.0f) {
            center_mask_dx[y] = (int)(rad_x * sqrtf(1.0f - dy * dy)); 
        } else {
            center_mask_dx[y] = 0;
        }

        if (y >= 98 && y < 478) {
            center_copy_dx[y] = 288; 
        } else {
            center_copy_dx[y] = 0;
        }
    }

    for (int s_mod = 0; s_mod < 4; s_mod++) {
        // --- SECURES THE VBI LINES WITH PURE BLACK ---
        generate_hblank(buf_blank[s_mod], s_mod);
        for(int x = 186; x < 1136; x++) buf_blank[s_mod][x] = BLACK_LEVEL;

        draw_fubk_grid(buf_bg_blank_cast0[s_mod], s_mod, false);
        draw_fubk_grid(buf_grid_blank_cast0[s_mod], s_mod, true);
        apply_luma_lpf(buf_bg_blank_cast0[s_mod], 2);
        apply_luma_lpf(buf_grid_blank_cast0[s_mod], 2);

        int b_start = 357; 

        // ROW 1: Color Bars
        generate_hblank(buf_bars_cast0[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_bars_cast0[s_mod][x] = GRAY_LEVEL;
        int bw = 72;
        fill_color_bar(buf_bars_cast0[s_mod], b_start + 0*bw, bw, 197, 0, 0, s_mod);     
        fill_color_bar(buf_bars_cast0[s_mod], b_start + 1*bw, bw, 188, -35, 8, s_mod); 
        fill_color_bar(buf_bars_cast0[s_mod], b_start + 2*bw, bw, 173, 12, -50, s_mod);   
        fill_color_bar(buf_bars_cast0[s_mod], b_start + 3*bw, bw, 163, -23, -42, s_mod);  
        fill_color_bar(buf_bars_cast0[s_mod], b_start + 4*bw, bw, 150, 23, 42, s_mod);  
        fill_color_bar(buf_bars_cast0[s_mod], b_start + 5*bw, bw, 140, -12, 50, s_mod); 
        fill_color_bar(buf_bars_cast0[s_mod], b_start + 6*bw, bw, 125, 35, -8, s_mod);   
        fill_color_bar(buf_bars_cast0[s_mod], b_start + 7*bw, bw, 116, 0, 0, s_mod);     

        // ROW 2: Grayscale
        generate_hblank(buf_gray_cast0[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_gray_cast0[s_mod][x] = GRAY_LEVEL;
        int gsw = 115;
        int g_levels[5] = {116, 143, 170, 197, 224};
        for (int b = 0; b < 5; b++) {
            for (int x = b_start + b*gsw; x < b_start + (b+1)*gsw; x++) {
                buf_gray_cast0[s_mod][x] = g_levels[b];
            }
        }
        
        for(int x=644; x<=646; x++) buf_gray_cast0[s_mod][x] = WHITE_LEVEL;

        // ROW 3: Top Mid (White BG, Black Box)
        generate_hblank(buf_bg_z1_cast0[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_bg_z1_cast0[s_mod][x] = GRAY_LEVEL;
        for(int x=b_start; x<b_start+576; x++) buf_bg_z1_cast0[s_mod][x] = WHITE_LEVEL;
        for(int x=472; x<818; x++) buf_bg_z1_cast0[s_mod][x] = BLACK_LEVEL; 
        for(int x=644; x<=646; x++) buf_bg_z1_cast0[s_mod][x] = WHITE_LEVEL; 

        memcpy(buf_cross_mid[s_mod], buf_bg_z1_cast0[s_mod], 1136);
        for(int x=b_start; x<b_start+576; x++) buf_cross_mid[s_mod][x] = WHITE_LEVEL; 

        // ROW 4: Bottom Mid (Redesigned Gratings Layout)
        generate_hblank(buf_gratings_cast0[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_gratings_cast0[s_mod][x] = GRAY_LEVEL;
        
        for(int x = b_start; x < b_start + 72; x++) buf_gratings_cast0[s_mod][x] = WHITE_LEVEL;
        fill_fubk_grating(buf_gratings_cast0[s_mod], b_start + 86, 86, 1.0f);
        fill_fubk_grating(buf_gratings_cast0[s_mod], b_start + 184, 86, 2.0f);
        fill_fubk_grating(buf_gratings_cast0[s_mod], b_start + 306, 86, 3.0f);
        fill_color_bar(buf_gratings_cast0[s_mod], b_start + 408, 144, 160, -34, 16, s_mod);
        
        for(int x=644; x<=646; x++) buf_gratings_cast0[s_mod][x] = WHITE_LEVEL; 

        // ROW 4B: The Tapering Triangle
        for (int i = 0; i < 13; i++) {
            generate_hblank(tri_bufs[i][s_mod], s_mod);
            for(int x=186; x<1136; x++) tri_bufs[i][s_mod][x] = GRAY_LEVEL;
            for(int x=b_start; x<b_start+576; x++) tri_bufs[i][s_mod][x] = WHITE_LEVEL;
            
            int w = 12 - i; 
            for(int x = 645 - w; x < 645 + w; x++) {
                tri_bufs[i][s_mod][x] = BLACK_LEVEL;
            }
        }

        // ROW 5 & 6: Bottom Blocks
        generate_hblank(buf_bg_z2_cast0[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_bg_z2_cast0[s_mod][x] = GRAY_LEVEL;
        fill_color_gradient(buf_bg_z2_cast0[s_mod], b_start, 384, 140.0f, -12.0f, 50.0f, s_mod); 
        for(int x = b_start + 384; x < b_start + 576; x++) buf_bg_z2_cast0[s_mod][x] = GRAY_LEVEL;
        fill_pal_test_sector(buf_bg_z2_cast0[s_mod], b_start + 384, 96, GRAY_LEVEL, 44, true, s_mod);  
        fill_pal_test_sector(buf_bg_z2_cast0[s_mod], b_start + 480, 96, GRAY_LEVEL, 44, false, s_mod); 

        generate_hblank(buf_bg_z3_cast0[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_bg_z3_cast0[s_mod][x] = GRAY_LEVEL;
        fill_color_gradient(buf_bg_z3_cast0[s_mod], b_start, 384, 125.0f, 35.0f, -8.0f, s_mod); 
        for(int x = b_start + 384; x < b_start + 576; x++) buf_bg_z3_cast0[s_mod][x] = GRAY_LEVEL;
        fill_pal_test_sector(buf_bg_z3_cast0[s_mod], b_start + 384, 96, GRAY_LEVEL, 44, true, s_mod);  
        fill_pal_test_sector(buf_bg_z3_cast0[s_mod], b_start + 480, 96, GRAY_LEVEL, 44, false, s_mod); 
        
        apply_luma_lpf(buf_gray_cast0[s_mod], 2);
        apply_luma_lpf(buf_bg_z1_cast0[s_mod], 2);
        apply_luma_lpf(buf_cross_mid[s_mod], 2);
        for (int i = 0; i < 13; i++) apply_luma_lpf(tri_bufs[i][s_mod], 2);
    }

    // Mapping
    for (int i = 0; i < 2500; i++) {
        int line = (i % 625) + 1;
        int s_mod = i % 4; 

        if (line == 624 || line == 625 || line == 4 || line == 5 || line == 311 || line == 312 || line == 316 || line == 317) {
            base_table[i] = buf_eq; special_table[i] = buf_eq;
        } else if (line == 1 || line == 2 || line == 314 || line == 315) {
            base_table[i] = buf_vsync; special_table[i] = buf_vsync;
        } else if (line == 3) {
            base_table[i] = buf_sync_eq; special_table[i] = buf_sync_eq;
        } else if (line == 313) {
            base_table[i] = buf_eq_sync; special_table[i] = buf_eq_sync;
        } else if (line == 623) {
            base_table[i] = buf_blank_eq; special_table[i] = buf_blank_eq;
        } else if (line == 318) {
            base_table[i] = buf_eq_blank; special_table[i] = buf_eq_blank;
        } else if (line < 23 || (line > 310 && line < 336) || line > 623) {
            // --- EXPLICITLY BINDS VBI REGIONS TO PURE BLACK ---
            base_table[i] = buf_blank[s_mod]; 
            special_table[i] = buf_blank[s_mod];
        } else {
            int frame_y = (line < 313) ? (line - 23) * 2 : (line - 336) * 2 + 1;
            if (frame_y < 3 || frame_y >= 573) {
                base_table[i] = buf_bg_blank_cast0[s_mod]; special_table[i] = buf_bg_blank_cast0[s_mod];
                continue;
            }

            bool is_h_grid = ((frame_y - 22) % 38 == 0 || (frame_y - 22) % 38 == 37);    

            if (is_h_grid) base_table[i] = buf_grid_blank_cast0[s_mod];
            else base_table[i] = buf_bg_blank_cast0[s_mod];

            if (frame_y >= 98 && frame_y < 212) special_table[i] = buf_bars_cast0[s_mod];
            else if (frame_y >= 212 && frame_y < 287) special_table[i] = buf_gray_cast0[s_mod];
            else if (frame_y == 287 || frame_y == 289) special_table[i] = buf_cross_mid[s_mod];
            else if (frame_y >= 289 && frame_y < 326) special_table[i] = buf_bg_z1_cast0[s_mod];
            else if (frame_y >= 326 && frame_y < 364) special_table[i] = buf_gratings_cast0[s_mod];
            else if (frame_y >= 364 && frame_y < 402) {
                int step = (frame_y - 364) / 3;
                if (step > 12) step = 12;
                special_table[i] = tri_bufs[step][s_mod];
            }
            else if (frame_y >= 402 && frame_y < 440) special_table[i] = buf_bg_z2_cast0[s_mod];
            else if (frame_y >= 440 && frame_y < 478) special_table[i] = buf_bg_z3_cast0[s_mod];
            else special_table[i] = base_table[i]; 
        }
    }
}
