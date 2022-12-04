#define USE_XCB
#if defined(USE_XCB)
#include <xcb/xcb.h>
#else
# error "Platform implementation for a Windowing system is missing"
#endif

#include <vulkan/vulkan.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h> // printf

struct XWindow
{
#if defined(USE_XCB)
	xcb_connection_t *connection;
	xcb_window_t window;
#endif
};

XWindow CreateXWindow()
{
	// X11

#if defined(USE_XCB)

	// Connect to the X server
	xcb_connection_t *connection = xcb_connect(NULL, NULL);

	// Get the first screen
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t * screen = iter.data;

	// Configure events to capture
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t values[2] = {
		screen->black_pixel,
		XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE    |
		XCB_EVENT_MASK_BUTTON_PRESS   | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE       |
		XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW 
	};

	// Create a window
	xcb_window_t window = xcb_generate_id(connection);
	xcb_create_window(
		connection,                    // xcb connection
		XCB_COPY_FROM_PARENT,          // depth
		window,                        // window id
		screen->root,                  // parent window
		0, 0,                          // x, y
		150, 150,                      // width, height
		0,                             // bnorder_width
		XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
		screen->root_visual,           // visual
		mask, values);                 // value_mask, value_list

	// Map the window to the screen
	xcb_map_window(connection, window);
	
	// Flush the commands before continuing
	xcb_flush(connection);

#endif

	// Vulkan

#if 0
	VkInstance vkInstance;

	#define VK_ALLOCATOR 0
	VkXcbSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
	createInfo.flags = 0;
	VkSurfaceKHR surface;
	vkCreateXcbSurfaceKHR(vkInstance, &createInfo, VK_ALLOCATOR, &surface);
#endif


#if defined(USE_XCB)
	XWindow xwindow = {};
	xwindow.connection = connection;
	xwindow.window = window;
#endif

	return xwindow;
}

void CloseXWindow(XWindow &window)
{
#if defined(USE_XCB)
	xcb_destroy_window(window.connection, window.window);
	xcb_disconnect(window.connection);
#endif
}

#if defined(USE_XCB)
void
print_modifiers (uint32_t mask)
{
	const char **mod, *mods[] = {
		"Shift", "Lock", "Ctrl", "Alt",
		"Mod2", "Mod3", "Mod4", "Mod5",
		"Button1", "Button2", "Button3", "Button4", "Button5"
	};
	printf ("Modifier mask: ");
	for (mod = mods ; mask; mask >>= 1, mod++)
		if (mask & 1)
			printf(*mod);
	putchar ('\n');
}
#endif

int main(int argc, char **argv)
{
	// Create Window
	XWindow window = CreateXWindow();

	// Input loop
#if defined(USE_XCB)
	xcb_generic_event_t *event;
#endif

	bool gGlobalExit = false;

	while ( !gGlobalExit )
	{
#if defined(USE_XCB)
		while ( (event = xcb_poll_for_event(window.connection)) != 0 )
		{
			printf("Event received.\n");
			switch ( event->response_type & ~0x80 )
			{

				case XCB_EXPOSE:
					{
						xcb_expose_event_t *ev = (xcb_expose_event_t *)event;

						printf ("Window %ld exposed. Region to be redrawn at location (%d,%d), with dimension (%d,%d)\n",
								ev->window, ev->x, ev->y, ev->width, ev->height);
						break;
					}

				case XCB_BUTTON_PRESS:
					{
						xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
						print_modifiers(ev->state);

						switch (ev->detail) {
							case 4:
								printf ("Wheel Button up in window %ld, at coordinates (%d,%d)\n",
										ev->event, ev->event_x, ev->event_y);
								break;
							case 5:
								printf ("Wheel Button down in window %ld, at coordinates (%d,%d)\n",
										ev->event, ev->event_x, ev->event_y);
								break;
							default:
								printf ("Button %d pressed in window %ld, at coordinates (%d,%d)\n",
										ev->detail, ev->event, ev->event_x, ev->event_y);
						}
						break;
					}

				case XCB_BUTTON_RELEASE:
					{
						xcb_button_release_event_t *ev = (xcb_button_release_event_t *)event;
						print_modifiers(ev->state);

						printf ("Button %d released in window %ld, at coordinates (%d,%d)\n",
								ev->detail, ev->event, ev->event_x, ev->event_y);
						break;
					}

				case XCB_MOTION_NOTIFY:
					{
						xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)event;

						printf ("Mouse moved in window %ld, at coordinates (%d,%d)\n",
								ev->event, ev->event_x, ev->event_y);
						break;
					}
				case XCB_ENTER_NOTIFY:
					{
						xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)event;

						printf ("Mouse entered window %ld, at coordinates (%d,%d)\n",
								ev->event, ev->event_x, ev->event_y);
						break;
					}

				case XCB_LEAVE_NOTIFY:
					{
						xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)event;

						printf ("Mouse left window %ld, at coordinates (%d,%d)\n",
								ev->event, ev->event_x, ev->event_y);
						break;
					}

				case XCB_KEY_PRESS:
					{
						xcb_key_press_event_t *ev = (xcb_key_press_event_t *)event;
						print_modifiers(ev->state);

						printf ("Key pressed in window %ld\n", ev->event);
						break;
					}

				case XCB_KEY_RELEASE:
					{
						xcb_key_release_event_t *ev = (xcb_key_release_event_t *)event;
						print_modifiers(ev->state);

						printf ("Key released in window %ld\n", ev->event);

						if ( ev->detail == 9 ) // 9 is code for Escape
						{
							gGlobalExit = true;
						}
						break;
					}

				default:
					/* Unknown event type, ignore it */
					printf("Unknown event: %d\n", event->response_type);
					break;
			}

			free(event);
		}
#endif
	}

	// Close window
	CloseXWindow(window);
	return 1;
}

