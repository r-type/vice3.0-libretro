#ifndef _VIDEOARCH_H
#define _VIDEOARCH_H

typedef struct video_canvas_s {

    /** \brief Nonzero if the structure has been fully realized. */
    unsigned int created;

    /** \brief Rendering configuration as seen by the emulator
     *         core. */
    struct video_render_config_s *videoconfig;

    /** \brief Drawing buffer as seen by the emulator core. */
    struct draw_buffer_s *draw_buffer;

    /** \brief Display window as seen by the emulator core. */
    struct viewport_s *viewport;

    /** \brief Machine screen geometry as seen by the emulator
     *         core. */
    struct geometry_s *geometry;

    /** \brief Color palette for translating display results into
     *         window colors. */
    struct palette_s *palette;

    /** \brief Methods for managing the draw buffer when the core
     *         rasterizer handles it. */
    struct video_draw_buffer_callback_s *video_draw_buffer_callback;
    
    unsigned int depth;
} video_canvas_t;

typedef struct vice_renderer_backend_s {
} vice_renderer_backend_t;

#endif
