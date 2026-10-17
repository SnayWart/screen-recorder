#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"
extern const struct wl_interface wl_buffer_interface;
extern const struct wl_interface wl_output_interface;
extern const struct wl_interface zwlr_screencopy_frame_v1_interface;
static const struct wl_interface *wlr_screencopy_unstable_v1_types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	&zwlr_screencopy_frame_v1_interface,
	NULL,
	&wl_output_interface,
	&zwlr_screencopy_frame_v1_interface,
	NULL,
	&wl_output_interface,
	NULL,
	NULL,
	NULL,
	NULL,
	&wl_buffer_interface,
	&wl_buffer_interface,
};
static const struct wl_message zwlr_screencopy_manager_v1_requests[] = {
	{ "capture_output", "nio", wlr_screencopy_unstable_v1_types + 4 },
	{ "capture_output_region", "nioiiii", wlr_screencopy_unstable_v1_types + 7 },
	{ "destroy", "", wlr_screencopy_unstable_v1_types + 0 },
};
WL_EXPORT const struct wl_interface zwlr_screencopy_manager_v1_interface = {
	"zwlr_screencopy_manager_v1", 3,
	3, zwlr_screencopy_manager_v1_requests,
	0, NULL,
};
static const struct wl_message zwlr_screencopy_frame_v1_requests[] = {
	{ "copy", "o", wlr_screencopy_unstable_v1_types + 14 },
	{ "destroy", "", wlr_screencopy_unstable_v1_types + 0 },
	{ "copy_with_damage", "2o", wlr_screencopy_unstable_v1_types + 15 },
};
static const struct wl_message zwlr_screencopy_frame_v1_events[] = {
	{ "buffer", "uuuu", wlr_screencopy_unstable_v1_types + 0 },
	{ "flags", "u", wlr_screencopy_unstable_v1_types + 0 },
	{ "ready", "uuu", wlr_screencopy_unstable_v1_types + 0 },
	{ "failed", "", wlr_screencopy_unstable_v1_types + 0 },
	{ "damage", "2uuuu", wlr_screencopy_unstable_v1_types + 0 },
	{ "linux_dmabuf", "3uuu", wlr_screencopy_unstable_v1_types + 0 },
	{ "buffer_done", "3", wlr_screencopy_unstable_v1_types + 0 },
};
WL_EXPORT const struct wl_interface zwlr_screencopy_frame_v1_interface = {
	"zwlr_screencopy_frame_v1", 3,
	3, zwlr_screencopy_frame_v1_requests,
	7, zwlr_screencopy_frame_v1_events,
};
