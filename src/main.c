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

#define RF_MAX_TX_BITS 4000	/* Max TX pulse/space elements in one message */
#define RF_MAX_RX_BITS 4000	/* Max read RX pulse/space elements at one go */

#define DEFAULT_DEVICE "/dev/rfbb"

typedef enum { MODE_UNKNOWN, MODE_READ, MODE_WRITE } rfMode_t;
typedef enum { IFC_UNKNOWN, IFC_RFBB, IFC_CUL, IFC_TELLSTICK } rfInterface_t;
typedef enum { PROT_UNKNOWN, PROT_RAW, PROT_NEXA, PROT_PROOVE, PROT_NEXA_L,
	PROT_SARTANO, PROT_WAVEMAN, PROT_IKEA, PROT_ESIC, PROT_IMPULS
} rfProtocol_t;

static void printUsage(void);
static void printVersion(void);
static void signalTerminate(int signo);

/* Local variables */
bool verbose = false;		/* -v option */
bool stopNow = false;

/* Program name, derived from argv[0] */
static char *prognm = NULL;

/* Command line option handling */
static const char *optString = "d:i:p:rwg:c:l:vh?";

static const struct option longOpts[] = {
	{"device", required_argument, NULL, 'd'},
	{"interface", required_argument, NULL, 'i'},
	{"protocol", required_argument, NULL, 'p'},
	{"read", no_argument, NULL, 'r'},
	{"write", no_argument, NULL, 'w'},
	{"group", required_argument, NULL, 'g'},
	{"channel", required_argument, NULL, 'c'},
	{"serialnumber", required_argument, NULL, 's'},
	{"level", required_argument, NULL, 'l'},
	{"verbose", no_argument, NULL, 'v'},
	{"help", no_argument, NULL, 'h'},
	{NULL, no_argument, NULL, 0}
};

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
	int opt = 0;
	int longIndex = 0;
	rfInterface_t rfInterface = IFC_RFBB;
	char defaultDevice[255] = DEFAULT_DEVICE;
	char *device = defaultDevice;	/* -d option */
	rfMode_t mode = MODE_WRITE;	/* read/write */
	char *rfProtocolStr = NULL;
	rfProtocol_t rfProtocol = PROT_NEXA;	/* protocol */
	const char *groupStr = NULL;	/* house/group/system opåtion */
	const char *channelStr = NULL;	/* -c (channel/unit) option */
	const char *levelStr = NULL;	/* level 0 - 100 % or on/off */
	int32_t txBitstream[RF_MAX_TX_BITS];
	int32_t rxBitstream[RF_MAX_RX_BITS];
	int32_t rxValue = 0;
	int rxCount = 0;
	int txItemCount = 0;
	int repeatCount = 0;
	int i;
	char asciiCmdStr[RF_MAX_TX_BITS * 6];	/* hex/ASCII repr is longer than bitstream */
	int asciiCmdLength = 0;

	prognm = progname(argv[0]);
	if (argc < 2) {
		printUsage();
		exit(1);
	}

	opt = getopt_long(argc, argv, optString, longOpts, &longIndex);

	/* parse command options */
	while (opt != -1) {

		switch (opt) {
		case 'd':
			if (optarg) {
				device = optarg;
			} else {
				fprintf(stderr, "Error. Missing device path.\n");
				printUsage();
				exit(1);
			}
			break;

		case 'i':
			if (optarg) {
				if (strcmp("RFBB", optarg) == 0) {
					rfInterface = IFC_RFBB;
				} else if (strcmp("CUL", optarg) == 0) {
					rfInterface = IFC_CUL;
				} else if (strcmp("TELLSTICK", optarg) == 0) {
					rfInterface = IFC_TELLSTICK;
				} else {
					rfInterface = IFC_UNKNOWN;
					fprintf(stderr, "Error. Unknown interface type: %s\n", optarg);
					printUsage();
					exit(1);
				}
			} else {
				fprintf(stderr, "Error. Missing interface type.\n");
				printUsage();
				exit(1);
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
				rfProtocolStr = optarg;
				if (strcmp("NEXA", rfProtocolStr) == 0) {
					rfProtocol = PROT_NEXA;
				} else if (strcmp("PROOVE", rfProtocolStr) == 0) {
					rfProtocol = PROT_NEXA;
				} else if (strcmp("WAVEMAN", rfProtocolStr) == 0) {
					rfProtocol = PROT_WAVEMAN;
				} else if (strcmp("SARTANO", rfProtocolStr) == 0) {
					rfProtocol = PROT_SARTANO;
				} else if (strcmp("IMPULS", rfProtocolStr) == 0) {
					rfProtocol = PROT_IMPULS;
				} else if (strcmp("NEXA_L", rfProtocolStr) == 0) {
					rfProtocol = PROT_NEXA_L;
				} else {
					rfProtocol = PROT_UNKNOWN;
					fprintf(stderr, "Error. Unknown protocol: %s\n", rfProtocolStr);
					printUsage();
					exit(1);
				}
			} else {
				fprintf(stderr, "Error. Missing protocol\n");
				printUsage();
				exit(1);
			}
			break;

		case 'g':
			if (optarg) {
				groupStr = optarg;
			} else {
				fprintf(stderr, "Error. Missing group/house/system ID\n");
				printUsage();
				exit(1);
			}
			break;

		case 'c':
			if (optarg) {
				channelStr = optarg;
			} else {
				fprintf(stderr, "Error. Missing channel number\n");
				printUsage();
				exit(1);
			}
			break;

		case 'l':
			if (optarg) {
				levelStr = optarg;
			} else {
				fprintf(stderr, "Error. Missing level\n");
				printUsage();
				exit(1);
			}
			break;

		case 'v':
			verbose = true;
			break;

		case 'h':	/* Fall through by design */
		case '?':
			printVersion();
			printUsage();
			exit(0);
			break;

		case 0:	/* Long option without a short arg */
			if (longOpts[longIndex].flag != 0)
				break;
			printf("option %s", longOpts[longIndex].name);
			if (optarg)
				printf(" with arg %s", optarg);
			printf("\n");

		default:
			/* You won't actually get here. */
			break;
		}

		opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
	}

	/* Build generic transmit bitstream for the selected protocol */
	if (mode == MODE_WRITE) {
		switch (rfProtocol) {
		case PROT_NEXA:
			PRINT("NEXA protocol selected\n");
			txItemCount = createNexaBitstream(groupStr, channelStr, levelStr, false, txBitstream, &repeatCount);
			if (txItemCount == 0) {
				printUsage();
				exit(1);
			}
			break;

		case PROT_WAVEMAN:
			PRINT("WAVEMAN protocol selected\n");
			txItemCount = createNexaBitstream(groupStr, channelStr, levelStr, true, txBitstream, &repeatCount);
			if (txItemCount == 0) {
				printUsage();
				exit(1);
			}
			break;

		case PROT_SARTANO:
			PRINT("SARTANO protocol selected\n");
			txItemCount = createSartanoBitstream(channelStr, levelStr, txBitstream, &repeatCount);
			if (txItemCount == 0) {
				printUsage();
				exit(1);
			}
			break;

		case PROT_IMPULS:
			PRINT("IMPULS protocol selected\n");
			txItemCount = createImpulsBitstream(channelStr, levelStr, txBitstream, &repeatCount);
			if (txItemCount == 0) {
				printUsage();
				exit(1);
			}
			break;

		case PROT_IKEA:
			PRINT("IKEA protocol selected\n");
			txItemCount = createIkeaBitstream(groupStr, channelStr, levelStr, "1", txBitstream, &repeatCount);
			if (txItemCount == 0) {
				printUsage();
				exit(1);
			}
			break;

		default:
			fprintf(stderr, "Protocol: %s is currently not supported\n", rfProtocolStr);
			printUsage();
			exit(1);

		}
	}

	/* Transmit/read handling for each interface type */
	switch (rfInterface) {
	case IFC_RFBB:
		PRINT("Selected RFBB interface (RF Bitbanger)\n");

		if (0 > (fd = open(device, O_RDWR))) {
			fprintf(stderr, "%s - Error opening %s\n", prognm, device);
			exit(1);
		}

		if (mode == MODE_WRITE) {
			PRINT("Writing %d pulse_space_items, (%d bytes) to %s\n", txItemCount * repeatCount,
			      txItemCount * 4 * repeatCount, device);
			for (i = 0; i < repeatCount; i++) {
				if (write(fd, txBitstream, txItemCount * 4) < 0) {
					perror("Error writing to RFBB device");
					break;
				}
			}
			sleep(1);
		} else if (mode == MODE_READ) {
			stopNow = false;
			PRINT("Reading pulse_space_items\n");

			/*
			 * Set up signal handlers to act on CTRL-C events
			 */
			if (signal(SIGINT, signalTerminate) == SIG_ERR) {
				perror("Can't register signal handler for CTRL-C et al: ");
				exit(-1);
			}

			while (stopNow == false) {	/* repeat until CTRL-C */
				rxCount = read(fd, rxBitstream, 4);
				if (rxCount == 4) {
					rxValue = (uint32_t)*&rxBitstream[0];
					if (LIRC_IS_TIMEOUT(rxValue))
						printf("\nRX Timeout");
					else if (LIRC_IS_PULSE(rxValue))
						printf("\n1 - %05d us", LIRC_VALUE(rxValue));
					else if (LIRC_IS_SPACE(rxValue))
						printf("\n0 - %05d us", LIRC_VALUE(rxValue));
				} else {
					if (rxCount == 0) {
						usleep(100 * 1000);	/* 100 ms */
						printf(".");
						fflush(stdout);
					} else {
						printf("Read %d bytes\n", rxCount);
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
			exit(1);
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
			exit(1);
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
			asciiCmdLength = txBitstream2culStr(txBitstream, txItemCount, repeatCount, asciiCmdStr);

			printf("CUL cmd: %s\n", asciiCmdStr);

			if (write(fd, asciiCmdStr, asciiCmdLength) < 0)
				perror("Error writing to CUL device");
			sleep(1);
		} else if (mode == MODE_READ) {
			stopNow = false;
			PRINT("Reading pulse_space_items\n");

			/*
			 * Set up signal handlers to act on CTRL-C events
			 */
			if (signal(SIGINT, signalTerminate) == SIG_ERR) {
				perror("Can't register signal handler for CTRL-C et al: ");
				exit(-1);
			}

			/* start rx */
			if (write(fd, "\r\nX01\r\n", 7) < 0) {
				perror("Error issuing RX cmd to CUL device");
				stopNow = true;
			}

			while (stopNow == false) {	/* repeat until CTRL-C */
				rxCount = read(fd, rxBitstream, 5);
				if (rxCount == 5) {
					rxValue = (uint32_t)*&rxBitstream[0];
					printf("\n%08X: ", rxValue);
					if (rxValue & 0x8000)
						printf("1 - %05d us", rxValue & 0x7FFF);
					else
						printf("0 - %05d us", rxValue & 0x7FFF);

					if ((rxValue & 0x7FFF) == 0x7FFF)
						printf(" - Timeout");
				} else {
					if (rxCount == 0) {
						usleep(100 * 1000);	/* 100 ms */
						printf(".");
						fflush(stdout);
					} else {
						printf("Read %d bytes\n", rxCount);
						fflush(stdout);
					}
				}
			}
		}
		close(fd);
		break;


	default:
		fprintf(stderr, "%s - Illegal interface type (%d)\n", prognm, rfInterface);
		break;
	}

	exit(0);
}


int createImpulsBitstream(const char *pChannelStr, const char *pOn_offStr, int32_t *pTxBitstream, int *repeatCount)
{
	int itemCount = 0;
	int on_offCode;
	int bit;

	on_offCode = atoi(pOn_offStr);	/* ON/OFF 0..1 */
	*repeatCount = SARTANO_REPEAT;

	PRINT("Channel: %s, on_off: %d\n", pChannelStr, on_offCode);

	/* check converted parameters for validity */
	if ((strlen(pChannelStr) != 10) || (on_offCode < 0) || (on_offCode > 1)) {
		fprintf(stderr, "Invalid channel or on/off code\n");
		return 0;
	}

	// The house code:
	for (bit = 0; bit < 5; bit++) {
		/* "1" bit */
		// 11101110 is on
		if (strncmp(pChannelStr + bit, "1", 1) == 0) {
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
		}
		/* "0" bit */
		else {
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
		}
	}

	// The group code
	for (bit = 5; bit < 10; bit++) {
		/* "1" bit */
		// 10001000 is on
		if (strncmp(pChannelStr + bit, "1", 1) == 0) {
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		}
		/* "0" bit */
		else {
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
			pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
		}
	}

	if (on_offCode >= 1) {
		/* ON == "10" */
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
	} else {
		/* OFF == "01" */
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_LONG_PERIOD);
		pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_LONG_PERIOD);
		pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SHORT_PERIOD);
	}

	/* add stop/sync bit and command termination char '+' */
	pTxBitstream[itemCount++] = LIRC_PULSE(SARTANO_SHORT_PERIOD);
	pTxBitstream[itemCount++] = LIRC_SPACE(SARTANO_SYNC_PERIOD);

	return itemCount;
}

/* Convert generic bitstream format to CUL433 format */
int txBitstream2culStr(int32_t *pTxBitstream, int txItemCount, int repeatCount, char *txStrCul)
{
	int i;
	int pulses = 0;
	char *pCulStr = txStrCul;
	char tmpStr[20];

	*pCulStr = '\0';

	strcat(pCulStr, "\r\nX01\r\n");	/* start radio */
	strcat(pCulStr, "E\r\n");	/* empty tx buffer */

	for (i = 0; i < txItemCount; i++) {
		sprintf(tmpStr, "%04X", LIRC_VALUE(pTxBitstream[i]));

		if (LIRC_IS_PULSE(pTxBitstream[i]) == true) {
			strcat(pCulStr, "A");
			strcat(pCulStr, tmpStr);
			pulses++;
		} else {	/* low */

			strcat(pCulStr, tmpStr);
			strcat(pCulStr, "\r\n");
		}
	}

	if (pulses > 1) {
		/* number of repetitions */
		sprintf(tmpStr, "S%02d\r\n", repeatCount);
		strcat(pCulStr, tmpStr);
		return strlen(pCulStr);
	}

	txStrCul[0] = '\0';
	return 0;
}

static void printUsage(void)
{
	printf("\nUsage: %s <-diprwgcslvh> [value]\n", prognm);
	printf("\t -d --device <path> defaults to %s\n", DEFAULT_DEVICE);
	printf("\t -i --interface. RFBB, CUL or TELLSTICK. Defaults to RFBB (RF Bitbanger)\n");
	printf("\t -p --protocol. NEXA, NEXA_L, SARTANO, WAVEMAN, IKEA or RAW\n");
	printf("\t -r --read. Raw space/pulse reading. For interfaces above that supports reading\n");
	printf("\t -w --write. Send command (default)\n");
	printf("\t -g --group. The group/house/system number or letter\n");
	printf("\t -c --channel. The channel/unit number\n");
	printf("\t -s --serialnumber. The serial/unique number used by NEXA L (self-learning)\n");
	printf("\t -l --level. 0 - 100. All values above 0 will switch on non dimmable devices\n");
	printf("\t -v --verbose\n");
	printf("\t -h --help\n");
	printf("\n\t Some useful protocol arguments - NEXA, WAVEMAN:\n");
	printf("\t\tgroup: A..P\n\t\tchannel: 1..16\n\t\toff/on: 0..1\n");
	printf("\n");
	printf("\t Protocol arguments - SARTANO:\n");
	printf("\t\tchannel: 0000000000..1111111111\n\t\toff/on: 0..1\n");
	printf("\n");
	printf("\t Protocol arguments - IKEA:\n");
	printf("\t\tgroup (system): 1..16\n\t\tchannel(device): 1..10\n");
	printf("\t\tlevel: 0..100\n\t\t(dimstyle 0..1)\n\n");
	printf("\tA typical example (NEXA D1 on): %s -d /dev/rfbb -i RFBB -p NEXA -g D -c 1 -l 1\n\n", prognm);
}

static void printVersion(void)
{
	printf("%s (RF Bitbanger cmd tool) v%s\n", prognm, VERSION);
	printf("\n");
	printf("Copyright (C) Tord Andersson 2010\n");
	printf("License: GPL v. 2\n");
	printf("Written by Tord Andersson. Code fragments from rfcmd by Tord Andersson, Micke Prag,\n");
	printf("Gudmund Berggren, Tapani Rintala and others\n");
}

static void signalTerminate(int signo)
{
	/*
	 * This will force the exit handler to run
	 */
	PRINT("Signal handler for %d signal\n", signo);
	stopNow = true;
}
