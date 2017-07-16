/* Control tool for NEXA and other RF remote receivers
 *
 * Copyright (C) 2010, 2012  Tord Andersson <tord.andersson@endian.se>
 * Copyright (C) 2017        Joachim Nilsson <troglobit@gmail.com>
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

#include "common.h"
#include "protocol.h"

int ikea_bitstream(const char *house, const char *chan, const char *level,
		   const char *dim_style, int32_t *bitstream, int *repeat)
{
#if 0
	*pStrReturn = '\0';	/* Make sure tx Bitstream is empty */

	const char STARTCODE[] = "STTTTTT�";
	const char TT[] = "TT";
	const char A[] = "�";
	int system = atoi(house) - 1;	/* System 1..16 */
	int channel = atoi(chan);	/* Channel 1..10 */
	int lvl = atoi(level);	/* off,10,20,..,90,on */
	int dim = atoi(dim_style);
	int code = 0;
	int checksum1 = 0;
	int checksum2 = 0;
	int fade;
	int i;
	int raw = 0;

	/* check converted parameters for validity */
	if ((channel <= 0) || (channel > 10) || (system < 0) || (system > 15) ||
	    (lvl < 0) || (lvl > 10) || (dim < 0) || (dim > 1))
		return 0;

	if (channel == 10)
		channel = 0;
	raw = (1 << (9 - channel));

	strcat(pStrReturn, STARTCODE);	//Startcode, always like this;
	code = (system << 10) | raw;

	for (i = 13; i >= 0; --i) {
		if ((code >> i) & 1) {
			strcat(pStrReturn, TT);
			if (i % 2 == 0)
				checksum2++;
			else
				checksum1++;
		} else {
			strcat(pStrReturn, A);
		}
	}

	if (checksum1 % 2 == 0)
		strcat(pStrReturn, TT);
	else
		strcat(pStrReturn, A);	//1st checksum

	if (checksum2 % 2 == 0)
		strcat(pStrReturn, TT);
	else
		strcat(pStrReturn, A);	//2nd checksum

	if (dim == 1)
		fade = 11 << 4;	//Smooth
	else
		fade = 1 << 4;	//Instant

	switch (lvl) {
	case 0:
		code = (10 | fade);	//Concat level and fade
		break;

	case 1:
		code = (1 | fade);	//Concat level and fade
		break;

	case 2:
		code = (2 | fade);	//Concat level and fade
		break;

	case 3:
		code = (3 | fade);	//Concat level and fade
		break;

	case 4:
		code = (4 | fade);	//Concat level and fade
		break;

	case 5:
		code = (5 | fade);	//Concat level and fade
		break;

	case 6:
		code = (6 | fade);	//Concat level and fade
		break;

	case 7:
		code = (7 | fade);	//Concat level and fade
		break;

	case 8:
		code = (8 | fade);	//Concat level and fade
		break;

	case 9:
		code = (9 | fade);	//Concat level and fade
		break;

	case 10:
	default:
		code = (0 | fade);	//Concat level and fade
		break;
	}

	checksum1 = 0;
	checksum2 = 0;

	for (i = 0; i < 6; ++i) {
		if ((code >> i) & 1) {
			strcat(pStrReturn, TT);

			if (i % 2 == 0)
				checksum1++;
			else
				checksum2++;
		} else {
			strcat(pStrReturn, A);
		}
	}

	if (checksum1 % 2 == 0)
		strcat(pStrReturn, TT);
	else
		strcat(pStrReturn, A);	//2nd checksum

	if (checksum2 % 2 == 0)
		strcat(pStrReturn, TT);
	else
		strcat(pStrReturn, A);	//2nd checksum
	strcat(pStrReturn, "+");

	return strlen(pStrReturn);
#endif
	return 0;
}

