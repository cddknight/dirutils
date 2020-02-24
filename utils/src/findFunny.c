/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I N D  F U N N Y . C                                                                                            *
 *  ======================                                                                                            *
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
 *  \brief Find funny chars.
 */
#include "config.h"
#define _GNU_SOURCE
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/fcntl.h>
#include <getopt.h>
#ifdef HAVE_VALUES_H
#include <values.h>
#else
#define MAXINT 2147483647
#endif

#include <dircmd.h>
#include "buildDate.h"

/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colLinesDescs[4] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Line", 0	},	/* 0 */
	{	10, 10, 0,	3,	0x07,	0,					"Funny",	1	},	/* 1 */
	{	160,12, 0,	0,	0x07,	0,					"Filename", 2	},	/* 2 */
};

COLUMN_DESC *ptrLinesColumn[4] =
{
	&colLinesDescs[0],
	&colLinesDescs[1],
	&colLinesDescs[2]
};

int filesFound = 0;
long totalFunny = 0;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V E R S I O N                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the version of the application.
 *  \result None.
 */
void version (void)
{
	printf ("TheKnight: Find funny chars in a File, Version: %s, Built: %s\n", VERSION, buildDate);
	displayLine ();
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
	void *fileList = NULL;
	int i = 1, found = 0;
	char fullVersion[81];

	strcpy (fullVersion, VERSION);
#ifdef USE_STATX
	strcat (fullVersion, ".X");
#endif
	if (strcmp (directoryVersion(), fullVersion) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), fullVersion);
		exit (1);
	}

	displayInit ();

	if (argc == 1)
	{
		version ();
		printf ("Enter the command: %s <filename>\n", argv[0]);
		exit (1);
	}
	/*------------------------------------------------------------------------*
     * If we got a path then split it into a path and a file pattern to match *
     * files with.                                                            *
     *------------------------------------------------------------------------*/
	while (i < argc)
		found += directoryLoad (argv[i++], ONLYFILES|ONLYLINKS, NULL, &fileList);

	/*------------------------------------------------------------------------*
     * Now we can sort the directory.                                         *
     *------------------------------------------------------------------------*/
	directorySort (&fileList);

	if (found)
	{
		char numBuff[15];

		if (!displayColumnInit (3, ptrLinesColumn, DISPLAY_HEADINGS))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 1;
		}
		directoryProcess (showDir, &fileList);

		displayDrawLine (0);
		displayInColumn (1, displayCommaNumber (totalFunny, numBuff));
		displayInColumn (2, "Funnies");
		displayNewLine(DISPLAY_INFO);
		displayInColumn (1, displayCommaNumber (filesFound, numBuff));
		displayInColumn (2, "Files Found");
		displayNewLine(DISPLAY_INFO);
		displayAllLines ();

		displayTidy ();
	}
	else
	{
		version ();
		printf ("No files found\n");
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  D I R                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Count the lines in a file.
 *  \param file File to count lines in.
 *  \result 1 if all OK.
 */
int showDir (DIR_ENTRY *file)
{
	char inBuffer[2049], tempBuffer[41], inFile[PATH_SIZE];
	long linesFound = 0, fileTotal = 0;
	FILE *readFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		filesFound ++;

		while (fgets (inBuffer, 2048, readFile))
		{
			int i = 0;
			while (inBuffer[i])
			{
				switch (inBuffer[i])
				{
				case '\t': /* tab */
					break;
				case '\n': /* CR */
					++linesFound;
					break;
				default:
					if (inBuffer[i] < ' ' || inBuffer[i] >= 127)
					{
						displayInColumn (0, displayCommaNumber (linesFound, tempBuffer));
						displayInColumn (1, "0x%02X", inBuffer[i] & 0xFF);
						displayInColumn (2, "%s", file -> fileName);
						displayNewLine(0);
						++fileTotal;
					}
					break;
				}
				++i;
			}
		}
		if (fileTotal == 0)
		{
			displayInColumn (1, "None");
			displayInColumn (2, "%s", file -> fileName);
			displayNewLine(0);
		}
		else
		{
			totalFunny += fileTotal;
		}
		fclose (readFile);
	}
	return (linesFound ? 1 : 0);
}

