#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/ioctl.h>


struct define_info_t {
  const char     *name;
  const uint32_t value;
  const char    *comment;
};

static const struct define_info_t _def_iflags[] = {
  {"IGNBRK",  IGNBRK,  NULL},
  {"BRKINT",  BRKINT,  NULL},
  {"IGNPAR",  IGNPAR,  NULL},
  {"PARMRK",  PARMRK,  NULL},
  {"INPCK",   INPCK,   NULL},
  {"ISTRIP",  ISTRIP,  NULL},
  {"INLCR",   INLCR,   NULL},
  {"IGNCR",   IGNCR,   NULL},
  {"ICRNL",   ICRNL,   NULL},
  {"IUCLC",   IUCLC,   NULL},
  {"IXON",    IXON,    NULL},
  {"IXANY",   IXANY,   NULL},
  {"IXOFF",   IXOFF,   NULL},
  {"IMAXBEL", IMAXBEL, NULL},
  {"IUTF8",   IUTF8,   NULL},
  {NULL, 0, NULL},
};

static const struct define_info_t _def_cflags[] = {
  {"CSTOPB", CSTOPB, NULL},
  {"CREAD",  CREAD,  NULL},
  {"PARENB", PARENB, NULL},
  {"PARODD", PARODD, NULL},
  {"HUPCL",  HUPCL,  NULL},
  {"CLOCAL", CLOCAL, NULL},
  {NULL, 0, NULL},
};

static const struct define_info_t _def_oflags[] = {
  {"OPOST",  OPOST,  NULL},
  {"OLCUC",  OLCUC,  NULL},
  {"ONLCR",  ONLCR,  NULL},
  {"OCRNL",  OCRNL,  NULL},
  {"ONOCR",  ONOCR,  NULL},
  {"ONLRET", ONLRET, NULL},
  {"OFILL",  OFILL,  NULL},
  {"OFDEL",  OFDEL,  NULL},
  {NULL, 0, NULL},
};

static const struct define_info_t _def_lflags[] = {
  {"ISIG", ISIG, NULL},
  {"ICANON", ICANON, NULL},
#if defined __USE_MISC || (defined __USE_XOPEN && !defined __USE_XOPEN2K)
  {"XCASE", XCASE, NULL},
#endif
  {"ECHO", ECHO, NULL},
  {"ECHOE", ECHOE, NULL},
  {"ECHOK", ECHOK, NULL},
  {"ECHONL", ECHONL, NULL},
  {"NOFLSH", NOFLSH, NULL},
  {"TOSTOP", TOSTOP, NULL},
#ifdef __USE_MISC
  {"ECHOCTL", ECHOCTL, NULL},
  {"ECHOPRT", ECHOPRT, NULL},
  {"ECHOKE", ECHOKE, NULL},
  {"FLUSHO", FLUSHO, NULL},
  {"PENDIN", PENDIN, NULL},
#endif
  {"IEXTEN", IEXTEN, NULL},
#ifdef __USE_MISC
  {"EXTPROC", EXTPROC, NULL},
#endif
  {NULL, 0, NULL},
};


static uint32_t _get_speed(speed_t speed);
static void _display_define_infos(const struct define_info_t *p_defines, uint32_t flags);


/** 指定したパスの tty 設定取得
 * @param path デバイスパス
 * @return struect termios ポインタ. free で解放する必要がある.
 */
struct termios *tty_get_info(const char *path) {
  //int fd = open(path, O_RDONLY | O_ASYNC);
  int fd = open(path, O_RDONLY | O_NONBLOCK);

  if(fd <= 0) {
    goto pt_open_error;
  }

  struct termios termios;
  memset((void *)&termios, '\0', sizeof(struct termios));

  int status = ioctl(fd, TCGETS, (void *)&termios);
  if(status != 0) {
    goto pt_ioctl_error;
  }

  close(fd);


  struct termios *p_ret = (struct termios *)calloc(1, sizeof(struct termios));
  if(p_ret == NULL) {
    return NULL;
  }

  memcpy((void *)p_ret, (const void *)&termios, sizeof(struct termios));
  return p_ret;

pt_ioctl_error:
  close(fd);
pt_open_error:
  return NULL;
}


#define FLAG_STATUS(filter, flag) \
  (((filter & flag) == filter) ? "\033[32;1mEnable\033[0m" : "\033[31;1mDisable\033[0m")


/** struct termios の情報を表示
 * @param p_term tty_get_info(const char *) で取得した設定のポインタ
 * @return void
 */
void tty_print_info(const struct termios *p_term) {
  fprintf(stdout, "  ispeed: %u, ospeed: %u\n\n",
      _get_speed(cfgetispeed(p_term)),
      _get_speed(cfgetospeed(p_term)));


  fprintf(stdout, "  Input modes\n");
  _display_define_infos(_def_iflags, p_term->c_iflag);
  fprintf(stdout, "\n");


  fprintf(stdout, "  Output modes\n");
  _display_define_infos(_def_oflags, p_term->c_oflag);
  fprintf(stdout, "\n");


  fprintf(stdout, "  Control modes\n");
  fprintf(stdout, "    Character size: ");
  switch(p_term->c_cflag & CSIZE) {
    case CS5:
      fprintf(stdout, "CS5\n");
      break;

    case CS6:
      fprintf(stdout, "CS6\n");
      break;

    case CS7:
      fprintf(stdout, "CS7\n");
      break;

    case CS8:
      fprintf(stdout, "CS8\n");
      break;

    default:
      fprintf(stdout, "undefined\n");
      break;
  }


  _display_define_infos(_def_cflags, p_term->c_cflag);
  fprintf(stdout, "\n");


  fprintf(stdout, "  Local modes\n");
  _display_define_infos(_def_lflags, p_term->c_lflag);
  fprintf(stdout, "\n");

  fprintf(stdout, "  c_cc[MIN]:   %d\n", p_term->c_cc[VMIN]);
  fprintf(stdout, "  c_cc[VTIME]: %d\n", p_term->c_cc[VTIME]);
  fprintf(stdout, "  c_cc[VEOF]:  %c(0x%02x)\n", p_term->c_cc[VEOF], p_term->c_cc[VEOF]);
  fprintf(stdout, "  c_cc[VQUIT]: %c(0x%02x)\n", p_term->c_cc[VQUIT], p_term->c_cc[VQUIT]);
}


static uint32_t _get_speed(speed_t speed) {
  switch(speed) {
    case B0: return 0;
    case B50: return 50;
    case B75: return 75;
    case B110: return 110;
    case B134: return 134;
    case B150: return 150;
    case B200: return 200;
    case B300: return 300;
    case B600: return 600;
    case B1200: return 1200;
    case B1800: return 1800;
    case B2400: return 2400;
    case B4800: return 4800;
    case B9600: return 9600;
    case B19200: return 19200;
    case B38400: return 38400;
    case B57600: return 57600;
    case B115200: return 115200;
    case B230400: return 230400;
    case B460800: return 460800;
    case B500000: return 500000;
    case B576000: return 576000;
    case B921600: return 921600;
    case B1000000: return 1000000;
    case B1152000: return 1152000;
    case B1500000: return 1500000;
    case B2000000: return 2000000;
    case B2500000: return 2500000;
    case B3000000: return 3000000;
    case B3500000: return 3500000;
    case B4000000: return 4000000;
  }

  return 0;
}


static void _display_define_infos(const struct define_info_t *p_defines, uint32_t flags) {
  while(p_defines->name != NULL) {
    fprintf(stdout, "    %-8s %s\n",
        p_defines->name,
        FLAG_STATUS(p_defines->value, flags));
    ++p_defines;
  }
}
