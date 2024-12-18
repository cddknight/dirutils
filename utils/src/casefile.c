/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A S E F I L E . C                                                                                               *
 *  ===================                                                                                               *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File casefile.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms    *
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
 *  \brief Convert the case in a file.
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
 * Defines                                                                    *
 *----------------------------------------------------------------------------*/
#define CASE_LOWER	0
#define CASE_UPPER	1
#define CASE_PROPER 2

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[3] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Lines",	1	},	/* 0 */
	{	160,12, 0,	0,	0x07,	0,					"Filename", 0	},	/* 1 */
};

COLUMN_DESC *ptrChangeColumn[3] =
{
	&colChangeDescs[0],
	&colChangeDescs[1]
};

int filesFound = 0;
int totalLines = 0;
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
	printf ("TheKnight: Case File Contents, Version: %s, Built: %s\n", VERSION, buildDate);
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
	int i = 1, found =0;
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
			printf ("        -l  To make the contents lower case.\n");
			printf ("        -u  To make the contents upper case.\n");
			printf ("        -p  To make the contents proper case.\n");
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

		if (!displayColumnInit (2, ptrChangeColumn, DISPLAY_HEADINGS))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 1;
		}
		directoryProcess (showDir, &fileList);

		if (filesFound)
			displayDrawLine (0);

		displayInColumn (0, displayCommaNumber (totalLines, numBuff));
		displayInColumn (1, "Lines changed");
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
	char inBuffer[256], outBuffer[512], inFile[PATH_SIZE], outFile[PATH_SIZE];
	int linesFixed = 0, changed = 0, i, j, bytesIn;
	FILE *readFile, *writeFile;
	char lastChar = 0, byteIn;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	strcat (outFile, "csfl$$$$.000");

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		if ((writeFile = fopen (outFile, "wb")) != NULL)
		{
			while ((bytesIn = fread (inBuffer, 1, 255, readFile)) != 0)
			{
				for (i = j = 0; i < bytesIn; i++)
				{
					byteIn = inBuffer[i];

					switch (useCase)
					{
					case CASE_UPPER:
						if (islower (byteIn))
							inBuffer[i] = toupper(byteIn);
						break;
					case CASE_LOWER:
						if (isupper (byteIn))
							inBuffer[i] = tolower(byteIn);
						break;
					case CASE_PROPER:
						if (isalpha(lastChar))
							inBuffer[i] = tolower(byteIn);
						else
							inBuffer[i] = toupper(byteIn);
						break;
					}

					if (byteIn != inBuffer[i])
						changed = 1;

					if (byteIn == 10 && changed)
					{
						linesFixed ++;
						changed = 0;
					}
					outBuffer[j++] = lastChar = inBuffer[i];
				}
				if (j)
				{
					if (!fwrite (outBuffer, j, 1, writeFile))
					{
						printf ("Error writing temp file\n");
						linesFixed = 0;
						break;
					}
				}
			}
			if (changed)
				linesFixed ++;

			fclose (writeFile);
		}
		fclose (readFile);

		if (linesFixed)
		{
			unlink (inFile);
			rename (outFile, inFile);

			displayInColumn (0, displayCommaNumber (linesFixed, inBuffer));
			displayInColumn (1, "%s", file -> fileName);
			displayNewLine (0);

			totalLines += linesFixed;
			filesFound ++;
		}
		else
			unlink (outFile);
	}
	return linesFixed ? 1 : 0;
}

