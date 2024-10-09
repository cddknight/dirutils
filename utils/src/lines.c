/**********************************************************************************************************************
 *                                                                                                                    *
 *  L I N E S . C                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File lines.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of    *
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
 *  \brief Program to count the number of lines in a file.
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
int readDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colLinesDescs[3] =
{
	{	10, 10, 0,	3,	0x0A,	COL_ALIGN_RIGHT,	"Lines",	0	},	/* 0 */
	{	255,12, 0,	0,	0x0E,	0,					"Filename", 1	},	/* 1 */
};

COLUMN_DESC *ptrLinesColumn[3] =
{
	&colLinesDescs[0],
	&colLinesDescs[1]
};

#define SHOW_PATH		0x0001
#define SHOW_RORDER		0x0002

#define ORDER_NAMES		0
#define ORDER_LINES		1

int showFlags = 0;
int showOrder = ORDER_NAMES;
int filesFound = 0;
long totalLines = 0;

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
	printf ("TheKnight: Count the Lines in a File, Version: %s, Built: %s\n", VERSION, buildDate);
	displayLine ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  H E L P  T H E M                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function to display help information.
 *  \param progName Name of the running program.
 *  \result None.
 */
void helpThem(char *progName)
{
	printf ("Enter the command: %s [-Ccpr] <filename>\n", basename (progName));
	printf ("    -C . . . . . Display output in colour.\n");
	printf ("    -c . . . . . Directory case sensitive.\n");
	printf ("    -on  . . . . Order results by file name.\n");
	printf ("    -ol  . . . . Order by number of lines.\n");
	printf ("    -or  . . . . Reverse the current order.\n");
	printf ("    -p . . . . . Show the path and filename.\n");
	printf ("    -r . . . . . Search in subdirectories.\n");
	printf ("    -R . . . . . Search links to directories.\n");
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R E C T O R Y  C O M P A R E                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Not using default so that we can compare number of lines.
 *  \param fileOne First file to compare.
 *  \param fileTwo Second file to compare.
 *  \result -1, 0 or 1 depening on the order.
 */
int directoryCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo)
{
	int retn = 0;

	if (showOrder == ORDER_LINES)
	{
		if (fileOne -> match > fileTwo -> match)
		{
			retn = 1;
		}
		else if (fileOne -> match < fileTwo -> match)
		{
			retn = -1;
		}
	}
	if (retn == 0)
	{
		retn = strcasecmp (fileOne -> fileName, fileTwo -> fileName);
	}
	if (showFlags & SHOW_RORDER && retn)
	{
		retn = (retn == 1 ? -1 : 1);
	}
	return retn;
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
	int i = 1, found = 0, dirType = ONLYFILES|ONLYLINKS, displayColour = 0;;
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

	while ((i = getopt(argc, argv, "Cco:prR?")) != -1)
	{
		switch (i)
		{
		case 'C':
			displayColour = DISPLAY_COLOURS;
			break;

		case 'c':
			dirType ^= USECASE;
			break;

		case 'r':
			dirType ^= RECUDIR;
			break;

		case 'R':
			dirType ^= RECULINK;
			break;

		case 'p':
			showFlags ^= SHOW_PATH;
			break;

		case 'o':
		{
			int i = 0;
			while (optarg[i])
			{
				if (optarg[i] == 'l')
				{
					showOrder = ORDER_LINES;
				}
				else if (optarg[i] == 'n')
				{
					showOrder = ORDER_NAMES;
				}
				else if (optarg[i] == 'r')
				{
					showFlags ^= SHOW_RORDER;
				}
				else
				{
					helpThem (argv[0]);
					exit (1);
				}
				++i;
			}
			break;
		}
		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], dirType, directoryCompare, &fileList);
	}

	/*------------------------------------------------------------------------*
     * Now we can process the directory.                                      *
     *------------------------------------------------------------------------*/
	if (found)
	{
		char numBuff[15];

		directoryRead (readDir, &fileList);
		directorySort (&fileList);
		if (!displayColumnInit (2, ptrLinesColumn, DISPLAY_HEADINGS | displayColour))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 1;
		}
		directoryProcess (showDir, &fileList);

		displayDrawLine (0);
		displayInColumn (0, displayCommaNumber (totalLines, numBuff));
		displayInColumn (1, "Lines Found");
		displayNewLine(DISPLAY_INFO);
		displayInColumn (0, displayCommaNumber (filesFound, numBuff));
		displayInColumn (1, "Files Found");
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
 *  R E A D  D I R                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called back before the sort to read the lines in the file.
 *  \param file File to count lines in.
 *  \result 1 if all OK.
 */
int readDir (DIR_ENTRY *file)
{
	char inBuffer[16384], inFile[PATH_SIZE];
	long linesFound = 0;
	FILE *readFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		int readBytes;
		++filesFound;
		do
		{
			int i;
			readBytes = fread (inBuffer, 1, 16384, readFile);
			for (i = 0; i < readBytes; ++i)
			{
				if (inBuffer[i] == '\n')
					++linesFound;
			}
		}
		while (readBytes > 0);
		fclose (readFile);
	}
	file -> match = linesFound;
	totalLines += linesFound;

	return (linesFound ? 1 : 0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  D I R                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Count the lines in a file.
 *  \param file File to show.
 *  \result 1 if all OK.
 */
int showDir (DIR_ENTRY *file)
{
	char numBuffer[41];
	long linesFound = file -> match;

	displayInColumn (0, displayCommaNumber (linesFound, numBuffer));
	if (showFlags & SHOW_PATH)
	{
		char inFile[PATH_SIZE];
		strcpy (inFile, file -> fullPath);
		strcat (inFile, file -> fileName);
		displayInColumn (1, "%s", inFile);
	}
	else
	{
		displayInColumn (1, "%s", file -> fileName);
	}
	displayNewLine(0);

	return (linesFound ? 1 : 0);
}

