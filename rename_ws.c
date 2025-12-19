#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s \"New Name\"\n", argv[0]);
        return 1;
    }

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;

    Window root = DefaultRootWindow(dpy);
    Atom net_names = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);
    Atom net_current = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    Atom net_number = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
    Atom utf8 = XInternAtom(dpy, "UTF8_STRING", False);

    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

    // 1. Get total number of desktops
    XGetWindowProperty(dpy, root, net_number, 0, 1, False, XA_CARDINAL,
                       &actual_type, &actual_format, &nitems, &bytes_after, &data);
    int total_desktops = (data) ? *(long *)data : 40;
    if (data) XFree(data);

    // 2. Get current desktop index
    XGetWindowProperty(dpy, root, net_current, 0, 1, False, XA_CARDINAL,
                       &actual_type, &actual_format, &nitems, &bytes_after, &data);
    long current_idx = (data) ? *(long *)data : 0;
    if (data) XFree(data);

    // 3. Get all existing names
    XGetWindowProperty(dpy, root, net_names, 0, 4096, False, utf8,
                       &actual_type, &actual_format, &nitems, &bytes_after, &data);

    // 4. Reconstruct the names list with the new name
    char *new_buffer = malloc(16384);
    int offset = 0;
    unsigned char *ptr = data;

    for (int i = 0; i < total_desktops; i++) {
        if (i == current_idx) {
            // Inject the new name
            strcpy(new_buffer + offset, argv[1]);
            offset += strlen(argv[1]) + 1;

            // Advance original pointer past the old name
            if (ptr && ptr < (data + nitems)) {
                ptr += strlen((char*)ptr) + 1;
            }
        } else {
            // Copy existing name if available, else use a placeholder
            if (ptr && ptr < (data + nitems) && strlen((char*)ptr) > 0) {
                strcpy(new_buffer + offset, (char*)ptr);
                offset += strlen((char*)ptr) + 1;
                ptr += strlen((char*)ptr) + 1;
            } else {
                int len = sprintf(new_buffer + offset, "%d", i + 1);
                offset += len + 1;
            }
        }
    }

    // 5. Set the property correctly (using PropModeReplace)
    XChangeProperty(dpy, root, net_names, utf8, 8, PropModeReplace,
                    (unsigned char *)new_buffer, offset);

    XFlush(dpy);
    XCloseDisplay(dpy);
    free(new_buffer);
    if (data) XFree(data);

    return 0;
}
