#include <strato/strato.h>

HAL_DEFINE_MAIN()

static bool on_start(void);
static bool on_update(void);
static void on_render(void);

char window_title = "Hello";

bool hal_bootstrap(char **title, int *width, int *height, struct hal_callback *cb)
{
	*title = window_title;
	*width = 640;
	*height = 480;

	cb->on_start = on_start;
	cb->on_update = on_update;
	cb->on_render = on_render;

	return true;
}

static bool on_start(void)
{
	return true;
}

static bool on_update(void)
{
	return true;
}

static void on_render(void)
{
}
