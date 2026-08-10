#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "dfplayer_ctl.h"

/*
 * dfplayer-daemon: owns the DFPlayer's UART connection and listens on a
 * unix domain datagram socket for plain-text commands. Any client (the
 * IR remote daemon, a future RF remote daemon, a manual test script)
 * can send a command by writing a line to the socket.
 *
 * Supported command lines (case-sensitive, no trailing newline needed):
 *   PLAY
 *   PAUSE
 *   STOP
 *   NEXT
 *   PREV
 *   TRACK <n>
 *   VOLUME <n>      (0-30)
 *   VOLUP
 *   VOLDOWN
 *
 * Default socket path: /run/dfplayer.sock
 */

#define DEFAULT_SOCKET_PATH "/run/dfplayer.sock"
#define DEFAULT_UART_DEVICE "/dev/ttyAMA1"
#define RECV_BUF_SIZE 128

static int sockfd = -1;
static dfp_handle_t dfp_handle = { .fd = -1 };
static char active_socket_path[256];

static void cleanup_and_exit(int signum)
{
    (void)signum;
    if (sockfd >= 0) {
        close(sockfd);
    }
    if (active_socket_path[0] != '\0') {
        unlink(active_socket_path);
    }
    dfp_close(&dfp_handle);
    _exit(0);
}

static void dispatch_command(const char *line)
{
    char cmd[32] = {0};
    long arg = 0;
    int has_arg = 0;

    /* Split off the first token and an optional numeric argument */
    int matched = sscanf(line, "%31s %ld", cmd, &arg);
    if (matched >= 2) {
        has_arg = 1;
    }
    if (matched < 1) {
        fprintf(stderr, "dfplayer-daemon: ignoring empty/unparseable line\n");
        return;
    }

    int rc = -1;

    if (strcmp(cmd, "PLAY") == 0) {
        rc = dfp_play(&dfp_handle);
    } else if (strcmp(cmd, "PAUSE") == 0) {
        rc = dfp_pause(&dfp_handle);
    } else if (strcmp(cmd, "STOP") == 0) {
        rc = dfp_stop(&dfp_handle);
    } else if (strcmp(cmd, "NEXT") == 0) {
        rc = dfp_next(&dfp_handle);
    } else if (strcmp(cmd, "PREV") == 0) {
        rc = dfp_prev(&dfp_handle);
    } else if (strcmp(cmd, "TRACK") == 0 && has_arg) {
        rc = dfp_play_track(&dfp_handle, (uint16_t)arg);
    } else if (strcmp(cmd, "VOLUME") == 0 && has_arg) {
        if (arg < 0 || arg > 30) {
            fprintf(stderr, "dfplayer-daemon: VOLUME out of range: %ld\n", arg);
            return;
        }
        rc = dfp_set_volume(&dfp_handle, (uint8_t)arg);
    } else if (strcmp(cmd, "VOLUP") == 0) {
        rc = dfp_volume_up(&dfp_handle);
    } else if (strcmp(cmd, "VOLDOWN") == 0) {
        rc = dfp_volume_down(&dfp_handle);
    } else if (strcmp(cmd, "REPEAT_START") == 0) {
        rc = dfp_repeat(&dfp_handle, 1);
    } else if (strcmp(cmd, "REPEAT_STOP") == 0) {
        rc = dfp_repeat(&dfp_handle, 0);
    } else {
        fprintf(stderr, "dfplayer-daemon: unrecognized command: '%s'\n", line);
        return;
    }

    if (rc != 0) {
        fprintf(stderr, "dfplayer-daemon: command '%s' failed to send\n", line);
    } else {
        printf("dfplayer-daemon: executed '%s'\n", line);
    }
}

int main(int argc, char *argv[])
{
    const char *uart_device = (argc > 1) ? argv[1] : DEFAULT_UART_DEVICE;
    const char *socket_path = (argc > 2) ? argv[2] : DEFAULT_SOCKET_PATH;

    setvbuf(stdout, NULL, _IONBF, 0);
    strncpy(active_socket_path, socket_path, sizeof(active_socket_path) - 1);

    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);

    if (dfp_open(&dfp_handle, uart_device) != 0) {
        fprintf(stderr, "dfplayer-daemon: failed to open UART %s\n", uart_device);
        return 1;
    }

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        dfp_close(&dfp_handle);
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path); /* remove stale socket file from a prior run */

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        close(sockfd);
        dfp_close(&dfp_handle);
        return 1;
    }

    printf("dfplayer-daemon: listening on %s, UART %s\n", socket_path, uart_device);

    char buf[RECV_BUF_SIZE];
    for (;;) {
        ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (n < 0) {
            perror("recv");
            continue;
        }
        buf[n] = '\0';

        /* Strip a trailing newline, if a client included one */
        char *newline = strchr(buf, '\n');
        if (newline) {
            *newline = '\0';
        }

        dispatch_command(buf);
    }

    /* unreachable, cleanup happens via signal handler */
    return 0;
}
