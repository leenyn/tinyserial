#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

#include <termios.h>

static bool baudrate_is_valid(int baud)
{
    switch (baud) {
    case 50: case 75: case 110: case 134: case 150: case 200: case 300: case 600: case 1200:
    case 1800: case 2400: case 4800: case 9600: case 19200: case 38400:
    case 57600: case 115200: case 230400: case 460800: case 500000: case 576000: case 921600:
    case 1000000: case 1152000: case 2000000: case 2500000: case 3000000: case 4000000:
        return true;
    }
    return false;
}
static int baudrate_get_mask(int baud)
{
    switch (baud) {
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 500000: return B500000;
        case 576000: return B576000;
        case 921600: return B921600;
        case 1000000: return B1000000;
        case 1152000: return B1152000;
        case 2000000: return B2000000;
        case 2500000: return B2500000;
        case 3000000: return B3000000;
        case 4000000: return B4000000;
    }
    return 0;
}

static int show_serial(int fdterm, const char *buf, size_t buf_len)
{
    ssize_t written;
    size_t i, data_len;
    char c, data[32];
    for (i = 0; i < buf_len; i++) {
        c = buf[i];
        if (isprint(c) || c == '\r' || c == '\n' || c == '\t') {
            data[0] = c;
            data_len = 1;
        } else {
            snprintf(data, sizeof(data)-1, "\033[1;33m%02X\033[0m", c);
            data[sizeof(data)-1] = '\0';
            data_len = strlen(data);
        }
        written = write(fdterm, data, data_len);
        if ((size_t)written != data_len)
            return -1;
    }
    return 0;
}

static int show_serial_debug(int fdterm, const char *buf, size_t buf_len)
{
    ssize_t written;
    size_t i, data_len;
    char c, data[32];
    for (i = 0; i < buf_len; i++) {
        c = buf[i];
        if (isprint(c)) {
            snprintf(data, sizeof(data)-1, "\033[1;32m%c\033[0m", c);
        } else {
            snprintf(data, sizeof(data)-1, "\033[1;32m%02X\033[0m", c);
        }
        data[sizeof(data)-1] = '\0';
        data_len = strlen(data);
        written = write(fdterm, data, data_len);
        if ((size_t)written != data_len)
            return -1;
    }
    return 0;
}

static int show_term_debug(int fdterm, const char *buf, size_t buf_len)
{
    ssize_t written;
    size_t i, data_len;
    char c, data[32];
    for (i = 0; i < buf_len; i++) {
        c = buf[i];
        if (isprint(c)) {
            snprintf(data, sizeof(data)-1, "\033[1;34m%c\033[0m", c);
        } else {
            snprintf(data, sizeof(data)-1, "\033[1;34m%02X\033[0m", c);
        }
        data[sizeof(data)-1] = '\0';
        data_len = strlen(data);
        written = write(fdterm, data, data_len);
        if ((size_t)written != data_len)
            return -1;
    }
    return 0;
}

static int serial_write(int fdserial, const char *buf, size_t buf_len, bool cr, bool lf)
{
    ssize_t written;
    size_t i, data_len;
    char c, data[32];
    for (i = 0; i < buf_len; i++) {
        c = buf[i];
        #if 0
        if (c == '\n') {
            if (cr) {
                c = '\n';
            } else if (lf) {
                c = '\r';
            } else {
                strcpy(data, "\r\n");
                data_len = 2;
                written = write(fdserial, data, data_len);
                if ((size_t)written != data_len)
                    return -1;
                continue;
            }
        }
        #endif
        written = write(fdserial, &c, 1);
        if (written != 1)
            return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int err;
    int ch;
    bool want_help = false;
    int baud = 115200, size = '8', parity = 'N', stop = '1';
    bool rtscts = false, cr = false, lf = false, debug = false;
    const char *device_file;
    long tmp;
    char *end;

    int fdserial = -1, fdterm = 0, fd_max;
    struct termios ios, ios_term;
    bool term_restore = false;
    fd_set rfds;

    char buf[256];
    ssize_t len_read;

    struct option longopts[] = {
        {"help", no_argument, 0, 'h'},
        {"baud", required_argument, 0, 1},
        {"size", required_argument, 0, 2},
        {"parity", required_argument, 0, 3},
        {"stop", required_argument, 0, 4},
        {"rtscts", no_argument, 0, 5},
        {"cr", no_argument, 0, 6},
        {"lf", no_argument, 0, 7},
        {NULL, 0, 0,0},
    };

    
    // test
    #if 0
    for (ch = 0; ch < 0x20; ch++) {
        sprintf(buf, "%02X  ", ch);
        buf[4] = ch;
        buf[5] = '\n';
        write(fdterm, buf, 6);
    }
    return 0;
    #endif

    while((ch = getopt_long(argc, argv, "hd", longopts, NULL))!= -1) {
        switch (ch) {
        case 1:
            tmp = strtol(optarg, &end, 0);
            if (end == optarg || *end != '\0')
                want_help = true;
            else
                baud = (int)tmp;
            if (!baudrate_is_valid(baud))
                want_help = true;
            break;
        case 2:
            if (strlen(optarg) != 1 || (optarg[0] != '5' && optarg[0] != '6' && optarg[0] != '7' && optarg[0] != '8'))
                want_help = true;
            else
                size = optarg[0];
            break;
        case 3:
            if (strlen(optarg) != 1 || (optarg[0] != 'N' && optarg[0] != 'E' && optarg[0] != 'O'))
                want_help = true;
            else
                parity = optarg[0];
            break;
        case 4:
            if (strlen(optarg) != 1 || (optarg[0] != '1' && optarg[0] != '2'))
                want_help = true;
            else
                stop = optarg[0];
            break;
        case 5:
            rtscts = true;
            break;
        case 6:
            cr = true;
            lf = false;
            break;
        case 7:
            cr = false;
            lf = true;
            break;
        case 'd':
            debug = true;
            break;
        default:
            want_help = true;
            break;
        }
    }
    if (want_help) {
        fprintf(stderr, "Usage: tinyserial [OPTION] port\n");
        fprintf(stderr, "\ntinyserial - A simple terminal program for serial port\n");
        fprintf(stderr, "\nOptions:\n");
        fprintf(stderr, "  -h, --help             show this help message and exit\n");
        fprintf(stderr, "  --baud=BAUDRATE        set baud rate, default 115200\n");
        fprintf(stderr, "  --size=SIZE            set size (possible values: 5, 6, 7, 8, default 8)\n");
        fprintf(stderr, "  --parity=PARITY        set parity, one of [N, E, O], default=N\n");
        fprintf(stderr, "  --stop=STOP            set stop bit (possible values: 1, 2, default 1)\n");
        fprintf(stderr, "  --rtscts               enable RTS/CTS flow control (default off)\n");
        fprintf(stderr, "  --cr                   do not send CR+LF, send CR only\n");
        fprintf(stderr, "  --lf                   do not send CR+LF, send LF only\n");
        return 2;
    }
    if (optind != argc-1) {
        fprintf(stderr, "missing serial device\n");
        return 2;
    }
    device_file = argv[optind];



    fdserial = open(device_file, O_RDWR);
    if (fdserial < 0) {
        fprintf(stderr, "failed to open device: %s : %s\n", device_file, strerror(errno));
        return 3;
    }
    err = tcgetattr(fdserial, &ios);
    if (err < 0) {
        fprintf(stderr, "failed to get attr of serial: %s\n", strerror(errno));
        err = 3;
        goto clean;
    }
    tcflush(fdserial, TCIOFLUSH);
    ios.c_iflag = IGNBRK | IGNPAR;
    if (parity != 'N')
        ios.c_iflag |= INPCK;
    ios.c_oflag = 0;
    ios.c_cflag = baudrate_get_mask(baud);
    switch (size) {
    case '5': ios.c_cflag |= CS5; break;
    case '6': ios.c_cflag |= CS6; break;
    case '7': ios.c_cflag |= CS7; break;
    case '8': ios.c_cflag |= CS8; break;
    }
    switch (stop) {
    case '2': ios.c_cflag |= CSTOPB; break;
    }
    ios.c_cflag |= CREAD | CLOCAL;
    if (parity != 'N')
        ios.c_cflag |= PARENB;
    if (parity == 'O')
        ios.c_cflag |= PARODD;
    if (rtscts)
        ios.c_cflag |= CRTSCTS;
    ios.c_lflag = 0;
    ios.c_cc[VMIN] = 1;
    ios.c_cc[VTIME] = 0;
    err = tcsetattr(fdserial, TCSANOW, &ios);
    if (err < 0) {
        fprintf(stderr, "failed to set attr of serial: %s\n", strerror(errno));
        err = 3;
        goto clean;
    }



    if (!isatty(fdterm)) {
        fprintf(stderr, "not a terminal\n");
        err = 4;
        goto clean;
    }
    err = tcgetattr(fdterm, &ios);
    if (err < 0) {
        fprintf(stderr, "failed to get attr of terminal: %s\n", strerror(errno));
        err = 4;
        goto clean;
    }
    ios_term = ios;
    ios.c_iflag &= ~(INLCR | ICRNL);
    ios.c_oflag &= ~(ONLCR | OCRNL);
    ios.c_lflag &= ~(ICANON | ECHO);
    ios.c_cc[VINTR] = 0;
    ios.c_cc[VQUIT] = 0;
    ios.c_cc[VSUSP] = 0;
    err = tcsetattr(fdterm, TCSANOW, &ios);
    if (err < 0) {
        fprintf(stderr, "failed to set attr of terminal: %s\n", strerror(errno));
        err = 4;
        goto clean;
    }
    term_restore = true;

    // test
    #if 0
    for (ch = 0; ch < 0x20; ch++) {
        sprintf(buf, "%02X  ", ch);
        buf[4] = ch;
        buf[5] = '\n';
        write(fdterm, buf, 6);
    }
    goto clean;
    #endif



    while (1) {
        FD_ZERO(&rfds);
        FD_SET(fdterm, &rfds);
        FD_SET(fdserial, &rfds);
        fd_max = fdserial > fdterm ? fdserial : fdterm;
        err = select(fd_max + 1, &rfds, NULL, NULL, NULL);
        if (err <= 0) {
            err = 5;
            break;
        }
        if (FD_ISSET(fdserial, &rfds)) {
            len_read = read(fdserial, buf, sizeof(buf) - 1);
            if (len_read < 0) {
                if (errno == EINTR)
                    continue;
                err = 10;
                break;
            } else if (len_read == 0) {
                err = 11;
                break;
            }
            buf[len_read] = 0;
            if (debug)
                show_serial_debug(fdterm, buf, len_read);
            show_serial(fdterm, buf, len_read);
        } else if (FD_ISSET(fdterm, &rfds)) {
            len_read = read(fdterm, buf, sizeof(buf) - 1);
            if (len_read < 0) {
                if (errno == EINTR)
                    continue;
                err = 20;
                break;
            } else if (len_read == 0) {
                err = 21;
                break;
            }
            buf[len_read] = 0;
            err = 0;
            if (buf[0] == 003) { // intr ^C
                break;
            } else if (buf[0] == 034) { // quit
                break;
            } else if (buf[0] == 025) { // kill ^U
                break;
            } else if (buf[0] == 004) { // eof ^D
                break;
            } else if (buf[0] == 032) { // susp ^Z
                break;
            }
            if (debug)
                show_term_debug(fdterm, buf, len_read);
            serial_write(fdserial, buf, len_read, cr, lf);
        }
    }
clean:
    if (fdserial >= 0)
        close(fdserial);
    if (term_restore)
        tcsetattr(fdterm, TCSANOW, &ios_term);
	return err;
}
