/**********************************************************************************************************************
 *                                                                                                                    *
 *  L I N E  N U M . C                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File lineNum.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of  *
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
 *  \brief Display a text file with line numbers.
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

#define INBUFF_SIZE		2048
#define MASK_SIZE		2048

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
COLUMN_DESC colNumberDescs[3] =
{
	{ 20,	1,	2,	2,	0x02,	COL_ALIGN_RIGHT,	"Total",		0 },	/* 0 */
	{ 20,	1,	2,	2,	0x02,	COL_ALIGN_RIGHT,	"Line",			1 },	/* 1 */
	{ 1024, 10, 10, 0,	0x07,	0,					"Contents",		2 }		/* 2 */
};

COLUMN_DESC *ptrNumberColumn[3] =
{
	&colNumberDescs[0],
	&colNumberDescs[1],
	&colNumberDescs[2]
};

COLUMN_DESC fileDescs[] =
{
	{	60, 8,	16, 2,	0x07,	0,	"Filename", 1	},	/* 0 */
	{	20, 4,	4,	0,	0x07,	0,	"Size",		0	},	/* 1 */
};

COLUMN_DESC *ptrFileColumn[] =
{
	&fileDescs[0],	&fileDescs[1]
};

int tabSize = 8;
int showBlank = 1;
int filesFound = 0;
int totalLines = -1;
int displayQuiet = 0;
int displayFlags = 0;
int bitMaskSet = 0;
unsigned int bitMask[MASK_SIZE];

static struct option longOptions[] =
{
	{	"colour",		no_argument,		0,	'C' },
	{	"pages",		no_argument,		0,	'P' },
	{	"quiet",		no_argument,		0,	'q' },
	{	"totals",		no_argument,		0,	'T' },
	{	"blank",		no_argument,		0,	'b' },
	{	"lines",		required_argument,	0,	'l' },
	{	"tabs",			required_argument,	0,	't' },
	{	0,				0,					0,	0	}
};

void showStdIn (void);

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  B I T  M A S K                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set a bit can be in the range 0 to 511.
 *  \param bit Bit to be set.
 *  \result None.
 */
int setBitMask (int bit)
{
	unsigned int mask = 1, maskNum = bit >> 5, bitNum = bit & 0x1F;

	if (maskNum < MASK_SIZE)
	{
		bitMask[maskNum] |= (mask << bitNum);
		bitMaskSet = 1;
		return 1;
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  B I T  M A S K                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get whether a bit is set.
 *  \param bit Bit to check.
 *  \result 1 if it is set, 0 if not.
 */
int getBitMask (int bit)
{
	if (bitMaskSet)
	{
		unsigned int mask = 1, maskNum = bit >> 5, bitNum = bit & 0x1F;

		if (maskNum < MASK_SIZE)
		{
			return (bitMask[maskNum] & (mask << bitNum) ? 1 : 0);
		}
		return 0;
	}
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C  N U M B E R  R A N G E                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Allow you to input column number ranges and comma separated lists.
 *  \param value A number string that could be (1) (1-3) (3-) (-3) (1,2,3).
 *  \result None.
 */
void procNumberRange (char *value)
{
	int start = 0, end = 0, range = 0, ipos = 0, done = 0;

	while (!done)
	{
		if (value[ipos] >= '0' && value[ipos] <= '9')
		{
			if (range)
			{
				end = (end * 10) + (value[ipos] - '0');
			}
			else
			{
				start = (start * 10) + (value[ipos] - '0');
			}
		}
		else if (value[ipos] == '-')
		{
			if (range == 0)
			{
				if (start == 0)
				{
					start = 1;
				}
				range = 1;
			}
			else
			{
				start = end = range = 0;
			}
		}
		else if (value[ipos] == ',' || value[ipos] == 0)
		{
			/* Got here with 1-2 or -2 */
			if (start && end && range)
			{
				int l;
				for (l = start; l <= end; ++l)
				{
					setBitMask (l - 1);
				}
			}
			/* Got here with 1- */
			else if (start && range)
			{
				int l = start;
				while (setBitMask (l - 1))
				{
					++l;
				}
			}
			/* Got here with 1 */
			else if (start)
			{
				setBitMask (start - 1);
			}
			start = end = range = 0;
			if (value[ipos] == 0)
			{
				done = 1;
			}
		}
		else
		{
			start = end = range = 0;
		}
		++ipos;
	}
}

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
	printf ("TheKnight: Show Line Numbers for text files, Version: %s, Built: %s\n", VERSION, buildDate);
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
	printf ("Enter the command: %s [options] <filename>\n", basename(name));
	printf ("Options: \n");
	printf ("     --colour . . . -C . . . Display output in colour.\n");
	printf ("     --pages  . . . -P . . . Display output in pages.\n");
	printf ("     --blank  . . . -b . . . Do not count blank lines.\n");
	printf ("     --quiet  . . . -q . . . Quiet mode, only show file contents.\n");
	printf ("     --lines #  . . -l#  . . Lines to display, [example 1,3-5,7].\n");
	printf ("     --tabs # . . . -t#  . . Set the desired tab size, defaults to 8.\n");
	printf ("     --totals . . . -T . . . Show total lines in all files.\n");
	printf ("     --help . . . . -? . . . Display this help message.\n");
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
	int found = 0;
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

	while (1)
	{
		/*--------------------------------------------------------------------*
		 * getopt_long stores the option index here.                          *
	     *--------------------------------------------------------------------*/
		int optionIndex = 0;
		int opt = getopt_long (argc, argv, "bCPql:t:T?", longOptions, &optionIndex);

		/*--------------------------------------------------------------------*
		 * Detect the end of the options.                                     *
	     *--------------------------------------------------------------------*/
		if (opt == -1) break;

		switch (opt)
		{
		case 'b':
			showBlank = 0;
			break;
		case 'C':
			displayFlags |= DISPLAY_COLOURS;
			break;
		case 'P':
			displayFlags |= DISPLAY_IN_PAGES;
			break;
		case 'q':
			displayQuiet ^= 1;
			break;
		case 'l':
			procNumberRange (optarg);
			break;
		case 't':
			{
				int t = atoi (optarg);
				if (t > 0 && t <= 32)
				{
					tabSize = t;
				}
			}
			break;
		case 'T':
			totalLines = 0;
			break;
		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	if (optind == argc)
	{
		showStdIn ();
		exit (0);
	}
	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], ONLYFILES|ONLYLINKS, NULL, &fileList);
	}

	if (found)
	{
		/*--------------------------------------------------------------------*/
		/* Now we can sort the directory.                                     */
		/*--------------------------------------------------------------------*/
		directorySort (&fileList);
		directoryProcess (showDir, &fileList);

		if (filesFound && !displayQuiet)
		{
			if (!displayColumnInit (2, ptrFileColumn, 0))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 0;
			}
			displayDrawLine (0);
			displayInColumn (0, "%d %s shown\n", filesFound, filesFound == 1 ? "File" : "Files");
			displayNewLine(DISPLAY_INFO);
			displayAllLines ();
		}
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
 *  S H O W  F I L E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Show the contents of a file (could be stdin).
 *  \param readFile File handle to read from.
 *  \result Number of lines shown.
 */
int showFile (FILE *readFile)
{
	char inBuffer[INBUFF_SIZE + 1], outBuffer[(8 * INBUFF_SIZE) + 1];
	int linesFound = 0, terminated = 1, linesShown = 0;

	while (fgets (inBuffer, INBUFF_SIZE, readFile) != NULL)
	{
		int inPos = 0, outPos = 0, curPosn = 0, nextPosn = 0, blankLine = 0;

		if (terminated)
		{
			if (totalLines >= 0)
			{
				++totalLines;
			}
			++linesFound;
			terminated = 0;
		}
		while (inBuffer[inPos] && outPos < (8 * INBUFF_SIZE))
		{
			if (inBuffer[inPos] == ' ')
			{
				++nextPosn;
			}
			else if (inBuffer[inPos] == '\t')
			{
				nextPosn = ((nextPosn + tabSize) / tabSize) * tabSize;
			}
			else
			{
				while (curPosn < nextPosn && inBuffer[inPos] != '\n')
				{
					outBuffer[outPos++] = ' ';
					++curPosn;
				}
				if (inBuffer[inPos] >= ' ' && inBuffer[inPos] < 127)
				{
					outBuffer[outPos++] = inBuffer[inPos];
				}
				else if (inBuffer[inPos] < ' ' && inBuffer[inPos] != '\n')
				{
					outBuffer[outPos++] = '^';
					outBuffer[outPos++] = (inBuffer[inPos] + 'A');
				}
				else if (inBuffer[inPos] == '\n')
				{
					terminated = 1;
					if (showBlank)
						blankLine = 1;
				}
				nextPosn = ++curPosn;
			}
			++inPos;
		}
		if (outPos || blankLine)
		{
			if (getBitMask (linesFound - 1))
			{
				outBuffer[outPos] = 0;
				if (totalLines >= 0)
				{
					displayInColumn (0, "%d", totalLines);
				}
				displayInColumn (1, "%d", linesFound);
				displayInColumn (2, "%s", outBuffer);
				displayNewLine (0);
				++linesShown;
			}
		}
	}
	return linesShown;
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
	char fileBuffer[PATH_MAX];
	int linesShown = 0;
	FILE *readFile;

	/*------------------------------------------------------------------------*/
	/* First display a table with the file name and size.                     */
	/*------------------------------------------------------------------------*/
	if (!displayColumnInit (2, ptrFileColumn, 0))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}
	if (!displayQuiet)
	{
		displayDrawLine (0);
		displayHeading (0);
		displayNewLine (0);
		displayInColumn (0, "%s", file -> fileName);
#ifdef USE_STATX
		displayInColumn (1, displayFileSize (file -> fileStat.stx_size, (char *)fileBuffer));
#else
		displayInColumn (1, displayFileSize (file -> fileStat.st_size, (char *)fileBuffer));
#endif
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();
	}
	displayTidy ();

	strcpy (fileBuffer, file -> fullPath);
	strcat (fileBuffer, file -> fileName);

	if ((readFile = fopen (fileBuffer, "rb")) != NULL)
	{
		if (!displayColumnInit (3, ptrNumberColumn, displayFlags))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 0;
		}
		if (!displayQuiet)
		{
			displayDrawLine (0);
		}
		linesShown = showFile (readFile);
		fclose (readFile);
		displayAllLines ();
		displayTidy ();
	}
	if (linesShown)
	{
		++filesFound;
		return 1;
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  S T D  I N                                                                                               *
 *  ===================                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process stdin as if it was a file.
 *  \result None.
 */
void showStdIn ()
{
	/*------------------------------------------------------------------------*/
	/* First display a table with the file name and size.                     */
	/*------------------------------------------------------------------------*/
	if (!displayColumnInit (1, ptrFileColumn, 0))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return;
	}
	if (!displayQuiet)
	{
		displayDrawLine (0);
		displayHeading (0);
		displayNewLine (0);
		displayInColumn (0, "stdin");
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();
	}
	displayTidy ();

	if (!displayColumnInit (3, ptrNumberColumn, displayFlags))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return;
	}
	if (!displayQuiet)
	{
		displayDrawLine (0);
	}
	if (showFile (stdin) != 0)
	{
		if (!displayQuiet)
		{
			displayDrawLine (0);
		}
	}
	displayAllLines ();
	displayTidy ();
}

