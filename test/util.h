typedef struct RTP_header RTP_header;

struct RTP_header {
    unsigned char v_p_x_cc;
    unsigned char m_pt;
    unsigned short seq;
    unsigned int timestamp;
    unsigned int ssrc;
};