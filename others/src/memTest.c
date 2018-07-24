/**********************************************************************************************************************
 *                                                                                                                    *
 *  M E M  T E S T . C                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU General Public         *
 *  License version 2 as published by the Free Software Foundation.  Note that I am not granting permission to        *
 *  redistribute or modify this under the terms of any later version of the General Public License.                   *
 *                                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied        *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in the file            *
 *  "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111,   *
 *  USA.                                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Program to test the memory for problems.
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <dircmd.h>
#include <unistd.h>

#define CHECKSIZE	(1024 * 1024)

void *memQueue;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T E S T  M E M O R Y                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Create, test and free the memory blocks.
 *  \param mode 0 create, 1 test, 2 free.
 *  \param loopCount Number of blocks.
 *  \param oldValue Old value to check for.
 *  \param newValue New value to set.
 *  \result Number of faults found.
 */
int testMemory (int mode, int loopCount, unsigned char oldValue, unsigned char newValue)
{
	int i, j, problems = 0;
	unsigned char *buff;

	if (mode == 0)
		printf ("Allocate memory and write (%02X) ...\n", newValue);
	else if (mode == 1)
		printf ("Check (%02X) and write (%02X) ...\n", oldValue, newValue);
	else
		printf ("Check (%02X) and free memory ...\n", oldValue);

	for (i = 0; i < loopCount; ++i)
	{
		if (mode == 0)
		{
			buff = (unsigned char *)malloc (CHECKSIZE);
			if (buff)
			{
				for (j = 0; j < CHECKSIZE; ++j)
					buff[j] = newValue;
				queuePut (memQueue, buff);
			}
		}
		else
		{
			buff = (unsigned char *)queueGet (memQueue);
			if (buff)
			{
				for (j = 0; j < CHECKSIZE; ++j)
				{
					if (buff[j] != oldValue)
					{
						++problems;
						printf ("Problem @ %p[%d] = [%X]\n", buff, j, (unsigned int)buff[j]);
						buff = NULL;
						break;
					}
					else if (mode == 1)
					{
						buff[j] = newValue;
					}
				}
				if (buff)
				{
					if (mode == 1)
						queuePut (memQueue, buff);
					else
						free (buff);
				}
			}
		}
	}
	return problems;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A I N                                                                                                           *
 *  =======                                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief The program starts here.
 *  \param argc The number of arguments passed to the program.
 *  \param argv Pointers to the arguments passed to the program.
 *  \result 0 (zero) if all process OK.
 */
int main (int argc, char *argv[])
{
	int loopCount, problems = 0;

	if (argc != 2)
	{
		printf ("memtest <number of 1M blocks>\n");
		exit (1);
	}

	memQueue = queueCreate ();
	loopCount = atoi (argv[1]);

	/*************************************************************************
    *                                                                        *
    *************************************************************************/
	problems += testMemory (0, loopCount, 0, 0);
	sleep (1);
	problems += testMemory (1, loopCount, 0, 0x55);
	sleep (1);
	problems += testMemory (1, loopCount, 0x55, 0xAA);
	sleep (1);
	problems += testMemory (1, loopCount, 0xAA, 0xFF);
	sleep (1);
	problems += testMemory (2, loopCount, 0xFF, 0);

	printf ("Done %d problem(s) found.\n", problems);

	/*************************************************************************
    *                                                                        *
    *************************************************************************/
	queueDelete (memQueue);
	exit (0);
}

