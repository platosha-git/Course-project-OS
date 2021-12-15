#include <stddef.h>

#define DRIVER_VERSION "v1.0"
#define DRIVER_AUTHOR "Olga"
#define DRIVER_DESC "USB Mouse Click Sound Driver"

/* Button codes */
#define LEFT_BTN_BIT 0x01
#define RGHT_BTN_BIT 0X02
#define MIDL_BTN_BIT 0x04

/* Thread delay range */
#define DELAY_LO 1000
#define DELAY_HI 2000

/* aplay commands */
char *press_argv[] = {"/bin/aplay", "/home/platosha/Desktop/BMSTU/7sem/OS/audio/press.wav", NULL };
char *release_argv[] = {"/bin/aplay", "/home/platosha/Desktop/BMSTU/7sem/OS/audio/release.wav", NULL };

/* aplay environment */
char *envp[] = {"HOME=/", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
