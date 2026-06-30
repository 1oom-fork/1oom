#ifndef INC_1OOM_VGABUF_H
#define INC_1OOM_VGABUF_H

#define VGABUF_W    320
#define VGABUF_H    200
#define VGABUF_SIZE     (VGABUF_W * VGABUF_H)

#define VGABUF_PITCH    320
#define VGABUF_OFFSET(x, y)   ((x) + (y) * (VGABUF_PITCH))

#define vgabuf_get hw_video_get_buf
#define vgabuf_get_back hw_video_get_buf
#define vgabuf_get_front hw_video_get_buf_front
#define vgabuf_flip hw_video_draw_buf

#define vgabuf_copy_front_to_back hw_video_copy_buf
#define vgabuf_copy_back_out hw_video_copy_buf_out
#define vgabuf_copy_back_to_page2 hw_video_copy_back_to_page2
#define vgabuf_copy_back_from_page2 hw_video_copy_back_from_page2
#define vgabuf_copy_back_to_page3 hw_video_copy_back_to_page3
#define vgabuf_copy_back_from_page3 hw_video_copy_back_from_page3

#endif
