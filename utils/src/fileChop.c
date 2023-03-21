/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I L E  C H O P . C                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File fileChop.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms    *
 *  of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License,  *
 *  or (at your option) any later version.                                                                            *
 *                                                                                                                    *
 *  DirCmdUtils is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the         *
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for  *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see            *
 *  <http://www.gnu.org/licenses/>.                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Add CR's to LF's to make a file DOS compatable.
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

#define EXTN ".chopped"

/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[4] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Removed",	1	},	/* 0 */
	{	160,12, 0,	0,	0x07,	0,					"Input", 0	},	/* 1 */
	{	160,12, 0,	0,	0x07,	0,					"Output", 0	},	/* 1 */
};

COLUMN_DESC *ptrChangeColumn[4] =
{
	&colChangeDescs[0],
	&colChangeDescs[1],
	&colChangeDescs[2]
};

int filesFound = 0;
int totalSkipped = 0;
long startPos = 0;
long endPos = LONG_MAX;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V E R S I O N                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Print the version of the application.
 *  \result None.
 */
void version (void)
{
	printf ("TheKnight: Chop files, Version: %s, Built: %s\n", VERSION, buildDate);
	displayLine ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  H E L P  T H E M                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display help information.
 *  \param name Name of the application.
 *  \result None.
 */
void helpThem (char *name)
{
	version ();
	printf ("Enter the command: %s [-s <start>] [-e <end>] <filename>\n", basename(name));
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
	int i, found = 0;
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
	displayGetWidth();

	while ((i = getopt(argc, argv, "e:s:?")) != -1)
	{
		switch (i)
		{
		case 'e':
			endPos = atol (optarg);
			break;

		case 's':
			startPos = atol (optarg);
			break;

		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}
	
	if (endPos <= startPos && endPos != -1)
	{
		fprintf (stderr, "ERROR: End must be greater than start\n");
		return 1;
	}

	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], ONLYFILES, NULL, &fileList);
	}

	/*------------------------------------------------------------------------*
     * Now we can sort the directory.                                         *
     *------------------------------------------------------------------------*/
	directorySort (&fileList);

	if (found)
	{
		char numBuff[15];

		if (!displayColumnInit (3, ptrChangeColumn, DISPLAY_HEADINGS))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 1;
		}
		directoryProcess (showDir, &fileList);

		displayDrawLine (0);
		displayInColumn (0, displayCommaNumber (totalSkipped, numBuff));
		displayInColumn (1, "Bytes removed");
		displayNewLine(DISPLAY_INFO);
		displayInColumn (0, displayCommaNumber (filesFound, numBuff));
		displayInColumn (1, "Files changed");
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
 *  \brief Convert the file that was found.
 *  \param file File to convert.
 *  \result 1 if file changed.
 */
int showDir (DIR_ENTRY *file)
{
	char inBuffer[10250], outBuffer[10250], inFile[PATH_SIZE], outFile[PATH_SIZE];
	long bytesInTotal = 0, bytesRead, bytesSkipped = 0, posIn, posOut = 0;
	FILE *readFile, *writeFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	strcat (outFile, file -> fileName);
	strcat (outFile, EXTN);

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		if ((writeFile = fopen (outFile, "wb")) != NULL)
		{
			while ((bytesRead = fread (inBuffer, 1, 10240, readFile)) != 0)
			{
				for (posIn = 0; posIn < bytesRead; ++posIn)
				{
					if (bytesInTotal + posIn < startPos || bytesInTotal + posIn >= endPos)
					{
						outBuffer[posOut++] = inBuffer[posIn];
					}
					else
					{
						++bytesSkipped;
					}
				}
				fwrite (outBuffer, 1, posOut, writeFile);
				bytesInTotal += bytesRead;
				posOut = 0;
			}
			fclose (writeFile);
			displayInColumn (0, displayCommaNumber (bytesSkipped, inBuffer));
			displayInColumn (1, "%s", file -> fileName);
			displayInColumn (2, "%s%s", file -> fileName, EXTN);
			totalSkipped += bytesSkipped;
		}
		else
		{
			displayInColumn (0, "Write fail");
			displayInColumn (1, "%s%s", file -> fileName, EXTN);
		}
		fclose (readFile);
	}
	else
	{
		displayInColumn (0, "Read fail");
		displayInColumn (1, "%s", file -> fileName);
	}
	displayNewLine (0);
	return bytesSkipped ? 1 : 0;
}

