/**********************************************************************************************************************
 *                                                                                                                    *
 *  M O D C R . C                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File modcr.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of    *
 *  the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or  *
 *  (at your option) any later version.                                                                               *
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

/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[3] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Added",	1	},	/* 0 */
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Removed",	1	},	/* 1 */
	{	160,12, 0,	0,	0x07,	0,					"Filename", 0	},	/* 2 */
};

COLUMN_DESC *ptrChangeColumn[3] =
{
	&colChangeDescs[0],
	&colChangeDescs[1],
	&colChangeDescs[2]
};

int changeMode = 0;
int filesFound = 0;
int totalLines = 0;

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
	printf ("TheKnight: Modify Carrage Return, Version: %s, Built: %s\n", VERSION, buildDate);
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
	printf ("Enter the command: %s [-a|-r] <filename>\n", basename(name));
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

	while ((i = getopt(argc, argv, "ar?")) != -1)
	{
		switch (i)
		{
		case 'a':
			changeMode = 1;
			break;

		case 'r':
			changeMode = 2;
			break;

		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	if (changeMode == 0)
	{
			helpThem (argv[0]);
			exit (1);
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
		int col = changeMode - 1;

		if (!displayColumnInit (3, ptrChangeColumn, DISPLAY_HEADINGS))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 1;
		}
		directoryProcess (showDir, &fileList);

		displayDrawLine (0);
		displayInColumn (col, displayCommaNumber (totalLines, numBuff));
		displayInColumn (2, "CR's modified");
		displayNewLine(DISPLAY_INFO);
		displayInColumn (col, displayCommaNumber (filesFound, numBuff));
		displayInColumn (2, "Files changed");
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
	char inBuffer[256], outBuffer[512], inFile[PATH_SIZE], outFile[PATH_SIZE];
	int linesFixed = 0, i, j, bytesIn, col = changeMode - 1;
	FILE *readFile, *writeFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	strcat (outFile, "modcr$$$.000");

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		if ((writeFile = fopen (outFile, "wb")) != NULL)
		{
			while ((bytesIn = fread (inBuffer, 1, 255, readFile)) != 0)
			{
				inBuffer[bytesIn] = 0;
				for (i = j = 0; i < bytesIn; i++)
				{
					if (changeMode == 1)
					{
						if (inBuffer[i] != 10)
							outBuffer[j++] = inBuffer[i];
						else
						{
							outBuffer[j++] = 13;
							outBuffer[j++] = inBuffer[i];
							linesFixed++;
						}
					}
					else if (changeMode == 2)
					{
						if (inBuffer[i] != 13)
							outBuffer[j++] = inBuffer[i];
						else
							linesFixed++;
					}
				}
				if (j)
				{
					if (!fwrite (outBuffer, j, 1, writeFile))
					{
						linesFixed = 0;
						break;
					}
				}
			}
			fclose (writeFile);
		}
		fclose (readFile);

		if (linesFixed)
		{
			unlink (inFile);
			rename (outFile, inFile);
			totalLines += linesFixed;
			filesFound ++;
		}
		else
			unlink (outFile);

		displayInColumn (col, displayCommaNumber (linesFixed, inBuffer));
		displayInColumn (2, "%s", file -> fileName);
	}
	else
	{
		displayInColumn (col, "Failed");
		displayInColumn (2, "%s", file -> fileName);
	}
	displayNewLine (0);
	return linesFixed ? 1 : 0;
}

