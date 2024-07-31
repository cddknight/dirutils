/**********************************************************************************************************************
 *                                                                                                                    *
 *  N U M  C O M M E N T . C                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 *  Copyright (c) 2024                                                                                                *
 *                                                                                                                    *
 *  File numComment.c is free software: you can redistribute it and/or modify it under the terms of the GNU General   *
 *  Public License as published by the Free Software Foundation, either version 3 of the License, or (at your         *
 *  option) any later version.                                                                                        *
 *                                                                                                                    *
 *  File numComment.c is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the   *
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for  *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see:           *
 *  <http://www.gnu.org/licenses/>                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Replace marker with numbered comments.
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

/**********************************************************************************************************************
 * Prototypes                                                                                                         *
 **********************************************************************************************************************/
int showDir (DIR_ENTRY *file);

/**********************************************************************************************************************
 * Globals                                                                                                            *
 **********************************************************************************************************************/
COLUMN_DESC colChangeDescs[3] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Updated",	1	},	/* 1 */
	{	160,12, 0,	0,	0x07,	0,					"Filename", 0	},	/* 2 */
};

COLUMN_DESC *ptrChangeColumn[3] =
{
	&colChangeDescs[0],
	&colChangeDescs[1],
	&colChangeDescs[2]
};

int incCount = 1;
int startCount = 1;
int filesFound = 0;
int totalLines = 0;
char *matchStr = "/* # */";
char formatStr[41] = "/* %1d */";

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
	printf ("TheKnight: Add numbered comments, Version: %s, Built: %s\n", VERSION, buildDate);
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
	printf ("Enter the command: %s -[options] <filename>\n", basename(name));
	printf ("     -h . . . . Count up in hex [false].\n");
	printf ("     -i # . . . Number to increment by [1] (can be negative).\n");
	printf ("     -p # . . . Pad the number up to # chars [1] (1 to 20).\n");
	printf ("     -s # . . . Set starting number [1] (can be negative).\n");
	printf ("     -z . . . . Zero pad the numbers [false].\n");
	printf ("This utility replaces \"/* # */\" with \"/* 123 */\".\n");
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
	int i, found = 0, padSize = 1, zeroPad = 0, hexNum = 0;
	void *fileList = NULL;
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
	displayGetWidth();

	while ((i = getopt(argc, argv, "hi:p:s:z?")) != -1)
	{
		switch (i)
		{
		case 'h':
			hexNum = !hexNum;
			break;

		case 'i':
			sscanf (optarg, "%d", &incCount);
			if (incCount < -100 || incCount > 100)
			{
				incCount = 1;
			}
			break;

		case 'p':
			sscanf (optarg, "%d", &padSize);
			if (padSize < 1 || padSize > 20)
			{
				padSize = 1;
			}
			break;

		case 's':
			sscanf (optarg, "%d", &startCount);
			if (startCount < -32767 || startCount > 32767)
			{
				startCount = 1;
			}
			break;

		case 'z':
			zeroPad = !zeroPad;
			break;

		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	if (zeroPad)
	{
		sprintf (formatStr, "/* %%0%d%c */", padSize, hexNum ? 'X' : 'd');
	}
	else
	{
		sprintf (formatStr, "/* %%%d%c */", padSize, hexNum ? 'X' : 'd');
	}
	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], ONLYFILES, NULL, &fileList);
	}
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
		displayInColumn (1, "CR's modified");
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
	char inBuffer[256], outBuffer[600], inFile[PATH_SIZE], outFile[PATH_SIZE];
	int linesFixed = 0, bytesIn, match = 0, count = startCount, err = 0;
	FILE *readFile, *writeFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	strcat (outFile, "modcr$$$.000");

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		if ((writeFile = fopen (outFile, "wb")) != NULL)
		{
			while ((bytesIn = fread (inBuffer, 1, 255, readFile)) != 0 && !err)
			{
				int i, j;

				inBuffer[bytesIn] = 0;
				for (i = j = 0; i < bytesIn; i++)
				{
					if (inBuffer[i] == matchStr[match])
					{
						if (matchStr[++match] == 0)
						{
							sprintf (&outBuffer[j], formatStr, count);
							j = strlen (outBuffer);
							count += incCount;
							++linesFixed;
							match = 0;
						}
					}
					else
					{
						if (match)
						{
							int k;
							for (k = 0; k < match; ++k)
							{
								outBuffer[j++] = matchStr[k];
								outBuffer[j] = 0;
							}
							match = 0;
						}
						if (inBuffer[i] == matchStr[0])
						{
							match = 1;
						}
						else
						{
							outBuffer[j++] = inBuffer[i];
							outBuffer[j] = 0;
						}
					}
					if (j > 500)
					{
						if (!fwrite (outBuffer, j, 1, writeFile))
						{
							err = 1;
						}
						outBuffer[j = 0] = 0;
					}
				}
				if (j && !err)
				{
					if (!fwrite (outBuffer, j, 1, writeFile))
					{
						err = 1;
					}
				}
			}
			fclose (writeFile);
		}
		fclose (readFile);

		if (linesFixed && !err)
		{
			unlink (inFile);
			rename (outFile, inFile);
			totalLines += linesFixed;
			filesFound ++;
		}
		else
			unlink (outFile);

		displayInColumn (0, displayCommaNumber (linesFixed, inBuffer));
		displayInColumn (1, "%s", file -> fileName);
	}
	else
	{
		displayInColumn (0, "Failed");
		displayInColumn (1, "%s", file -> fileName);
	}
	displayNewLine (0);
	return linesFixed ? 1 : 0;
}
