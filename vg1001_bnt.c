// vg1001_bnt.c

#define buf_grid_cast_top buf_top_bot
#define buf_grid_cast_bot buf_bars_lower_cast0
#define buf_grid_inner_even buf_grid_blank_cast0
#define buf_grid_inner_even_h buf_bg_blank_cast0
#define buf_grid_inner_odd buf_grid_z1_cast0
#define buf_grid_inner_odd_h buf_bg_z1_cast0
#define buf_grid_y2 buf_gratings_upper_cast0
#define buf_grid_y2_h buf_gratings_upper_cast1
#define buf_box_y4 buf_sqwave_cast0
#define buf_box_cbars buf_bars_cast0
#define buf_box_gratings buf_gratings_cast0
#define buf_box_gray buf_gray_cast0
#define buf_box_center buf_cross_top
#define buf_box_center_mid buf_cross_mid
#define buf_box_y2 buf_bg_z2_cast0

#define VG_DARK_GRAY 116

void draw_vg1001_bg(uint8_t *buf, int s_mod, int cell_y, bool is_h_grid) {
    generate_hblank(buf, s_mod);
    for (int x = 186; x < 1136; x++) {
        if (x >= 1106) { 
            buf[x] = BLACK_LEVEL;
            continue;
        }
        
        int cell_x = (x - 186) / 46;
        int mod_x = (x - 186) % 46;
        
        bool is_black_cast = ((cell_x + cell_y) % 2 == 0); 
        
        if (cell_y == 0 || cell_y == 14) {
            buf[x] = is_black_cast ? WHITE_LEVEL : BLACK_LEVEL;
        } else if (cell_x == 0 || cell_x == 19) {
            buf[x] = is_black_cast ? WHITE_LEVEL : BLACK_LEVEL;
        } else {
            if (is_h_grid || mod_x <= 1) buf[x] = WHITE_LEVEL;
            else buf[x] = VG_DARK_GRAY;
        }
    }
}

void fill_vg1001_grating(uint8_t *buf, int start_x, int width, float mhz) {
    float phase_inc = (mhz / 17.734475f) * 2.0f * 3.14159265f;
    for (int i = 0; i < width; i++) {
        float sine_val = sinf(i * phase_inc); 
        int pixel_val = 170 + (int)(sine_val * 54.0f); 
        if (pixel_val < BLACK_LEVEL) pixel_val = BLACK_LEVEL;
        if (pixel_val > WHITE_LEVEL) pixel_val = WHITE_LEVEL;
        buf[start_x + i] = (uint8_t)pixel_val;
    }
}

void build_vg1001_bnt() {
    enable_center_outline = true;
    
    for (int y = 0; y < 575; y++) {
        float rad_y = 230.0f;
        float rad_x = 276.0f; 
        float dy = (y - 288) / rad_y;
        if (dy >= -1.0f && dy <= 1.0f) {
            center_mask_dx[y] = (int)(rad_x * sqrtf(1.0f - dy * dy)); 
        } else {
            center_mask_dx[y] = 0;
        }

        // Center Box spans from Y=3 to Y=10 (lines 117 to 459)
        if (y >= 118 && y < 459) {
            center_copy_dx[y] = 274; 
        } else {
            center_copy_dx[y] = 0;
        }
    }

    for (int s_mod = 0; s_mod < 4; s_mod++) {
        generate_hblank(buf_blank[s_mod], s_mod);
        for(int x = 186; x < 1136; x++) buf_blank[s_mod][x] = BLACK_LEVEL;

        draw_vg1001_bg(buf_grid_cast_top[s_mod], s_mod, 0, false);
        draw_vg1001_bg(buf_grid_cast_bot[s_mod], s_mod, 14, false);
        
        draw_vg1001_bg(buf_grid_inner_even[s_mod], s_mod, 2, false);
        draw_vg1001_bg(buf_grid_inner_even_h[s_mod], s_mod, 2, true);
        
        draw_vg1001_bg(buf_grid_inner_odd[s_mod], s_mod, 1, false);
        draw_vg1001_bg(buf_grid_inner_odd_h[s_mod], s_mod, 1, true);
        
        memcpy(buf_grid_y2[s_mod], buf_grid_inner_even[s_mod], 1136);
        fill_vg1001_grating(buf_grid_y2[s_mod], 278, 46, 3.0f); 
        fill_vg1001_grating(buf_grid_y2[s_mod], 968, 46, 3.0f); 
        
        memcpy(buf_grid_y2_h[s_mod], buf_grid_inner_even_h[s_mod], 1136);
        fill_vg1001_grating(buf_grid_y2_h[s_mod], 278, 46, 3.0f);
        fill_vg1001_grating(buf_grid_y2_h[s_mod], 968, 46, 3.0f);
        
        apply_luma_lpf(buf_grid_inner_even[s_mod], 2);
        apply_luma_lpf(buf_grid_inner_even_h[s_mod], 2);
        apply_luma_lpf(buf_grid_inner_odd[s_mod], 2);
        apply_luma_lpf(buf_grid_inner_odd_h[s_mod], 2);

        int b_start = 372; 
        
        // Y=4: Black / White Split
        generate_hblank(buf_box_y4[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_box_y4[s_mod][x] = VG_DARK_GRAY;
        apply_luma_lpf(buf_box_y4[s_mod], 2);
        for(int x=b_start; x<b_start+276; x++) buf_box_y4[s_mod][x] = BLACK_LEVEL;
        for(int x=b_start+276; x<b_start+552; x++) buf_box_y4[s_mod][x] = WHITE_LEVEL;

        // Y=5, 6: 100% Color Bars
        generate_hblank(buf_box_cbars[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_box_cbars[s_mod][x] = VG_DARK_GRAY;
        int bw = 69;
        fill_color_bar(buf_box_cbars[s_mod], b_start + 0*bw, bw, 224, 0, 0, s_mod);     
        fill_color_bar(buf_box_cbars[s_mod], b_start + 1*bw, bw, 188, -35, 8, s_mod); 
        fill_color_bar(buf_box_cbars[s_mod], b_start + 2*bw, bw, 173, 12, -50, s_mod);   
        fill_color_bar(buf_box_cbars[s_mod], b_start + 3*bw, bw, 163, -23, -42, s_mod);  
        fill_color_bar(buf_box_cbars[s_mod], b_start + 4*bw, bw, 150, 23, 42, s_mod);  
        fill_color_bar(buf_box_cbars[s_mod], b_start + 5*bw, bw, 140, -12, 50, s_mod); 
        fill_color_bar(buf_box_cbars[s_mod], b_start + 6*bw, bw, 125, 35, -8, s_mod);   
        fill_color_bar(buf_box_cbars[s_mod], b_start + 7*bw, bw, 116, 0, 0, s_mod);

        // Y=7: Center Notch & Line
        generate_hblank(buf_box_center[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_box_center[s_mod][x] = VG_DARK_GRAY;
        apply_luma_lpf(buf_box_center[s_mod], 2);
        apply_luma_lpf(buf_box_center_mid[s_mod], 2);
        for(int x=576; x<=714; x++) buf_box_center[s_mod][x] = BLACK_LEVEL;
        for(int x=643; x<=647; x++) buf_box_center[s_mod][x] = WHITE_LEVEL;
        for(int x=b_start; x<b_start+207; x++) buf_box_center[s_mod][x] = WHITE_LEVEL;
        for(int x=b_start+345; x<b_start+552; x++) buf_box_center[s_mod][x] = WHITE_LEVEL;// Vertical Notch

        memcpy(buf_box_center_mid[s_mod], buf_box_center[s_mod], 1136); 
        for(int x=b_start; x<b_start+552; x++) buf_box_center_mid[s_mod][x] = WHITE_LEVEL; // Horizontal line

        // Y=8, 9: Gratings
        generate_hblank(buf_box_gratings[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_box_gratings[s_mod][x] = VG_DARK_GRAY;
        apply_luma_lpf(buf_box_gratings[s_mod], 2);
        int gw = 138;
        fill_vg1001_grating(buf_box_gratings[s_mod], b_start + 0*gw, gw, 1.0f);
        fill_vg1001_grating(buf_box_gratings[s_mod], b_start + 1*gw, gw, 2.0f);
        fill_vg1001_grating(buf_box_gratings[s_mod], b_start + 2*gw, gw, 3.0f);
        fill_vg1001_grating(buf_box_gratings[s_mod], b_start + 3*gw, gw, 4.0f);

        // Y=10: Grayscale
        generate_hblank(buf_box_gray[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_box_gray[s_mod][x] = VG_DARK_GRAY;
        apply_luma_lpf(buf_box_gray[s_mod], 2);
        int gsw = 69;
        int g_levels[8] = {224, 208, 193, 178, 162, 147, 131, 116};
        for (int b = 0; b < 8; b++) {
            for (int x = b_start + b*gsw; x < b_start + (b+1)*gsw; x++) {
                buf_box_gray[s_mod][x] = g_levels[b];
            }
        }
        
        generate_hblank(buf_box_y2[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_box_y2[s_mod][x] = VG_DARK_GRAY;
        for(int x=b_start; x<b_start+552; x++) buf_box_y2[s_mod][x] = BLACK_LEVEL;
        apply_luma_lpf(buf_grid_y2[s_mod], 2);
        apply_luma_lpf(buf_grid_y2_h[s_mod], 2);
        
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
            if (frame_y < 3 || frame_y >= 573) {
                base_table[i] = buf_blank[s_mod]; special_table[i] = buf_blank[s_mod];
                continue;
            }

            int cell_y = (frame_y - 3) / 38;
            int mod_y = (frame_y - 3) % 38;
            bool is_h_grid = (mod_y <= 1);    
            bool is_even = (cell_y % 2 == 0);
            
            uint8_t *base_buf;
            if (cell_y == 0) base_buf = buf_grid_cast_top[s_mod];
            else if (cell_y == 14) base_buf = buf_grid_cast_bot[s_mod];
            else if (cell_y == 2 || cell_y == 12) {
                base_buf = is_h_grid ? buf_grid_y2_h[s_mod] : buf_grid_y2[s_mod];
            } else {
                if (is_even) base_buf = is_h_grid ? buf_grid_inner_even_h[s_mod] : buf_grid_inner_even[s_mod];
                else base_buf = is_h_grid ? buf_grid_inner_odd_h[s_mod] : buf_grid_inner_odd[s_mod];
            }
            
            base_table[i] = base_buf;

            if (cell_y >= 3 && cell_y <= 11) {
                if (cell_y == 3) special_table[i] = buf_box_y4[s_mod];
                else if (cell_y == 4) special_table[i] = buf_box_cbars[s_mod];
                else if (cell_y == 5 || cell_y == 6) special_table[i] = buf_box_cbars[s_mod];
                else if (cell_y == 7) {
                    if (mod_y == 18 || mod_y == 19) special_table[i] = buf_box_center_mid[s_mod];
                    else special_table[i] = buf_box_center[s_mod];
                }
                else if (cell_y == 8 || cell_y == 9) special_table[i] = buf_box_gratings[s_mod];
                else if (cell_y == 10) special_table[i] = buf_box_gray[s_mod];
                else if (cell_y == 11) special_table[i] = buf_box_y2[s_mod];
                else special_table[i] = base_buf;
            } else {
                special_table[i] = base_buf;
            }
        }
    }
}
