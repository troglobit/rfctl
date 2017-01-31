/* RF bitbang control tool for NEXA and other RF remote receivers
 *
 * Copyright (C) 2010 Tord Andersson <tord.andersson@endian.se>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, visit the Free Software Foundation
 * website at http://www.gnu.org/licenses/gpl-2.0.html or write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>

#include "common.h"
#include "protocol.h"

/* Local variables */
bool verbose = false;		/* -v option */
bool running = true;

/* Program name, derived from argv[0] */
static char *prognm = NULL;


static int usage(int code)
{
	printf("\n"
	       "Usage: %s [rwVvh] [-d DEV] [-i IFACE] [-p PROTO] [-s NO]\n"
	       "                        [-g GROUP] [-c CHAN] [-l LEVEL]\n"
	       "\n"
	       " -d, --device=DEV       Device to use, defaults to %s\n"
	       " -i, --interface=IFACE  RFBB, CUL or TELLSTICK. Defaults to RFBB (RF Bitbanger)\n"
	       " -p, --protocol=PROTO   NEXA, NEXA_L, SARTANO, WAVEMAN, IKEA or RAW\n"
	       " -r, --read             Raw space/pulse read, only on supported interfaces\n"
	       " -w, --write            Send command (default)\n"
	       " -g, --group=GROUP      The group/house/system number or letter\n"
	       " -c, --channel=CHAN     The channel/unit number\n"
	       " -s, --serialnumber=NO  The serial/unique number used by NEXA L (self-learning)\n"
	       " -l, --level=LEVEL      Dimmer level, 0..100.  All values above 0 will switch\n"
	       "                        on non-dimmable devices\n"
	       " -V, --verbose          Enable verbose messages during operation\n"
	       " -v, --version          Show program version and exit\n"
	       " -h, --help             Show summary of command line options and exit\n"
	       "\n"
	       "NEXA, WAVEMAN protocol arguments:\n"
	       "  group   : A..P\n"
	       "  channel : 1..16\n"
	       "  off/on  : 0..1\n"
	       "\n"
	       "SARTANO protocol arguments:\n"
	       "  channel : 0000000000..1111111111\n"
	       "  off/on  : 0..1\n"
	       "\n"
	       "IKEA protocol arguments:\n"
	       "  group   : 1..16   (system)\n"
	       "  channel : 1..10   (device)\n"
	       "  level   : 0..100\n"
	       "  dimstyle: 0..1    (N/A)\n"
	       "\n"
	       "Example:\n"
	       "  %s -p NEXA -g D -c 1 -l 1      (NEXA D1 on)\n"
	       "\n"
	       "Bug report address: https://github.com/troglobit/pibang/issues\n"
	       "\n", prognm, DEFAULT_DEVICE, prognm);

	return code;
}

static void sigterm_cb(int signo)
{
	/*
	 * This will force the exit handler to run
	 */
	PRINT("Signal handler for %d signal\n", signo);
	running = false;
}

static char *progname(char *arg0)
{
       char *nm;

       nm = strrchr(arg0, '/');
       if (nm)
	       nm++;
       else
	       nm = arg0;

       return nm;
}

int main(int argc, char **argv)
{
	struct termios tio;
	int fd = -1;
	rf_interface_t iface = IFC_RFBB;
	char default_dev[255] = DEFAULT_DEVICE;
	char *device = default_dev;	/* -d option */
	rf_mode_t mode = MODE_WRITE;	/* read/write */
	char *proto = NULL;
	rf_protocol_t protocol = PROT_NEXA;	/* protocol */
	const char *group = NULL;	/* house/group/system opåtion */
	const char *channel = NULL;	/* -c (channel/unit) option */
	const char *level = NULL;	/* level 0 - 100 % or on/off */
	int32_t tx_bitstream[RF_MAX_TX_BITS];
	int32_t rx_bitstream[RF_MAX_RX_BITS];
	int32_t rx_val = 0;
	int rx_len = 0;
	int tx_len = 0;
	int repeat = 0;
	int i, c;
	char cmd[RF_MAX_TX_BITS * 6]; /* hex/ASCII representation is longer than bitstream */
	int cmd_len = 0;
	const struct option opt[] = {
		{ "device",       required_argument, NULL, 'd' },
		{ "interface",    required_argument, NULL, 'i' },
		{ "protocol",     required_argument, NULL, 'p' },
		{ "read",         no_argument,       NULL, 'r' },
		{ "write",        no_argument,       NULL, 'w' },
		{ "group",        required_argument, NULL, 'g' },
		{ "channel",      required_argument, NULL, 'c' },
		{ "serialnumber", required_argument, NULL, 's' },
		{ "level",        required_argument, NULL, 'l' },
		{ "version",      no_argument,       NULL, 'v' },
		{ "verbose",      no_argument,       NULL, 'V' },
		{ "help",         no_argument,       NULL, 'h' },
		{ NULL,           no_argument,       NULL,  0  }
	};

	prognm = progname(argv[0]);
	while ((c = getopt_long(argc, argv, "d:i:p:rwg:c:l:vVh?", opt, &i)) != EOF) {
		switch (c) {
		case 'd':
			if (optarg) {
				device = optarg;
			} else {
				fprintf(stderr, "Error. Missing device path.\n");
				return usage(1);
			}
			break;

		case 'i':
			if (optarg) {
				if (strcmp("RFBB", optarg) == 0) {
					iface = IFC_RFBB;
				} else if (strcmp("CUL", optarg) == 0) {
					iface = IFC_CUL;
				} else if (strcmp("TELLSTICK", optarg) == 0) {
					iface = IFC_TELLSTICK;
				} else {
					iface = IFC_UNKNOWN;
					fprintf(stderr, "Error. Unknown interface type: %s\n", optarg);
					return usage(1);
				}
			} else {
				fprintf(stderr, "Error. Missing interface type.\n");
				return usage(1);
			}
			break;

		case 'r':
			mode = MODE_READ;
			break;

		case 'w':
			mode = MODE_WRITE;
			break;

		case 'p':
			if (optarg) {
				proto = optarg;
				if (strcmp("NEXA", proto) == 0) {
					protocol = PROT_NEXA;
				} else if (strcmp("PROOVE", proto) == 0) {
					protocol = PROT_NEXA;
				} else if (strcmp("WAVEMAN", proto) == 0) {
					protocol = PROT_WAVEMAN;
				} else if (strcmp("SARTANO", proto) == 0) {
					protocol = PROT_SARTANO;
				} else if (strcmp("IMPULS", proto) == 0) {
					protocol = PROT_IMPULS;
				} else if (strcmp("NEXA_L", proto) == 0) {
					protocol = PROT_NEXA_L;
				} else {
					fprintf(stderr, "Error. Unknown protocol: %s\n", proto);
					return usage(1);
				}
			} else {
				fprintf(stderr, "Error. Missing protocol\n");
				return usage(1);
			}
			break;

		case 'g':
			if (optarg) {
				group = optarg;
			} else {
				fprintf(stderr, "Error. Missing group/house/system ID\n");
				return usage(1);
			}
			break;

		case 'c':
			if (optarg) {
				channel = optarg;
			} else {
				fprintf(stderr, "Error. Missing channel number\n");
				return usage(1);
			}
			break;

		case 'l':
			if (optarg) {
				level = optarg;
			} else {
				fprintf(stderr, "Error. Missing level\n");
				return usage(1);
			}
			break;

		case 'v':
			puts(VERSION);
			return 0;

		case 'V':
			verbose = true;
			break;

		case 0:	/* Long option without a short arg */
			if (opt[i].flag != 0)
				break;

			printf("option %s", opt[i].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");
			break;

		case 'h':	/* Fall through by design */
		case '?':
			return usage(0);

		default:
			return usage(1);
		}
	}

	if (!group || !channel || !level)
		return usage(1);

	/* Build generic transmit bitstream for the selected protocol */
	if (mode == MODE_WRITE) {
		switch (protocol) {
		case PROT_NEXA:
			PRINT("NEXA protocol selected\n");
			tx_len = nexa_bitstream(group, channel, level, tx_bitstream, &repeat);
			if (tx_len == 0)
				return usage(1);
			break;

		case PROT_WAVEMAN:
			PRINT("WAVEMAN protocol selected\n");
			tx_len = waveman_bitstream(group, channel, level, tx_bitstream, &repeat);
			if (tx_len == 0)
				return usage(1);
			break;

		case PROT_SARTANO:
			PRINT("SARTANO protocol selected\n");
			tx_len = sartano_bitstream(channel, level, tx_bitstream, &repeat);
			if (tx_len == 0)
				return usage(1);
			break;

		case PROT_IMPULS:
			PRINT("IMPULS protocol selected\n");
			tx_len = impulse_bitstream(channel, level, tx_bitstream, &repeat);
			if (tx_len == 0)
				return usage(1);
			break;

		case PROT_IKEA:
			PRINT("IKEA protocol selected\n");
			tx_len = ikea_bitstream(group, channel, level, "1", tx_bitstream, &repeat);
			if (tx_len == 0)
				return usage(1);
			break;

		default:
			fprintf(stderr, "Protocol: %s is currently not supported\n", proto);
			return usage(1);
		}
	}

	/* Transmit/read handling for each interface type */
	switch (iface) {
	case IFC_RFBB:
		PRINT("Selected RFBB interface (RF Bitbanger)\n");

		if (0 > (fd = open(device, O_RDWR))) {
			fprintf(stderr, "%s - Error opening %s\n", prognm, device);
			return 1;
		}

		if (mode == MODE_WRITE) {
			PRINT("Writing %d pulse_space_items, (%d bytes) to %s\n", tx_len * repeat,
			      tx_len * 4 * repeat, device);
			for (i = 0; i < repeat; i++) {
				if (write(fd, tx_bitstream, tx_len * 4) < 0) {
					perror("Error writing to RFBB device");
					break;
				}
			}
			sleep(1);
		} else if (mode == MODE_READ) {
			running = true;
			PRINT("Reading pulse_space_items\n");

			/*
			 * Set up signal handlers to act on CTRL-C events
			 */
			if (signal(SIGINT, sigterm_cb) == SIG_ERR) {
				perror("Can't register signal handler for CTRL-C et al: ");
				return -1;
			}

			while (running == true) {	/* repeat until CTRL-C */
				rx_len = read(fd, rx_bitstream, 4);
				if (rx_len == 4) {
					rx_val = (uint32_t)*&rx_bitstream[0];
					if (LIRC_IS_TIMEOUT(rx_val))
						printf("\nRX Timeout");
					else if (LIRC_IS_PULSE(rx_val))
						printf("\n1 - %05d us", LIRC_VALUE(rx_val));
					else if (LIRC_IS_SPACE(rx_val))
						printf("\n0 - %05d us", LIRC_VALUE(rx_val));
				} else {
					if (rx_len == 0) {
						usleep(100 * 1000);	/* 100 ms */
						printf(".");
						fflush(stdout);
					} else {
						printf("Read %d bytes\n", rx_len);
						fflush(stdout);
					}
				}
			}
		}
		printf("\n");
		close(fd);
		break;

	case IFC_TELLSTICK:
		PRINT("Selected Tellstick interface\n");
#if 0
		if (0 > (fd = open(*(argv + 1), O_RDWR))) {
			fprintf(stderr, "%s - Error opening %s\n", prognm, *(argv + 1));
			return 1;
		}

		/* adjust serial port parameters */
		bzero(&tio, sizeof(tio));	/* clear struct for new port settings */
		tio.c_cflag = B4800 | CS8 | CLOCAL | CREAD;	/* CREAD not used yet */
		tio.c_iflag = IGNPAR;
		tio.c_oflag = 0;
		tio.c_ispeed = 4800;
		tio.c_ospeed = 4800;
		tcflush(fd, TCIFLUSH);
		tcsetattr(fd, TCSANOW, &tio);
		if (write(fd, txStr, strlen(txStr)) < 0)
			perror("Error writing to Tellstick device");

		sleep(1);	/* one second sleep to avoid device 'choking' */
		close(fd);
#endif
		break;

	case IFC_CUL:
		PRINT("Selected CUL433 interface\n");

		if (0 > (fd = open(device, O_RDWR))) {
			fprintf(stderr, "%s - Error opening %s\n", prognm, device);
			return 1;
		}

		/* adjust serial port parameters */
		bzero(&tio, sizeof(tio));	/* clear struct for new port settings */
		tio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;	/* CREAD not used yet */
		tio.c_iflag = IGNPAR;
		tio.c_oflag = 0;
		tio.c_ispeed = 115200;
		tio.c_ospeed = 115200;
		tcflush(fd, TCIFLUSH);
		tcsetattr(fd, TCSANOW, &tio);

		printf("Mode : %d\n", mode);

		if (mode == MODE_WRITE) {
			/* CUL433 nethome format */
			cmd_len = bitstream2cul443(tx_bitstream, tx_len, repeat, cmd);

			printf("CUL cmd: %s\n", cmd);

			if (write(fd, cmd, cmd_len) < 0)
				perror("Error writing to CUL device");
			sleep(1);
		} else if (mode == MODE_READ) {
			running = true;
			PRINT("Reading pulse_space_items\n");

			/*
			 * Set up signal handlers to act on CTRL-C events
			 */
			if (signal(SIGINT, sigterm_cb) == SIG_ERR) {
				perror("Can't register signal handler for CTRL-C et al: ");
				return -1;
			}

			/* start rx */
			if (write(fd, "\r\nX01\r\n", 7) < 0) {
				perror("Error issuing RX cmd to CUL device");
				running = false;
			}

			while (running) {	/* repeat until CTRL-C */
				rx_len = read(fd, rx_bitstream, 5);
				if (rx_len == 5) {
					rx_val = (uint32_t)*&rx_bitstream[0];
					printf("\n%08X: ", rx_val);
					if (rx_val & 0x8000)
						printf("1 - %05d us", rx_val & 0x7FFF);
					else
						printf("0 - %05d us", rx_val & 0x7FFF);

					if ((rx_val & 0x7FFF) == 0x7FFF)
						printf(" - Timeout");
				} else {
					if (rx_len == 0) {
						usleep(100 * 1000);	/* 100 ms */
						printf(".");
						fflush(stdout);
					} else {
						printf("Read %d bytes\n", rx_len);
						fflush(stdout);
					}
				}
			}
		}
		close(fd);
		break;


	default:
		fprintf(stderr, "%s - Illegal interface type (%d)\n", prognm, iface);
		break;
	}

	return 0;
}
