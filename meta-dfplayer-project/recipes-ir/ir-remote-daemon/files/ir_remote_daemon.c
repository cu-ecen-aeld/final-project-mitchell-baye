#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <sys/un.h>

/*
 * ir-remote-daemon: reads decoded IR scancodes from a kernel rc-core input
 * device and forwards the corresponding DFPlayer command to dfplayer-daemon
 * over a unix domain socket. Unmapped scancodes are ignored. Repeated
 * scancodes from a held button are debounced.
 *
 * Fill in SCANCODE_MAP below using values captured with ir_scancode_logger.
 */

#define DEFAULT_INPUT_DEVICE "/dev/input/event4"
#define DEFAULT_SOCKET_PATH  "/run/dfplayer.sock"
#define DEBOUNCE_MS 300

typedef struct {
    uint32_t scancode;
    const char *command;
} scancode_map_entry_t;

/*
 * TODO: replace these placeholder scancodes with the real values from
 * your ir_scancodes.csv capture, matched up to whichever physical button
 * you want to perform each action.
 */
static const scancode_map_entry_t SCANCODE_MAP[] = {
    { 0x0000cf41, "PLAY"            }, // Play
    { 0x0000cf58, "PAUSE"           }, // Red
    { 0x0000cf59, "STOP"            }, // Green
    { 0x0000cf45, "NEXT"            }, // Blue
    { 0x0000cf44, "PREV"            }, // White
    { 0x0000cf14, "VOLUP"           }, // Red Up
    { 0x0000cf10, "VOLDOWN"         }, // Red Down
    { 0x0000cf15, "REPEAT_START"    }, // Green Up
    { 0x0000cf11, "REPEAT_STOP"     }, // Green Down
};
#define SCANCODE_MAP_LEN (sizeof(SCANCODE_MAP) / sizeof(SCANCODE_MAP[0]))

static const char *lookup_command(uint32_t scancode)
{
    for (size_t i = 0; i < SCANCODE_MAP_LEN; i++) {
        if (SCANCODE_MAP[i].scancode == scancode) {
            return SCANCODE_MAP[i].command;
        }
    }
    return NULL;
}

static long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000L) + (ts.tv_nsec / 1000000L);
}

int main(int argc, char *argv[])
{
    const char *input_device = (argc > 1) ? argv[1] : DEFAULT_INPUT_DEVICE;
    const char *socket_path  = (argc > 2) ? argv[2] : DEFAULT_SOCKET_PATH;

    setvbuf(stdout, NULL, _IONBF, 0);
    int input_fd = open(input_device, O_RDONLY);
    if (input_fd < 0) {
        perror("open input device");
        return 1;
    }

    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        close(input_fd);
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    printf("ir-remote-daemon: reading %s, forwarding to %s\n", input_device, socket_path);

    uint32_t last_scancode = 0;
    long last_sent_ms = 0;

    struct input_event ev;
    while (read(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type != EV_MSC || ev.code != MSC_SCAN) {
            continue;
        }

        uint32_t scancode = (uint32_t)ev.value;
        long t = now_ms();

        /* Debounce: ignore the same scancode repeating faster than DEBOUNCE_MS */
        if (scancode == last_scancode && (t - last_sent_ms) < DEBOUNCE_MS) {
            continue;
        }

        const char *command = lookup_command(scancode);
        if (!command) {
            printf("ir-remote-daemon: unmapped scancode 0x%08x, ignoring\n", scancode);
            last_scancode = scancode;
            last_sent_ms = t;
            continue;
        }

        ssize_t sent = sendto(sockfd, command, strlen(command), 0,
                               (struct sockaddr *)&addr, sizeof(addr));
        if (sent < 0) {
            perror("sendto");
            /* dfplayer-daemon may not be running yet/right now; keep going */
        } else {
            printf("ir-remote-daemon: scancode 0x%08x -> %s\n", scancode, command);
        }

        last_scancode = scancode;
        last_sent_ms = t;
    }

    close(sockfd);
    close(input_fd);
    return 0;
}
