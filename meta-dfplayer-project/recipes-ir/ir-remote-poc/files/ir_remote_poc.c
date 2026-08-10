#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

/*
 * Minimal proof-of-concept: reads raw IR scancodes from a kernel rc-core
 * input device (e.g. /dev/input/event0, bound to gpio_ir_recv) and prints
 * each distinct scancode as it's received.
 *
 * Find the right device with: ir-keytable   (lists rc0 -> eventN mapping)
 */

int main(int argc, char *argv[])
{
    const char *device = (argc > 1) ? argv[1] : "/dev/input/event0";

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("open");
        fprintf(stderr, "Usage: %s [/dev/input/eventN]\n", argv[0]);
        return 1;
    }

    printf("Listening for IR scancodes on %s (Ctrl-C to exit)...\n", device);

    struct input_event ev;
    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
        /* rc-core reports the decoded scancode as an EV_MSC / MSC_SCAN event */
        if (ev.type == EV_MSC && ev.code == MSC_SCAN) {
            printf("scancode = 0x%08x\n", ev.value);
        }
    }

    close(fd);
    return 0;
}
