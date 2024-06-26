/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A S E D I R . C                                                                                                 *
 *  =================                                                                                                 *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File casedir.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of  *
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
 *  \brief Change the case of the files in a directory.
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

/*----------------------------------------------------------------------*/
/* Prototypes                                                           */
/*----------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------*/
/* Defines                                                              */
/*----------------------------------------------------------------------*/
#define CASE_LOWER	0
#define CASE_UPPER	1
#define CASE_PROPER 2

/*----------------------------------------------------------------------*/
/* Globals                                                              */
/*----------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[3] =
{
	{	160,12, 0,	3,	0x07,	0,					"Old Name", 1	},	/* 0 */
	{	10, 3,	0,	3,	0x07,	COL_ALIGN_RIGHT,	"To",		2	},	/* 1 */
	{	160,12, 0,	0,	0x07,	0,					"New Name", 0	},	/* 2 */
};

COLUMN_DESC *ptrChangeColumn[3] =
{
	&colChangeDescs[0],
	&colChangeDescs[1],
	&colChangeDescs[2]
};

int filesFound = 0;
int totalChanged = 0;
int useCase = CASE_LOWER;

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
	printf ("TheKnight: Case Directory Contents, Version: %s, Built: %s\n", VERSION, buildDate);
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
	strcat (fullVersion, ".1");
#else
	strcat (fullVersion, ".0");
#endif
	if (strcmp (directoryVersion(), fullVersion) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), fullVersion);
		exit (1);
	}

	displayInit ();

	/*------------------------------------------------------------------------*
     * If we got a path then split it into a path and a file pattern to match *
     * files with.                                                            *
     *------------------------------------------------------------------------*/
	while ((i = getopt(argc, argv, "lup?")) != -1)
	{
		switch (i)
		{
		case 'l':
			useCase = CASE_LOWER;
			break;

		case 'u':
			useCase = CASE_UPPER;
			break;

		case 'p':
			useCase = CASE_PROPER;
			break;

		case '?':
			version ();
			printf ("%s -[options] <file names>\n\n", basename (argv[0]));
			printf ("        -l  To make the file names lower case.\n");
			printf ("        -u  To make the file names upper case.\n");
			printf ("        -p  To make the file names proper case.\n");
			displayLine ();
			exit (1);
		}
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
		displayInColumn (0, "Changed");
		displayInColumn (1, displayCommaNumber (totalChanged, numBuff));
		displayInColumn (2, "file%c", totalChanged == 1 ? ' ' : 's');
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
	int makeIt = (useCase == CASE_LOWER ? CASE_LOWER : CASE_UPPER);
	char inFile[PATH_SIZE], inFilename[PATH_SIZE], outFile[PATH_SIZE];
	int changed = 0, i = 0, j;

	strcpy (inFilename, file -> fileName);
	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	j = strlen (outFile);

	while (inFilename[i])
	{
		switch (makeIt)
		{
		case CASE_LOWER:
			outFile[j] = tolower (inFilename[i]);
			break;

		case CASE_UPPER:
			outFile[j] = toupper (inFilename[i]);
			break;
		}
		if (!changed)
			changed = (outFile[j] != inFilename[i]);

		if (useCase == CASE_PROPER)
			makeIt = (isalpha (outFile[j]) ? CASE_LOWER : CASE_UPPER);

		i++;
		j++;
	}
	outFile[j] = 0;

	if (changed)
	{
		displayInColumn (0, "%s", inFile);
		displayInColumn (1, "-->");
		displayInColumn (2, "%s", outFile);
		displayNewLine (0);
		totalChanged ++;

		rename (inFile, outFile);
	}
	filesFound ++;

	return changed;
}

