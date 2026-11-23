#include <shell.h>
#include <calc.h>
#include <kprintf.h>
#include <power.h>
#include <print.h>
#include <readline.h>
#include <rtc.h>
#include <stdbool.h>
#include <string.h>
#include <timer.h>

#define MAX_ARGS 16
#define MAX_LINE 128

struct command {
    const char* name;
    const char* usage;
    const char* description;
    int (*run)(int argc, char** argv);
};

static int cmd_help(int argc, char** argv);
static int cmd_clear(int argc, char** argv);
static int cmd_echo(int argc, char** argv);
static int cmd_calc(int argc, char** argv);
static int cmd_color(int argc, char** argv);
static int cmd_uptime(int argc, char** argv);
static int cmd_sleep(int argc, char** argv);
static int cmd_date(int argc, char** argv);
static int cmd_reboot(int argc, char** argv);
static int cmd_shutdown(int argc, char** argv);

static const struct command commands[] = {
    { "help",     "help",              "list the available commands",              cmd_help },
    { "clear",    "clear",             "clear the screen",                         cmd_clear },
    { "echo",     "echo <text...>",    "print text",                               cmd_echo },
    { "calc",     "calc <expression>", "integer math: + - * / % ( ), e.g. (2+3)*4", cmd_calc },
    { "color",    "color <fg> [bg]",   "change the text color (`color` lists them)", cmd_color },
    { "uptime",   "uptime",            "time since the computer started",          cmd_uptime },
    { "sleep",    "sleep <seconds>",   "wait for a number of seconds",             cmd_sleep },
    { "date",     "date",              "current date and time (UTC)",              cmd_date },
    { "reboot",   "reboot",            "restart the computer",                     cmd_reboot },
    { "shutdown", "shutdown",          "power off the computer",                   cmd_shutdown },
};

#define COMMAND_COUNT (sizeof commands / sizeof commands[0])

/* Index = VGA color number, matching the PRINT_COLOR_* values in print.h */
static const char* const color_names[16] = {
    "black", "blue", "green", "cyan", "red", "magenta", "brown", "light-gray",
    "dark-gray", "light-blue", "light-green", "light-cyan", "light-red", "pink",
    "yellow", "white",
};

/* Parse a decimal number no larger than `max`. */
static bool parse_uint(const char* text, uint64_t max, uint64_t* value) {
    if (text[0] == '\0') {
        return false;
    }
    uint64_t number = 0;
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
        number = number * 10 + (uint64_t) (text[i] - '0');
        if (number > max) {
            return false;
        }
    }
    *value = number;
    return true;
}

static int cmd_help(int argc, char** argv) {
    (void) argc;
    (void) argv;
    kprintf("Commands:\n");
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        kprintf("  %-18s %s\n", commands[i].usage, commands[i].description);
    }
    kprintf("Up/Down recall previous commands, Ctrl+C cancels the line.\n");
    return 0;
}

static int cmd_clear(int argc, char** argv) {
    (void) argc;
    (void) argv;
    print_clear();
    return 0;
}

static int cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        kprintf(i > 1 ? " %s" : "%s", argv[i]);
    }
    kprintf("\n");
    return 0;
}

static int cmd_calc(int argc, char** argv) {
    if (argc < 2) {
        kprintf("usage: calc <expression>, e.g. calc (2 + 3) * 4\n");
        return 1;
    }

    /* the shell split the expression on spaces: join it back together */
    char expression[MAX_LINE];
    size_t length = 0;
    for (int i = 1; i < argc; i++) {
        length += (size_t) ksnprintf(expression + length, sizeof expression - length,
                                     i > 1 ? " %s" : "%s", argv[i]);
        if (length >= sizeof expression) {
            kprintf("error: expression too long\n");
            return 1;
        }
    }

    int64_t result;
    const char* error;
    if (calc_evaluate(expression, &result, &error) != 0) {
        kprintf("error: %s\n", error);
        return 1;
    }
    kprintf("= %ld\n", result);
    return 0;
}

static bool parse_color(const char* text, uint8_t* color) {
    for (uint8_t i = 0; i < 16; i++) {
        if (strcmp(text, color_names[i]) == 0) {
            *color = i;
            return true;
        }
    }
    uint64_t number;
    if (parse_uint(text, 15, &number)) {
        *color = (uint8_t) number;
        return true;
    }
    return false;
}

static int cmd_color(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        kprintf("usage: color <foreground> [background]\ncolors:");
        for (int i = 0; i < 16; i++) {
            kprintf(" %s", color_names[i]);
        }
        kprintf(" (or 0-15)\n");
        return argc == 1 ? 0 : 1;
    }

    uint8_t foreground;
    uint8_t background = print_get_color() >> 4; /* keep the current background */
    if (!parse_color(argv[1], &foreground)) {
        kprintf("unknown color: %s (type `color` to list them)\n", argv[1]);
        return 1;
    }
    if (argc == 3 && !parse_color(argv[2], &background)) {
        kprintf("unknown color: %s (type `color` to list them)\n", argv[2]);
        return 1;
    }

    print_set_color(foreground, background);
    return 0;
}

static int cmd_uptime(int argc, char** argv) {
    (void) argc;
    (void) argv;
    uint64_t seconds = timer_uptime_ms() / 1000;
    kprintf("up %02lu:%02lu:%02lu (%lu ticks)\n",
            seconds / 3600, seconds / 60 % 60, seconds % 60, timer_ticks());
    return 0;
}

static int cmd_sleep(int argc, char** argv) {
    uint64_t seconds;
    if (argc != 2 || !parse_uint(argv[1], 3600, &seconds)) {
        kprintf("usage: sleep <seconds> (0-3600)\n");
        return 1;
    }
    sleep_ms(seconds * 1000);
    return 0;
}

static int cmd_date(int argc, char** argv) {
    (void) argc;
    (void) argv;
    struct rtc_time now;
    rtc_read(&now);
    kprintf("%04u-%02u-%02u %02u:%02u:%02u UTC\n",
            now.year, now.month, now.day, now.hour, now.minute, now.second);
    return 0;
}

static int cmd_reboot(int argc, char** argv) {
    (void) argc;
    (void) argv;
    kprintf("Rebooting...\n");
    reboot();
}

static int cmd_shutdown(int argc, char** argv) {
    (void) argc;
    (void) argv;
    kprintf("Powering off...\n");
    shutdown();
}

int shell_split(char* line, char** argv, int max) {
    int argc = 0;
    char* p = line;

    for (;;) {
        while (*p == ' ' || *p == '\t') {
            *p++ = '\0';
        }
        if (*p == '\0') {
            return argc;
        }
        if (argc == max) {
            return -1;
        }
        argv[argc++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
    }
}

int shell_execute(char* line) {
    char* argv[MAX_ARGS];
    int argc = shell_split(line, argv, MAX_ARGS);

    if (argc < 0) {
        kprintf("error: too many arguments (at most %d words)\n", MAX_ARGS);
        return -1;
    }
    if (argc == 0) {
        return 0;
    }

    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            return commands[i].run(argc, argv) == 0 ? 0 : 1;
        }
    }

    kprintf("unknown command: %s (type `help` to see the commands)\n", argv[0]);
    return -1;
}

void shell_run(void) {
    char line[MAX_LINE];
    for (;;) {
        kprintf("> ");
        if (readline(line, sizeof line) >= 0) {
            shell_execute(line);
        }
    }
}
