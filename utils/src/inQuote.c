/**********************************************************************************************************************
 *                                                                                                                    *
 *  I N  Q U O T E . C                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File inQuote.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of  *
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
 *  \brief Print out text within quotes.
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

#define SQ	0x27
#define DQ	0x22

/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[3] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Quotes",		1	},	/* 0 */
	{	160,12, 0,	3,	0x07,	0,					"Filename", 0	},	/* 1 */
};

COLUMN_DESC *ptrChangeColumn[3] =
{
	&colChangeDescs[0],
	&colChangeDescs[1]
};

int filesFound = 0;
int totalLines = 0;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V E R S I O N                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the library version.
 *  \result None.
 */
void version (void)
{
	printf ("TheKnight: Extract Text in Quotes, Version: %s, Built: %s\n", VERSION, buildDate);
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

		if (!displayColumnInit (2, ptrChangeColumn, DISPLAY_HEADINGS))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 1;
		}
		directoryProcess (showDir, &fileList);

		displayDrawLine (0);
		displayInColumn (0, displayCommaNumber (totalLines, numBuff));
		displayInColumn (1, "Quotes found");
		displayNewLine(DISPLAY_INFO);
		displayInColumn (0, displayCommaNumber (filesFound, numBuff));
		displayInColumn (1, "Files found");
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
 *  \brief Display a directory entry.
 *  \param file Directory entry to display, in the case extract text from.
 *  \result 1 if text was extracted.
 */
int showDir (DIR_ENTRY *file)
{
	char inBuffer[256], inFile[PATH_SIZE];
	int inQuote = 0, i, bytesIn, quoteFound = 0;
	FILE *readFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		while ((bytesIn = fread (inBuffer, 1, 255, readFile)) != 0)
		{
			inBuffer[bytesIn] = 0;
			for (i = 0; i < bytesIn; i++)
			{
				if (inBuffer[i] == DQ && inQuote == 2)
				{
					fprintf (stderr, "\n");
					inQuote = 0;
				}
				else if (inBuffer[i] == DQ && inQuote == 0)
				{
					quoteFound++;
					inQuote = 2;
				}
				else if (inBuffer[i] == SQ && inQuote == 1)
				{
					fprintf (stderr, "\n");
					inQuote = 0;
				}
				else if (inBuffer[i] == SQ && inQuote == 0)
				{
					quoteFound++;
					inQuote = 1;
				}
				else if (inQuote)
				{
					fprintf (stderr, "%c", inBuffer[i]);
				}
			}
		}
		fclose (readFile);

		displayInColumn (0, displayCommaNumber (quoteFound, inBuffer));
		displayInColumn (1, "%s", file -> fileName);
		displayNewLine (0);

		totalLines += quoteFound;
		filesFound ++;
	}
	return quoteFound ? 1 : 0;
}

