// teletext.c

// Calculate Odd Parity for 7-bit ASCII
uint8_t ttx_parity(uint8_t c) {
    int ones = 0;
    for (int i = 0; i < 7; i++) {
        if ((c >> i) & 1) ones++;
    }
    if ((ones % 2) == 0) c |= 0x80; 
    return c;
}

// Calculate Hamming 8/4 Protection for Address/Control Bytes
uint8_t ttx_hamming(uint8_t d) {
    int d1 = (d >> 0) & 1;
    int d2 = (d >> 1) & 1;
    int d3 = (d >> 2) & 1;
    int d4 = (d >> 3) & 1;

    int p1 = (1 + d1 + d3 + d4) % 2;
    int p2 = (1 + d1 + d2 + d4) % 2;
    int p3 = (1 + d1 + d2 + d3) % 2;
    int p4 = (1 + p1 + d1 + p2 + d2 + p3 + d3 + d4) % 2;

    return (p1 << 0) | (d1 << 1) | (p2 << 2) | (d2 << 3) | 
           (p3 << 4) | (d3 << 5) | (p4 << 6) | (d4 << 7);
}

// CORRECTED: Interleaves Magazine and Row bits matching the strict ETS 300 706 broadcast standard
void ttx_mrag(uint8_t mag, uint8_t row, uint8_t *out_b1, uint8_t *out_b2) {
    int m0 = (mag & 1);
    int m1 = (mag >> 1) & 1;
    int m2 = (mag >> 2) & 1;
    int r0 = (row & 1);
    int r1 = (row >> 1) & 1;
    int r2 = (row >> 2) & 1;
    int r3 = (row >> 3) & 1;
    int r4 = (row >> 4) & 1;
    
    // Byte 1 Data: M0, R0, R1, R2
    uint8_t d1 = m0 | (r0 << 1) | (r1 << 2) | (r2 << 3);
    // Byte 2 Data: R3, R4, M1, M2
    uint8_t d2 = r3 | (r4 << 1) | (m1 << 2) | (m2 << 3);
    
    *out_b1 = ttx_hamming(d1);
    *out_b2 = ttx_hamming(d2);
}

// 5-Tap Gaussian filter (0-byte stack, bit-for-bit identical)
void apply_teletext_filter(uint8_t *buf) {
    uint8_t prev2 = buf[186];
    uint8_t prev1 = buf[187];
    for (int x = 188; x < 1134; x++) {
        uint8_t curr = buf[x];
        buf[x] = (uint8_t)((prev2 + (prev1 << 1) + (curr << 1) + (buf[x + 1] << 1) + buf[x + 2]) >> 3);
        prev2 = prev1;
        prev1 = curr;
    }
}

// Float-based Phase Accumulator to render 6.9375 MHz NRZ data into a 17.73 MHz pixel array
void render_teletext_packet(uint8_t *buf, uint8_t *packet, int s_mod) {
    generate_hblank(buf, s_mod);
    for(int x = 186; x < 1136; x++) buf[x] = BLACK_LEVEL;
    
    int pixel_start = 187; 
    float pixels_per_bit = 17.734475f / 6.9375f; 
    
    for (int i = 0; i < 360; i++) {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        bool bit_val = (packet[byte_idx] & (1 << bit_idx)) != 0; 
        
        int px_start = pixel_start + (int)(i * pixels_per_bit);
        int px_end   = pixel_start + (int)((i + 1) * pixels_per_bit);
        uint8_t level = bit_val ? 188 : 116; 
        
        for (int x = px_start; x < px_end; x++) {
            if (x < 1136) buf[x] = level;
        }
    }
    
    // Apply the mathematical Gaussian smoothing
    apply_teletext_filter(buf);
    pack_line_1135(buf, s_mod);
}

void build_teletext_vbi() {
    for (int s_mod = 0; s_mod < 4; s_mod++) {
        uint8_t pkt_hdr[45];
        uint8_t pkt_row1[45];
        uint8_t pkt_row2[45];
        
        // 1. Clock Run-In & Framing Code
        pkt_hdr[0] = 0x55;  pkt_hdr[1] = 0x55;  pkt_hdr[2] = 0x27;
        pkt_row1[0] = 0x55; pkt_row1[1] = 0x55; pkt_row1[2] = 0x27;
        pkt_row2[0] = 0x55; pkt_row2[1] = 0x55; pkt_row2[2] = 0x27;
        
        // 2. Header Packet (Magazine 1, Row 0)
        ttx_mrag(1, 0, &pkt_hdr[3], &pkt_hdr[4]);
        pkt_hdr[5] = ttx_hamming(0x00); // Page Units = 0
        pkt_hdr[6] = ttx_hamming(0x00); // Page Tens = 0
        pkt_hdr[7] = ttx_hamming(0x00); // Subcode S1
        pkt_hdr[8] = ttx_hamming(0x00); // Subcode S2
        pkt_hdr[9] = ttx_hamming(0x00); // Subcode S3
        pkt_hdr[10] = ttx_hamming(0x00); // Subcode S4
        pkt_hdr[11] = ttx_hamming(0x00); // Control
        pkt_hdr[12] = ttx_hamming(0x00); // Control
        char hdr_text[32] = " RP2040 TELETEXT GENERATOR      "; 
        for (int i=0; i<32; i++) pkt_hdr[13+i] = ttx_parity(hdr_text[i]);

        // 3. Row 1 (Red Text)
        ttx_mrag(1, 1, &pkt_row1[3], &pkt_row1[4]);
        char r1_text[40] = " \x01RP2040 Teletext VBI Engine            "; 
        for (int i=0; i<40; i++) pkt_row1[5+i] = ttx_parity(r1_text[i]);
        
        // 4. Row 2 (Green Text)
        ttx_mrag(1, 2, &pkt_row2[3], &pkt_row2[4]);
        char r2_text[40] = " \x02Running at 17.734475 MHz              "; 
        for (int i=0; i<40; i++) pkt_row2[5+i] = ttx_parity(r2_text[i]);

        render_teletext_packet(buf_vbi_ttx_hdr[s_mod], pkt_hdr, s_mod);
        render_teletext_packet(buf_vbi_ttx_row1[s_mod], pkt_row1, s_mod);
        render_teletext_packet(buf_vbi_ttx_row2[s_mod], pkt_row2, s_mod);
    }
}
