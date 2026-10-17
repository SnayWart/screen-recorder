#ifndef WLR_SCREENCOPY_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define WLR_SCREENCOPY_UNSTABLE_V1_CLIENT_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"
#ifdef  __cplusplus
extern "C" {
#endif
struct wl_buffer;
struct wl_output;
struct zwlr_screencopy_frame_v1;
struct zwlr_screencopy_manager_v1;
#ifndef ZWLR_SCREENCOPY_MANAGER_V1_INTERFACE
#define ZWLR_SCREENCOPY_MANAGER_V1_INTERFACE
extern const struct wl_interface zwlr_screencopy_manager_v1_interface;
#endif
#ifndef ZWLR_SCREENCOPY_FRAME_V1_INTERFACE
#define ZWLR_SCREENCOPY_FRAME_V1_INTERFACE
extern const struct wl_interface zwlr_screencopy_frame_v1_interface;
#endif
#define ZWLR_SCREENCOPY_MANAGER_V1_CAPTURE_OUTPUT 0
#define ZWLR_SCREENCOPY_MANAGER_V1_CAPTURE_OUTPUT_REGION 1
#define ZWLR_SCREENCOPY_MANAGER_V1_DESTROY 2
#define ZWLR_SCREENCOPY_MANAGER_V1_CAPTURE_OUTPUT_SINCE_VERSION 1
#define ZWLR_SCREENCOPY_MANAGER_V1_CAPTURE_OUTPUT_REGION_SINCE_VERSION 1
#define ZWLR_SCREENCOPY_MANAGER_V1_DESTROY_SINCE_VERSION 1
static inline void
zwlr_screencopy_manager_v1_set_user_data(struct zwlr_screencopy_manager_v1 *zwlr_screencopy_manager_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zwlr_screencopy_manager_v1, user_data);
}
static inline void *
zwlr_screencopy_manager_v1_get_user_data(struct zwlr_screencopy_manager_v1 *zwlr_screencopy_manager_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zwlr_screencopy_manager_v1);
}
static inline uint32_t
zwlr_screencopy_manager_v1_get_version(struct zwlr_screencopy_manager_v1 *zwlr_screencopy_manager_v1)
{
	return wl_proxy_get_version((struct wl_proxy *) zwlr_screencopy_manager_v1);
}
static inline struct zwlr_screencopy_frame_v1 *
zwlr_screencopy_manager_v1_capture_output(struct zwlr_screencopy_manager_v1 *zwlr_screencopy_manager_v1, int32_t overlay_cursor, struct wl_output *output)
{
	struct wl_proxy *frame;
	frame = wl_proxy_marshal_flags((struct wl_proxy *) zwlr_screencopy_manager_v1,
			 ZWLR_SCREENCOPY_MANAGER_V1_CAPTURE_OUTPUT, &zwlr_screencopy_frame_v1_interface, wl_proxy_get_version((struct wl_proxy *) zwlr_screencopy_manager_v1), 0, NULL, overlay_cursor, output);
	return (struct zwlr_screencopy_frame_v1 *) frame;
}
static inline struct zwlr_screencopy_frame_v1 *
zwlr_screencopy_manager_v1_capture_output_region(struct zwlr_screencopy_manager_v1 *zwlr_screencopy_manager_v1, int32_t overlay_cursor, struct wl_output *output, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct wl_proxy *frame;
	frame = wl_proxy_marshal_flags((struct wl_proxy *) zwlr_screencopy_manager_v1,
			 ZWLR_SCREENCOPY_MANAGER_V1_CAPTURE_OUTPUT_REGION, &zwlr_screencopy_frame_v1_interface, wl_proxy_get_version((struct wl_proxy *) zwlr_screencopy_manager_v1), 0, NULL, overlay_cursor, output, x, y, width, height);
	return (struct zwlr_screencopy_frame_v1 *) frame;
}
static inline void
zwlr_screencopy_manager_v1_destroy(struct zwlr_screencopy_manager_v1 *zwlr_screencopy_manager_v1)
{
	wl_proxy_marshal_flags((struct wl_proxy *) zwlr_screencopy_manager_v1,
			 ZWLR_SCREENCOPY_MANAGER_V1_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) zwlr_screencopy_manager_v1), WL_MARSHAL_FLAG_DESTROY);
}
#ifndef ZWLR_SCREENCOPY_FRAME_V1_ERROR_ENUM
#define ZWLR_SCREENCOPY_FRAME_V1_ERROR_ENUM
enum zwlr_screencopy_frame_v1_error {
	ZWLR_SCREENCOPY_FRAME_V1_ERROR_ALREADY_USED = 0,
	ZWLR_SCREENCOPY_FRAME_V1_ERROR_INVALID_BUFFER = 1,
};
#endif  
#ifndef ZWLR_SCREENCOPY_FRAME_V1_FLAGS_ENUM
#define ZWLR_SCREENCOPY_FRAME_V1_FLAGS_ENUM
enum zwlr_screencopy_frame_v1_flags {
	ZWLR_SCREENCOPY_FRAME_V1_FLAGS_Y_INVERT = 1,
};
#endif  
struct zwlr_screencopy_frame_v1_listener {
	void (*buffer)(void *data,
		       struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
		       uint32_t format,
		       uint32_t width,
		       uint32_t height,
		       uint32_t stride);
	void (*flags)(void *data,
		      struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
		      uint32_t flags);
	void (*ready)(void *data,
		      struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
		      uint32_t tv_sec_hi,
		      uint32_t tv_sec_lo,
		      uint32_t tv_nsec);
	void (*failed)(void *data,
		       struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1);
	void (*damage)(void *data,
		       struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
		       uint32_t x,
		       uint32_t y,
		       uint32_t width,
		       uint32_t height);
	void (*linux_dmabuf)(void *data,
			     struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
			     uint32_t format,
			     uint32_t width,
			     uint32_t height);
	void (*buffer_done)(void *data,
			    struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1);
};
static inline int
zwlr_screencopy_frame_v1_add_listener(struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1,
				      const struct zwlr_screencopy_frame_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwlr_screencopy_frame_v1,
				     (void (**)(void)) listener, data);
}
#define ZWLR_SCREENCOPY_FRAME_V1_COPY 0
#define ZWLR_SCREENCOPY_FRAME_V1_DESTROY 1
#define ZWLR_SCREENCOPY_FRAME_V1_COPY_WITH_DAMAGE 2
#define ZWLR_SCREENCOPY_FRAME_V1_BUFFER_SINCE_VERSION 1
#define ZWLR_SCREENCOPY_FRAME_V1_FLAGS_SINCE_VERSION 1
#define ZWLR_SCREENCOPY_FRAME_V1_READY_SINCE_VERSION 1
#define ZWLR_SCREENCOPY_FRAME_V1_FAILED_SINCE_VERSION 1
#define ZWLR_SCREENCOPY_FRAME_V1_DAMAGE_SINCE_VERSION 2
#define ZWLR_SCREENCOPY_FRAME_V1_LINUX_DMABUF_SINCE_VERSION 3
#define ZWLR_SCREENCOPY_FRAME_V1_BUFFER_DONE_SINCE_VERSION 3
#define ZWLR_SCREENCOPY_FRAME_V1_COPY_SINCE_VERSION 1
#define ZWLR_SCREENCOPY_FRAME_V1_DESTROY_SINCE_VERSION 1
#define ZWLR_SCREENCOPY_FRAME_V1_COPY_WITH_DAMAGE_SINCE_VERSION 2
static inline void
zwlr_screencopy_frame_v1_set_user_data(struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zwlr_screencopy_frame_v1, user_data);
}
static inline void *
zwlr_screencopy_frame_v1_get_user_data(struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zwlr_screencopy_frame_v1);
}
static inline uint32_t
zwlr_screencopy_frame_v1_get_version(struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1)
{
	return wl_proxy_get_version((struct wl_proxy *) zwlr_screencopy_frame_v1);
}
static inline void
zwlr_screencopy_frame_v1_copy(struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1, struct wl_buffer *buffer)
{
	wl_proxy_marshal_flags((struct wl_proxy *) zwlr_screencopy_frame_v1,
			 ZWLR_SCREENCOPY_FRAME_V1_COPY, NULL, wl_proxy_get_version((struct wl_proxy *) zwlr_screencopy_frame_v1), 0, buffer);
}
static inline void
zwlr_screencopy_frame_v1_destroy(struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1)
{
	wl_proxy_marshal_flags((struct wl_proxy *) zwlr_screencopy_frame_v1,
			 ZWLR_SCREENCOPY_FRAME_V1_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) zwlr_screencopy_frame_v1), WL_MARSHAL_FLAG_DESTROY);
}
static inline void
zwlr_screencopy_frame_v1_copy_with_damage(struct zwlr_screencopy_frame_v1 *zwlr_screencopy_frame_v1, struct wl_buffer *buffer)
{
	wl_proxy_marshal_flags((struct wl_proxy *) zwlr_screencopy_frame_v1,
			 ZWLR_SCREENCOPY_FRAME_V1_COPY_WITH_DAMAGE, NULL, wl_proxy_get_version((struct wl_proxy *) zwlr_screencopy_frame_v1), 0, buffer);
}
#ifdef  __cplusplus
}
#endif
#endif
