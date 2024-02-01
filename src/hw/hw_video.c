/* double buffering + 2 aux buffers */
#define NUM_VIDEOBUF    4

/* buffers used by UI */
static uint8_t *video_buf[NUM_VIDEOBUF];
static int video_bufw;
static int video_bufh;
static int video_bufi;

/* palette as set by UI, 6bpp */
static uint8_t video_pal[256 * 3];

int hw_video_init(int w, int h)
{
    if (hw_video_init_do(w, h)) {
        return -1;
    }
    video_bufw = w;
    video_bufh = h;
    video_buf[0] = lib_malloc(w * h * NUM_VIDEOBUF);
    for (int i = 1; i < NUM_VIDEOBUF; ++i) {
        video_buf[i] = video_buf[0] + (w * h * i);
    }
    video_bufi = 0;
    memset(video_pal, 0, sizeof(video_pal));
    hw_video_refresh_palette();
    return 0;
}

void hw_video_shutdown(void)
{
    hw_video_shutdown_do();
    lib_free(video_buf[0]);
    for (int i = 0; i < NUM_VIDEOBUF; ++i) {
        video_buf[i] = NULL;
    }
}

void hw_video_refresh(int front)
{
    if (hw_video_lock()) {
        return;
    }
    video.render(video_buf[video_bufi ^ front]);
    hw_video_unlock();
    video.update();
}

void hw_video_update(void)
{
    video.update();
}

void hw_video_set_palette(const uint8_t *pal, int first, int num)
{
    memcpy(&video_pal[first * 3], pal, num * 3);
    video.setpal(pal, first, num);
}

void hw_video_set_palette_byte(int i, uint8_t b)
{
    video_pal[i] = b & 0x3f;
}

void hw_video_set_palette_color(int i, uint8_t r, uint8_t g, uint8_t b)
{
    int j = i * 3;
    hw_video_set_palette_byte(j, r);
    hw_video_set_palette_byte(j + 1, g);
    hw_video_set_palette_byte(j + 2, b);
}

void hw_video_refresh_palette(void)
{
    video.setpal(video_pal, 0, 256);
}

uint8_t *hw_video_get_buf(void)
{
    return video_buf[video_bufi];
}

uint8_t *hw_video_get_buf_front(void)
{
    return video_buf[video_bufi ^ 1];
}

uint8_t *hw_video_draw_buf(void)
{
    hw_video_refresh(0);
    video_bufi ^= 1;
    return video_buf[video_bufi];
}

void hw_video_redraw_front(void)
{
    hw_video_refresh(1);
}

void hw_video_copy_buf(void)
{
    memcpy(video_buf[video_bufi], video_buf[video_bufi ^ 1], video_bufw * video_bufh);
}

void hw_video_copy_buf_out(uint8_t *buf)
{
    memcpy(buf, video_buf[video_bufi], video_bufw * video_bufh);
}

void hw_video_copy_back_to_page2(void)
{
    memcpy(video_buf[2], video_buf[video_bufi], video_bufw * video_bufh);
}

void hw_video_copy_back_from_page2(void)
{
    memcpy(video_buf[video_bufi], video_buf[2], video_bufw * video_bufh);
}

void hw_video_copy_back_to_page3(void)
{
    memcpy(video_buf[3], video_buf[video_bufi], video_bufw * video_bufh);
}

void hw_video_copy_back_from_page3(void)
{
    memcpy(video_buf[video_bufi], video_buf[3], video_bufw * video_bufh);
}
