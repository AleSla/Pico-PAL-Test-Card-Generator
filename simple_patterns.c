// simple_patterns.c

void fill_sine_burst(uint8_t *buf, int start_x, int width, float mhz) {
    float phase_inc = (mhz / 17.734475f) * 2.0f * 3.14159265f;
    for (int i = 0; i < width; i++) {
        float sine_val = sinf(i * phase_inc); 
        int pixel_val = 170 + (int)(sine_val * 54.0f); 
        if (pixel_val < BLACK_LEVEL) pixel_val = BLACK_LEVEL;
        if (pixel_val > WHITE_LEVEL) pixel_val = WHITE_LEVEL;
        buf[start_x + i] = (uint8_t)pixel_val;
    }
}

void build_ebu_bars() {
    enable_center_outline = false;
    for (int y = 0; y < 576; y++) { center_copy_dx[y] = 0; center_mask_dx[y] = 0; }

    for (int s_mod = 0; s_mod < 4; s_mod++) {
        generate_hblank(buf_bars_cast0[s_mod], s_mod);
        for(int x = 186; x < 1136; x++) buf_bars_cast0[s_mod][x] = BLACK_LEVEL;

        int bw = 118; 
        int start = 190;
        fill_color_bar(buf_bars_cast0[s_mod], start + 0*bw, bw, 224, 0, 0, s_mod);     
        fill_color_bar(buf_bars_cast0[s_mod], start + 1*bw, bw, 188, -35, 8, s_mod);  
        fill_color_bar(buf_bars_cast0[s_mod], start + 2*bw, bw, 173, 12, -50, s_mod);  
        fill_color_bar(buf_bars_cast0[s_mod], start + 3*bw, bw, 161, -23, -42, s_mod); 
        fill_color_bar(buf_bars_cast0[s_mod], start + 4*bw, bw, 149, 23, 42, s_mod);   
        fill_color_bar(buf_bars_cast0[s_mod], start + 5*bw, bw, 137, -12, 50, s_mod);  
        fill_color_bar(buf_bars_cast0[s_mod], start + 6*bw, bw, 125, 35, -8, s_mod);  
        fill_color_bar(buf_bars_cast0[s_mod], start + 7*bw, bw+2, 116, 0, 0, s_mod);   
    }

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
            base_table[i] = buf_blank[s_mod]; special_table[i] = buf_blank[s_mod]; 
        } else {
            base_table[i] = buf_bars_cast0[s_mod]; 
            special_table[i] = buf_bars_cast0[s_mod];
        }
    }
}

void build_crosshatch() {
    enable_center_outline = false;
    for (int y = 0; y < 576; y++) { center_copy_dx[y] = 0; center_mask_dx[y] = 0; }

    for (int s_mod = 0; s_mod < 4; s_mod++) {
        generate_hblank(buf_grid_z1_cast0[s_mod], s_mod); 
        generate_hblank(buf_grid_z1_cast1[s_mod], s_mod); 
        
        for (int x = 186; x < 1136; x++) {
            buf_grid_z1_cast0[s_mod][x] = WHITE_LEVEL; 
            
            if (x < 190 || x >= 1130) {
                buf_grid_z1_cast1[s_mod][x] = BLACK_LEVEL;
            } else {
                int mod_x = (x - 190) % 47; 
                buf_grid_z1_cast1[s_mod][x] = (mod_x < 2) ? WHITE_LEVEL : DARK_GRAY_LEVEL; 
            }
        }
    }
    
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
            base_table[i] = buf_blank[s_mod]; special_table[i] = buf_blank[s_mod]; 
        } else {
            int frame_y = (line < 313) ? (line - 23) * 2 : (line - 336) * 2 + 1;
            
            if (frame_y % 36 < 2) { 
                base_table[i] = buf_grid_z1_cast0[s_mod]; 
            } else {
                base_table[i] = buf_grid_z1_cast1[s_mod];
            }
            special_table[i] = base_table[i];
        }
    }
}

void build_checkerboard() {
    enable_center_outline = false;
    for (int y = 0; y < 576; y++) { center_copy_dx[y] = 0; center_mask_dx[y] = 0; }

    for (int s_mod = 0; s_mod < 4; s_mod++) {
        generate_hblank(buf_sqwave_cast0[s_mod], s_mod); 
        generate_hblank(buf_sqwave_cast1[s_mod], s_mod); 
        
        for (int x = 186; x < 1136; x++) {
            if (x < 190 || x >= 1130) {
                buf_sqwave_cast0[s_mod][x] = BLACK_LEVEL;
                buf_sqwave_cast1[s_mod][x] = BLACK_LEVEL;
                continue;
            }
            int cell_x = (x - 190) / 47;
            buf_sqwave_cast0[s_mod][x] = (cell_x % 2 == 0) ? WHITE_LEVEL : BLACK_LEVEL;
            buf_sqwave_cast1[s_mod][x] = (cell_x % 2 == 0) ? BLACK_LEVEL : WHITE_LEVEL;
        }
    }

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
            base_table[i] = buf_blank[s_mod]; special_table[i] = buf_blank[s_mod]; 
        } else {
            int frame_y = (line < 313) ? (line - 23) * 2 : (line - 336) * 2 + 1;
            int cell_y = frame_y / 36; 
            
            if (cell_y % 2 == 0) {
                base_table[i] = buf_sqwave_cast0[s_mod]; 
            } else {
                base_table[i] = buf_sqwave_cast1[s_mod];
            }
            special_table[i] = base_table[i];
        }
    }
}

void build_multiburst() {
    enable_center_outline = false;
    for (int y = 0; y < 576; y++) { center_copy_dx[y] = 0; center_mask_dx[y] = 0; }

    for (int s_mod = 0; s_mod < 4; s_mod++) {
        generate_hblank(buf_gratings_cast0[s_mod], s_mod);
        
        for(int x = 186; x < 1136; x++) {
            if (x < 190 || x >= 1126) buf_gratings_cast0[s_mod][x] = BLACK_LEVEL;
            else buf_gratings_cast0[s_mod][x] = GRAY_LEVEL; 
        }

        int start = 190;
        int bw = 156; 
        
        fill_sine_burst(buf_gratings_cast0[s_mod], start + 0*bw, bw, 1.0f);
        fill_sine_burst(buf_gratings_cast0[s_mod], start + 1*bw, bw, 2.0f);
        fill_sine_burst(buf_gratings_cast0[s_mod], start + 2*bw, bw, 3.0f);
        fill_sine_burst(buf_gratings_cast0[s_mod], start + 3*bw, bw, 4.0f);
        fill_sine_burst(buf_gratings_cast0[s_mod], start + 4*bw, bw, 4.43f);
        fill_sine_burst(buf_gratings_cast0[s_mod], start + 5*bw, bw, 5.0f);
    }

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
            base_table[i] = buf_blank[s_mod]; special_table[i] = buf_blank[s_mod]; 
        } else {
            base_table[i] = buf_gratings_cast0[s_mod]; 
            special_table[i] = buf_gratings_cast0[s_mod];
        }
    }
}