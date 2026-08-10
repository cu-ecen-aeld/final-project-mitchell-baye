#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <linux/input.h>

/*
 * Captures IR scancodes from a kernel rc-core input device and logs each
 * DISTINCT scancode to a CSV file, one row per unique code. Duplicate
 * scancodes (repeat presses of the same button) are ignored after the
 * first sighting.
 *
 * Output CSV columns: scancode_hex,scancode_dec,first_seen,button_label
 * (button_label is left blank for you to fill in by hand afterward,
 * once you know which physical button produced which code)
 *
 * Usage: ir_scancode_logger /dev/input/eventN [output.csv]
 */

#define MAX_KNOWN_CODES 256

static uint32_t known_codes[MAX_KNOWN_CODES];
static int known_count = 0;

static int already_seen(uint32_t code)
{
    for (int i = 0; i < known_count; i++) {
        if (known_codes[i] == code) {
            return 1;
        }
    }
    return 0;
}

static void remember(uint32_t code)
{
    if (known_count < MAX_KNOWN_CODES) {
        known_codes[known_count++] = code;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventN [output.csv]\n", argv[0]);
        return 1;
    }

    const char *device = argv[1];
    const char *out_path = (argc > 2) ? argv[2] : "ir_scancodes.csv";

    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("open input device");
        return 1;
    }

    /* Append if the file already exists, so repeated runs build on prior results */
    int is_new_file = (access(out_path, F_OK) != 0);

    FILE *out = fopen(out_path, "a");
    if (!out) {
        perror("open output csv");
        close(fd);
        return 1;
    }

    if (is_new_file) {
        fprintf(out, "scancode_hex,scancode_dec,first_seen,button_label\n");
        fflush(out);
    } else {
        /*
         * File already existed — preload known_codes from it so we don't
         * re-log scancodes captured in a previous run of this program.
         */
        FILE *existing = fopen(out_path, "r");
        if (existing) {
            char line[256];
            fgets(line, sizeof(line), existing); /* skip header */
            while (fgets(line, sizeof(line), existing)) {
                uint32_t code;
                if (sscanf(line, "0x%x,", &code) == 1) {
                    remember(code);
                }
            }
            fclose(existing);
            printf("Loaded %d previously seen scancode(s) from %s\n", known_count, out_path);
        }
    }

    printf("Listening on %s, logging new scancodes to %s\n", device, out_path);
    printf("Press remote buttons now. Ctrl-C to stop.\n");

    struct input_event ev;
    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type != EV_MSC || ev.code != MSC_SCAN) {
            continue;
        }

        uint32_t scancode = (uint32_t)ev.value;

        if (already_seen(scancode)) {
            continue; /* duplicate, ignore */
        }

        remember(scancode);

        time_t now = time(NULL);
        char timestr[32];
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));

        printf("New scancode: 0x%08x (%u)\n", scancode, scancode);

        fprintf(out, "0x%08x,%u,%s,\n", scancode, scancode, timestr);
        fflush(out); /* write immediately in case of Ctrl-C or crash */
    }

    fclose(out);
    close(fd);
    return 0;
}
