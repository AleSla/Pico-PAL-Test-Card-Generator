// ueit.c

#define buf_ueit_top buf_top_bot
#define buf_ueit_bot buf_bars_lower_cast0
#define buf_ueit_even buf_grid_blank_cast0
#define buf_ueit_even_h buf_bg_blank_cast0
#define buf_ueit_odd buf_grid_z1_cast0
#define buf_ueit_odd_h buf_bg_z1_cast0

// Hijacked buffers for the top Low-Frequency check area
#define buf_ueit_r2_top buf_grid_z2_cast0
#define buf_ueit_r2_top_h buf_grid_z2_cast1
#define buf_ueit_r2_bot buf_bg_z2_cast0
#define buf_ueit_r3_top buf_grid_z3_cast0
#define buf_ueit_r3_top_h buf_grid_z3_cast1
#define buf_ueit_r3_bot buf_bg_z3_cast0
#define buf_ueit_r4 buf_grid_z4_cast0
#define buf_ueit_r4_h buf_grid_z4_cast1

#define buf_ueit_cbars_top buf_bars_cast0
#define buf_ueit_gray buf_gray_cast0
#define buf_ueit_stripes buf_sqwave_cast0
#define buf_ueit_stripes_h buf_sqwave_cast1
#define buf_ueit_slanted buf_cross_top
#define buf_ueit_slanted_h buf_top_ref_cast0
#define buf_ueit_slanted_2 buf_cross_mid
#define buf_ueit_slanted_2_h buf_top_ref_cast1
#define buf_ueit_gradient buf_bot_ref_cast0
#define buf_ueit_gradient_h buf_bot_ref_cast1
#define buf_ueit_gratings buf_gratings_cast0
#define buf_ueit_cbars_bot buf_bars_lower_cast1

// Newly hijacked buffers for the bottom Low-Frequency check area
#define buf_ueit_r15 buf_bg_z4_cast0
#define buf_ueit_r15_h buf_bg_z4_cast1
#define buf_ueit_r16_top buf_gratings_upper_cast0
#define buf_ueit_r16_top_h buf_gratings_upper_cast1
#define buf_ueit_r16_bot buf_top_lf_white_cast0
#define buf_ueit_r17_top buf_top_lf_black_cast0
#define buf_ueit_r17_top_h buf_top_lf_black_cast1
#define buf_ueit_r17_bot buf_yc_cast0

#define UEIT_GRAY 164
#define UEIT_LIGHT_GRAY 200

void draw_ueit_bg(uint8_t *buf, int s_mod, int cell_y, bool is_h_grid) {
    generate_hblank(buf, s_mod);
    for (int x = 186; x < 1136; x++) {
        if (x < 190 || x >= 1100) { 
            buf[x] = BLACK_LEVEL;
            continue;
        }
        
        int cell_x = (x - 190) / 35;
        int mod_x = (x - 190) % 35;
        bool is_black_cast = ((cell_x + cell_y) % 2 == 0); 
        
        if (cell_y == 0 || cell_y == 19 || cell_x == 0 || cell_x == 25) {
            buf[x] = is_black_cast ? BLACK_LEVEL : WHITE_LEVEL;
        } else {
            if (is_h_grid || mod_x <= 1) buf[x] = WHITE_LEVEL; 
            else buf[x] = UEIT_GRAY;
        }
    }
}

// Erases internal white lines by converting them to solid light gray,
// but skips the first 2 pixels to preserve the left vertical boundary box!
void fill_solid_light_gray(uint8_t *buf, int start_x, int end_x) {
    for (int x = start_x + 2; x < end_x; x++) {
        buf[x] = UEIT_LIGHT_GRAY;
    }
}

void fill_ueit_grating(uint8_t *buf, int start_x, int width, float mhz) {
    float phase_inc = (mhz / 17.734475f) * 2.0f * 3.14159265f;
    for (int i = 0; i < width; i++) {
        float sine_val = sinf(i * phase_inc); 
        int pixel_val = 170 + (int)(sine_val * 54.0f); 
        if (pixel_val < BLACK_LEVEL) pixel_val = BLACK_LEVEL;
        if (pixel_val > WHITE_LEVEL) pixel_val = WHITE_LEVEL;
        buf[start_x + i] = (uint8_t)pixel_val;
    }
}

void fill_color_transition(uint8_t *buf, int start, int width, float Y1, float U1, float V1, float Y2, float U2, float V2, int s_mod) {
    int phase_off = phase_offset[s_mod];
    bool is_odd = v_inverted[s_mod];
    
    for (int x = start; x < start + width; x++) {
        float progress = (float)(x - start) / (float)(width - 1); 
        
        float Y = Y1 + (Y2 - Y1) * progress;
        float U = U1 + (U2 - U1) * progress;
        float V = V1 + (V2 - V1) * progress;
        
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
        if (pixel_val < BLACK_LEVEL) pixel_val = BLACK_LEVEL;
        if (pixel_val > WHITE_LEVEL) pixel_val = WHITE_LEVEL;
        buf[x] = (uint8_t)pixel_val;
    }
}

void build_ueit() {
    enable_center_outline = false;
    
    for (int y = 0; y < 576; y++) {
        float rad_y = 224.0f;
        float rad_x = 280.0f; 
        float dy = (y - 288) / rad_y;
        
        if (dy >= -1.0f && dy <= 1.0f) {
            center_mask_dx[y] = (int)(rad_x * sqrtf(1.0f - dy * dy)); 
        } else {
            center_mask_dx[y] = 0;
        }

        // Expanded mask: Now spans from cell_y 2 to 17 (Lines 64 to 512)
        if (y >= 64 && y < 512) {
            int cell_y = (y - 8) / 28;
            
            if (cell_y == 5 || cell_y == 6 || cell_y == 7 || cell_y == 12 || cell_y == 13 || cell_y == 14) {
                center_copy_dx[y] = 420; // Full width (840px)
            } else {
                center_copy_dx[y] = center_mask_dx[y]; // Clamped dynamically to the circle
            }
        } else {
            center_copy_dx[y] = 0;
        }
    }

    for (int s_mod = 0; s_mod < 4; s_mod++) {
        generate_hblank(buf_blank[s_mod], s_mod);
        for(int x = 186; x < 1136; x++) buf_blank[s_mod][x] = BLACK_LEVEL;

        draw_ueit_bg(buf_ueit_top[s_mod], s_mod, 0, false);
        draw_ueit_bg(buf_ueit_bot[s_mod], s_mod, 19, false);
        
        draw_ueit_bg(buf_ueit_even[s_mod], s_mod, 2, false);
        draw_ueit_bg(buf_ueit_even_h[s_mod], s_mod, 2, true);
        
        draw_ueit_bg(buf_ueit_odd[s_mod], s_mod, 1, false);
        draw_ueit_bg(buf_ueit_odd_h[s_mod], s_mod, 1, true);

        int b_start = 225; 
        
        // --- 1. SETUP STATIC BACKGROUNDS & GRID OVERLAYS ---
        // Y=2 & 3: Top Low-Frequency Check 
        memcpy(buf_ueit_r2_top[s_mod], buf_ueit_even[s_mod], 1136);
        memcpy(buf_ueit_r2_top_h[s_mod], buf_ueit_even_h[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r2_top[s_mod], b_start+140, b_start+700);

        memcpy(buf_ueit_r2_bot[s_mod], buf_ueit_even[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r2_bot[s_mod], b_start+140, b_start+350);
        fill_solid_light_gray(buf_ueit_r2_bot[s_mod], b_start+490, b_start+700);

        memcpy(buf_ueit_r3_top[s_mod], buf_ueit_odd[s_mod], 1136);
        memcpy(buf_ueit_r3_top_h[s_mod], buf_ueit_odd_h[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r3_top[s_mod], b_start+140, b_start+350);
        fill_solid_light_gray(buf_ueit_r3_top_h[s_mod], b_start+140, b_start+350); 
        fill_solid_light_gray(buf_ueit_r3_top[s_mod], b_start+490, b_start+700);
        fill_solid_light_gray(buf_ueit_r3_top_h[s_mod], b_start+490, b_start+700); 

        memcpy(buf_ueit_r3_bot[s_mod], buf_ueit_odd[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r3_bot[s_mod], b_start+140, b_start+700);

        // Y=4: Pre-Color Bar block 
        memcpy(buf_ueit_r4[s_mod], buf_ueit_even[s_mod], 1136);
        memcpy(buf_ueit_r4_h[s_mod], buf_ueit_even_h[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r4[s_mod], b_start+140, b_start+245);
        fill_solid_light_gray(buf_ueit_r4_h[s_mod], b_start+140, b_start+245); 
        fill_solid_light_gray(buf_ueit_r4[s_mod], b_start+595, b_start+700);
        fill_solid_light_gray(buf_ueit_r4_h[s_mod], b_start+595, b_start+700); 

        generate_hblank(buf_ueit_cbars_top[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_ueit_cbars_top[s_mod][x] = UEIT_GRAY;
        
        generate_hblank(buf_ueit_gray[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_ueit_gray[s_mod][x] = UEIT_GRAY;
        
        memcpy(buf_ueit_stripes[s_mod], buf_ueit_even[s_mod], 1136);
        memcpy(buf_ueit_stripes_h[s_mod], buf_ueit_even_h[s_mod], 1136);

        // Y=9: Base Grid embedded into Slanted 1
        memcpy(buf_ueit_slanted[s_mod], buf_ueit_odd[s_mod], 1136);
        memcpy(buf_ueit_slanted_h[s_mod], buf_ueit_odd_h[s_mod], 1136);
        for(int x=b_start+140; x<b_start+315; x++) {
            buf_ueit_slanted[s_mod][x] = WHITE_LEVEL;
            buf_ueit_slanted_h[s_mod][x] = WHITE_LEVEL;
        }
        for(int x=b_start+315; x<b_start+385; x++) {
            buf_ueit_slanted[s_mod][x] = UEIT_GRAY;
            buf_ueit_slanted_h[s_mod][x] = UEIT_GRAY;
        }
        for(int x=b_start+457; x<b_start+525; x++) {
            buf_ueit_slanted[s_mod][x] = UEIT_GRAY;
            buf_ueit_slanted_h[s_mod][x] = UEIT_GRAY;
        }
        for(int x=b_start+525; x<b_start+700; x++) {
            buf_ueit_slanted[s_mod][x] = BLACK_LEVEL;
            buf_ueit_slanted_h[s_mod][x] = BLACK_LEVEL;
        }

        // Y=10: Base Grid embedded into Slanted 2
        memcpy(buf_ueit_slanted_2[s_mod], buf_ueit_even[s_mod], 1136);
        memcpy(buf_ueit_slanted_2_h[s_mod], buf_ueit_even_h[s_mod], 1136);
        for(int x=b_start+140; x<b_start+315; x++) {
            buf_ueit_slanted_2[s_mod][x] = BLACK_LEVEL;
            buf_ueit_slanted_2_h[s_mod][x] = BLACK_LEVEL;
        }
        for(int x=b_start+315; x<b_start+385; x++) {
            buf_ueit_slanted_2[s_mod][x] = UEIT_GRAY;
            buf_ueit_slanted_2_h[s_mod][x] = UEIT_GRAY;
        }
        for(int x=b_start+457; x<b_start+525; x++) {
            buf_ueit_slanted_2[s_mod][x] = UEIT_GRAY;
            buf_ueit_slanted_2_h[s_mod][x] = UEIT_GRAY;
        }
        for(int x=b_start+525; x<b_start+700; x++) {
            buf_ueit_slanted_2[s_mod][x] = WHITE_LEVEL;
            buf_ueit_slanted_2_h[s_mod][x] = WHITE_LEVEL;
        }

        memcpy(buf_ueit_gradient[s_mod], buf_ueit_odd[s_mod], 1136);
        memcpy(buf_ueit_gradient_h[s_mod], buf_ueit_odd_h[s_mod], 1136);

        generate_hblank(buf_ueit_gratings[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_ueit_gratings[s_mod][x] = UEIT_GRAY;

        generate_hblank(buf_ueit_cbars_bot[s_mod], s_mod);
        for(int x=186; x<1136; x++) buf_ueit_cbars_bot[s_mod][x] = UEIT_GRAY;

        // Y=15: Alternating Black & Light Gray Cells (Grid Preserved)
        memcpy(buf_ueit_r15[s_mod], buf_ueit_odd[s_mod], 1136);
        memcpy(buf_ueit_r15_h[s_mod], buf_ueit_odd_h[s_mod], 1136);
        for(int cx = 0; cx < 24; cx++) {
            int start_x = b_start + cx * 35;
            int end_x = start_x + 35;
            int color = (cx % 2 == 0) ? UEIT_LIGHT_GRAY : BLACK_LEVEL;
            for (int x = start_x + 2; x < end_x; x++) {
                // Safely replaces only the base gray to protect horizontal white grid lines
                if (buf_ueit_r15[s_mod][x] == UEIT_GRAY) buf_ueit_r15[s_mod][x] = color;
                if (buf_ueit_r15_h[s_mod][x] == UEIT_GRAY) buf_ueit_r15_h[s_mod][x] = color;
            }
        }

        // Y=16 & 17: Bottom Low-Frequency Check (Mirrored Layout)
        memcpy(buf_ueit_r16_top[s_mod], buf_ueit_even[s_mod], 1136);
        memcpy(buf_ueit_r16_top_h[s_mod], buf_ueit_even_h[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r16_top[s_mod], b_start+140, b_start+700);
        
        memcpy(buf_ueit_r16_bot[s_mod], buf_ueit_even[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r16_bot[s_mod], b_start+140, b_start+350);
        fill_solid_light_gray(buf_ueit_r16_bot[s_mod], b_start+490, b_start+700);

        memcpy(buf_ueit_r17_top[s_mod], buf_ueit_odd[s_mod], 1136);
        memcpy(buf_ueit_r17_top_h[s_mod], buf_ueit_odd_h[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r17_top[s_mod], b_start+140, b_start+350);
        fill_solid_light_gray(buf_ueit_r17_top_h[s_mod], b_start+140, b_start+350); 
        fill_solid_light_gray(buf_ueit_r17_top[s_mod], b_start+490, b_start+700);
        fill_solid_light_gray(buf_ueit_r17_top_h[s_mod], b_start+490, b_start+700); 

        memcpy(buf_ueit_r17_bot[s_mod], buf_ueit_odd[s_mod], 1136);
        fill_solid_light_gray(buf_ueit_r17_bot[s_mod], b_start+140, b_start+700);

        // --- 2. APPLY LOW-PASS FILTER ---
        apply_luma_lpf(buf_ueit_even[s_mod], 2);
        apply_luma_lpf(buf_ueit_even_h[s_mod], 2);
        apply_luma_lpf(buf_ueit_odd[s_mod], 2);
        apply_luma_lpf(buf_ueit_odd_h[s_mod], 2);
        
        apply_luma_lpf(buf_ueit_r2_top[s_mod], 2);
        apply_luma_lpf(buf_ueit_r2_top_h[s_mod], 2);
        apply_luma_lpf(buf_ueit_r2_bot[s_mod], 2);
        apply_luma_lpf(buf_ueit_r3_top[s_mod], 2);
        apply_luma_lpf(buf_ueit_r3_top_h[s_mod], 2);
        apply_luma_lpf(buf_ueit_r3_bot[s_mod], 2);
        apply_luma_lpf(buf_ueit_r4[s_mod], 2);
        apply_luma_lpf(buf_ueit_r4_h[s_mod], 2);

        apply_luma_lpf(buf_ueit_cbars_top[s_mod], 2);
        apply_luma_lpf(buf_ueit_cbars_bot[s_mod], 2);
        apply_luma_lpf(buf_ueit_gray[s_mod], 2);
        
        apply_luma_lpf(buf_ueit_stripes[s_mod], 2);
        apply_luma_lpf(buf_ueit_stripes_h[s_mod], 2);
        apply_luma_lpf(buf_ueit_slanted[s_mod], 2); 
        apply_luma_lpf(buf_ueit_slanted_h[s_mod], 2); 
        apply_luma_lpf(buf_ueit_slanted_2[s_mod], 2);
        apply_luma_lpf(buf_ueit_slanted_2_h[s_mod], 2);
        apply_luma_lpf(buf_ueit_gradient[s_mod], 2);
        apply_luma_lpf(buf_ueit_gradient_h[s_mod], 2);

        apply_luma_lpf(buf_ueit_r15[s_mod], 2);
        apply_luma_lpf(buf_ueit_r15_h[s_mod], 2);
        apply_luma_lpf(buf_ueit_r16_top[s_mod], 2);
        apply_luma_lpf(buf_ueit_r16_top_h[s_mod], 2);
        apply_luma_lpf(buf_ueit_r16_bot[s_mod], 2);
        apply_luma_lpf(buf_ueit_r17_top[s_mod], 2);
        apply_luma_lpf(buf_ueit_r17_top_h[s_mod], 2);
        apply_luma_lpf(buf_ueit_r17_bot[s_mod], 2);

        // --- 3. INJECT SHARP 4.43MHz COLOR VECTORS & GRATINGS ---
        // Y=5, 6: Top Color Bars
        int bw = 105;
        fill_color_bar(buf_ueit_cbars_top[s_mod], b_start + 0*bw, bw, 198, 0, 0, s_mod);     
        fill_color_bar(buf_ueit_cbars_top[s_mod], b_start + 1*bw, bw, 190, -35, 8, s_mod); 
        fill_color_bar(buf_ueit_cbars_top[s_mod], b_start + 2*bw, bw, 182, 12, -50, s_mod);   
        fill_color_bar(buf_ueit_cbars_top[s_mod], b_start + 3*bw, bw, 174, -23, -42, s_mod);  
        fill_color_bar(buf_ueit_cbars_top[s_mod], b_start + 4*bw, bw, 166, 23, 42, s_mod);  
        fill_color_bar(buf_ueit_cbars_top[s_mod], b_start + 5*bw, bw, 158, -12, 50, s_mod); 
        fill_color_bar(buf_ueit_cbars_top[s_mod], b_start + 6*bw, bw, 150, 35, -8, s_mod);   
        fill_color_bar(buf_ueit_cbars_top[s_mod], b_start + 7*bw, bw, 142, 0, 0, s_mod);

        // Y=7: Grayscale
        int g_start = b_start + 105;
        for(int x=b_start; x<g_start; x++) buf_ueit_gray[s_mod][x] = BLACK_LEVEL;
        int gw = 70; 
        int g_levels[9] = {128, 140, 152, 164, 176, 188, 200, 212, 224};
        for (int b = 0; b < 9; b++) {
            for (int x = g_start + b*gw; x < g_start + (b+1)*gw; x++) {
                buf_ueit_gray[s_mod][x] = g_levels[b];
            }
        }
        for(int x=g_start+630; x<=g_start+735; x++) buf_ueit_gray[s_mod][x] = BLACK_LEVEL; 

        // Y=8: Contrasting Color Stripes
        for (int i = 0; i < 32; i++) {
            int block;
            if (i < 10) block = 0;
            else if (i < 22) block = 1;
            else block = 2;
            
            bool is_first = (i % 2 == 0);
            int Y_val, U_val, V_val;
            
            if (block == 0) { 
                if (is_first) { Y_val = 174; U_val = -23; V_val = -42; }
                else          { Y_val = 166; U_val = 23;  V_val = 42;  }
            } else if (block == 1) { 
                if (is_first) { Y_val = 150; U_val = 35;  V_val = -8;  }
                else          { Y_val = 190; U_val = -35; V_val = 8;   }
            } else { 
                if (is_first) { Y_val = 158; U_val = -12; V_val = 50;  }
                else          { Y_val = 182; U_val = 12;  V_val = -50; }
            }
            
            int stripe_start = b_start + 140 + (i * 35) / 2;
            int next_start = b_start + 140 + ((i + 1) * 35) / 2;
            int exact_w = next_start - stripe_start;
            
            fill_color_bar(buf_ueit_stripes[s_mod], stripe_start, exact_w, Y_val, U_val, V_val, s_mod);
            fill_color_bar(buf_ueit_stripes_h[s_mod], stripe_start, exact_w, Y_val, U_val, V_val, s_mod);
        }

        // Y=11: Smooth Color Transition
        fill_color_transition(buf_ueit_gradient[s_mod], b_start+140, 560, 180.0f, -31.0f, -56.0f, 160.0f, 31.0f, 56.0f, s_mod);
        fill_color_transition(buf_ueit_gradient_h[s_mod], b_start+140, 560, 180.0f, -31.0f, -56.0f, 160.0f, 31.0f, 56.0f, s_mod);

        // Y=12: Gratings
        int freq_w = 120;
        fill_ueit_grating(buf_ueit_gratings[s_mod], b_start + 0*freq_w, freq_w, 2.0f);
        fill_ueit_grating(buf_ueit_gratings[s_mod], b_start + 1*freq_w, freq_w, 3.0f);
        fill_ueit_grating(buf_ueit_gratings[s_mod], b_start + 2*freq_w, freq_w, 4.0f);
        fill_ueit_grating(buf_ueit_gratings[s_mod], b_start + 3*freq_w, freq_w, 5.0f);
        fill_ueit_grating(buf_ueit_gratings[s_mod], b_start + 4*freq_w, freq_w, 4.0f);
        fill_ueit_grating(buf_ueit_gratings[s_mod], b_start + 5*freq_w, freq_w, 3.0f);
        fill_ueit_grating(buf_ueit_gratings[s_mod], b_start + 6*freq_w, freq_w, 2.0f);

        // Y=13, 14: Bottom Color Bars
        int lbw = 105;
        fill_color_bar(buf_ueit_cbars_bot[s_mod], b_start + 0*lbw, lbw, 197, 0, 0, s_mod); 
        fill_color_bar(buf_ueit_cbars_bot[s_mod], b_start + 1*lbw, lbw, 188, -35, 8, s_mod); 
        fill_color_bar(buf_ueit_cbars_bot[s_mod], b_start + 2*lbw, lbw, 173, 12, -50, s_mod);   
        fill_color_bar(buf_ueit_cbars_bot[s_mod], b_start + 3*lbw, lbw, 163, -23, -42, s_mod);  
        fill_color_bar(buf_ueit_cbars_bot[s_mod], b_start + 4*lbw, lbw, 150, 23, 42, s_mod);  
        fill_color_bar(buf_ueit_cbars_bot[s_mod], b_start + 5*lbw, lbw, 140, -12, 50, s_mod); 
        fill_color_bar(buf_ueit_cbars_bot[s_mod], b_start + 6*lbw, lbw, 125, 35, -8, s_mod); 
        fill_color_bar(buf_ueit_cbars_bot[s_mod], b_start + 7*lbw, lbw, 116, 0, 0, s_mod); 
    }

    // Mapping Engine
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
            if (frame_y < 8 || frame_y >= 568) {
                base_table[i] = buf_blank[s_mod]; special_table[i] = buf_blank[s_mod];
                continue;
            }

            int cell_y = (frame_y - 8) / 28;
            int mod_y = (frame_y - 8) % 28;
            bool is_h_grid = (mod_y <= 1); 
            bool is_even = (cell_y % 2 == 0);
            
            uint8_t *base_buf;
            if (cell_y == 0) base_buf = buf_ueit_top[s_mod];
            else if (cell_y == 19) base_buf = buf_ueit_bot[s_mod];
            else {
                if (is_even) base_buf = is_h_grid ? buf_ueit_even_h[s_mod] : buf_ueit_even[s_mod];
                else base_buf = is_h_grid ? buf_ueit_odd_h[s_mod] : buf_ueit_odd[s_mod];
            }
            
            base_table[i] = base_buf;

            if (cell_y == 2) {
                if (mod_y < 14) special_table[i] = is_h_grid ? buf_ueit_r2_top_h[s_mod] : buf_ueit_r2_top[s_mod];
                else special_table[i] = buf_ueit_r2_bot[s_mod]; 
            }
            else if (cell_y == 3) {
                if (mod_y < 14) special_table[i] = is_h_grid ? buf_ueit_r3_top_h[s_mod] : buf_ueit_r3_top[s_mod];
                else special_table[i] = buf_ueit_r3_bot[s_mod];
            }
            else if (cell_y == 4) special_table[i] = is_h_grid ? buf_ueit_r4_h[s_mod] : buf_ueit_r4[s_mod];
            else if (cell_y == 5 || cell_y == 6) special_table[i] = buf_ueit_cbars_top[s_mod];
            else if (cell_y == 7) special_table[i] = buf_ueit_gray[s_mod];
            else if (cell_y == 8) special_table[i] = is_h_grid ? buf_ueit_stripes_h[s_mod] : buf_ueit_stripes[s_mod];
            else if (cell_y == 9) special_table[i] = is_h_grid ? buf_ueit_slanted_h[s_mod] : buf_ueit_slanted[s_mod];
            else if (cell_y == 10) special_table[i] = is_h_grid ? buf_ueit_slanted_2_h[s_mod] : buf_ueit_slanted_2[s_mod];
            else if (cell_y == 11) special_table[i] = is_h_grid ? buf_ueit_gradient_h[s_mod] : buf_ueit_gradient[s_mod];
            else if (cell_y == 12) special_table[i] = buf_ueit_gratings[s_mod];
            else if (cell_y == 13 || cell_y == 14) special_table[i] = buf_ueit_cbars_bot[s_mod];
            else if (cell_y == 15) special_table[i] = is_h_grid ? buf_ueit_r15_h[s_mod] : buf_ueit_r15[s_mod];
            else if (cell_y == 16) {
                if (mod_y < 14) special_table[i] = is_h_grid ? buf_ueit_r16_top_h[s_mod] : buf_ueit_r16_top[s_mod];
                else special_table[i] = buf_ueit_r16_bot[s_mod];
            }
            else if (cell_y == 17) {
                if (mod_y < 14) special_table[i] = is_h_grid ? buf_ueit_r17_top_h[s_mod] : buf_ueit_r17_top[s_mod];
                else special_table[i] = buf_ueit_r17_bot[s_mod];
            }
            else special_table[i] = base_buf;
        }
    }
}
