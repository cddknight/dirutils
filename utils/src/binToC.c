/**********************************************************************************************************************
 *                                                                                                                    *
 *  B I N  T O  C . C                                                                                                 *
 *  =================                                                                                                 *
 *                                                                                                                    *
 *  Copyright (c) 2026                                                                                                *
 *                                                                                                                    *
 *  File binToC.c is free software: you can redistribute it and/or modify it under the terms of the GNU General       *
 *  Public License as published by the Free Software Foundation, either version 3 of the License, or (at your         *
 *  option) any later version.                                                                                        *
 *                                                                                                                    *
 *  File binToC.c is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the       *
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for  *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see:           *
 *  <http://www.gnu.org/licenses/>                                                                                    *
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
COLUMN_DESC colChangeDescs[2] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Read",	1	},	/* 0 */
	{	160,12, 0,	0,	0x07,	0,					"Filename", 0	},	/* 2 */
};

COLUMN_DESC *ptrChangeColumn[2] =
{
	&colChangeDescs[0],
	&colChangeDescs[1]
};

int colCount = 32;
int filesFound = 0;
int totalRead = 0;
char varName[256];

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
	printf ("TheKnight: Convert a binary file to C string, Version: %s, Built: %s\n", VERSION, buildDate);
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
	printf ("Enter the command: %s <filename>\n", basename(name));
	printf ("     -c #  . . . . . Number of columns in the array [32].\n");
	printf ("     -v <name> . . . Name of the character array.\n");
	printf ("                     By default array name is based on file name.\n");
	printf ("     -?  . . . . . . Show this help message.\n");
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
	int i, found = 0;
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

	while ((i = getopt(argc, argv, "c:v:?")) != -1)
	{
		switch (i)
		{
		case 'c':
			sscanf (optarg, "%d", &colCount);
			if (colCount < 8 || colCount > 200)
			{
				colCount = 32;
			}
			break;

		case 'v':
			strncpy (varName, optarg, 250);
			break;
			
		case '?':
			helpThem (argv[0]);
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

		displayDrawLine (0);
		displayInColumn (0, displayCommaNumber (totalRead, numBuff));
		displayInColumn (1, "Bytes read");
		displayNewLine(DISPLAY_INFO);
		displayInColumn (0, displayCommaNumber (filesFound, numBuff));
		displayInColumn (1, "Files read");
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
	char inBuffer[256], outBuffer[2048], inFile[PATH_SIZE], outFile[PATH_SIZE];
	int bytesRead = 0, i = 0, j = 0, bytesIn, skip = 0;
	FILE *readFile, *writeFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	strcat (outFile, file -> fileName);
	strcat (outFile, ".h");

	if (varName[0] == 0)
	{
		strncpy (inBuffer, file -> fileName, 250);
		for (i = 0; i < strlen (inBuffer); ++i)
		{
			if ((inBuffer[i] >= 'A' && inBuffer[i] <= 'Z') || 
				(inBuffer[i] >= 'a' && inBuffer[i] <= 'z') || 
				(inBuffer[i] >= '0' && inBuffer[i] <= '9') ||
				inBuffer[i] == '-')
			{
				varName[j++] = inBuffer[i];
				varName[j] = 0;
				skip = 0;
			}
			else if (!skip)
			{
				varName[j++] = '_';
				varName[j] = 0;
				skip = 1;
			}
		}
		if (varName[0] == 0)
		{
			strcpy (varName, "data");
		}
	}
	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		if ((writeFile = fopen (outFile, "wb")) != NULL)
		{
			fprintf (writeFile, "unsigned char %s[] =\n{", varName);
			while ((bytesIn = fread (inBuffer, 1, colCount, readFile)) != 0)
			{
				bytesRead += bytesIn;
				inBuffer[bytesIn] = 0;
				
				fprintf (writeFile, "\n\t");
				for (i = j = 0; i < bytesIn; i++)
				{
					sprintf (&outBuffer[j], "0x%02X,", inBuffer[i] & 0xFF);
					j += 5;
				}
				if (j)
				{
					if (!fwrite (outBuffer, j, 1, writeFile))
					{
						break;
					}
				}
			}
			fprintf (writeFile, "\n};\n#define %s_size %d\n", varName, bytesRead);
			fclose (writeFile);
		}
		fclose (readFile);

		displayInColumn (0, displayCommaNumber (bytesRead, inBuffer));
		displayInColumn (1, "%s", outFile);
	}
	else
	{
		displayInColumn (0, "Failed");
		displayInColumn (1, "%s", outFile);
	}
	displayNewLine (0);
	if (bytesRead)
	{
		totalRead += bytesRead;
		++filesFound;
		return 1;
	}
	return 0;
}

