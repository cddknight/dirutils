/**********************************************************************************************************************
 *                                                                                                                    *
 *  H E X  D U M P . C                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File hexDump.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of  *
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
 *  \brief Program to dump the contence of a file in Hex.
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

COLUMN_DESC **ptrDumpColumn;

COLUMN_DESC fileDescs[] =
{
	{	60, 8,	16, 2,	0x07,	0,	"Filename", 1	},	/* 0 */
	{	20, 4,	4,	0,	0x07,	0,	"Size",		0	},	/* 1 */
};

COLUMN_DESC *ptrFileColumn[] =
{
	&fileDescs[0],	&fileDescs[1]
};

int filesFound = 0;
int displayFlags = 0;
int displayQuiet = 0;
int displayWidth = -1;
int displayCols = 0;

void showStdIn (void);

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
	printf ("TheKnight: Hex Dump a File, Version: %s, Built: %s\n", VERSION, buildDate);
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
	printf ("Enter the command: %s [-Cqw <size>] <filename>\n", basename (progName));
	printf ("    -C . . . . . Display output in colour.\n");
	printf ("    -P . . . . . Display output in pages.\n");
	printf ("    -q . . . . . Quite mode, only dump the hex codes.\n");
	printf ("    -w <size>  . Set the display size (8, 16, 24, 32, 40 or 48).\n");
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  C O L U M N                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Helper function to fill in column details.
 *  \param col Column to be configured.
 *  \param maxWidth Maximum width of the column.
 *  \param minWidth Minimum width of the column.
 *  \param gap Gap between this and the next column.
 *  \param colour Colour of the text, only used if colour is on.
 *  \param priority Priority of the column.
 *  \param heading Heading text, will be alloced and copied in the function.
 *  \result None.
 */
void configColumn (COLUMN_DESC *col, int maxWidth, int minWidth, int gap, int colour, int priority, char *heading)
{
	col -> maxWidth = maxWidth;
	col -> minWidth = minWidth;
	col -> startWidth = 0;
	col -> gap = gap;
	col -> colour = colour;
	col -> attrib = 0;
	col -> priority = priority;
	col -> heading = (char *)malloc (strlen (heading) + 1);
	if (col -> heading != NULL)
	{
		strcpy (col -> heading, heading);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A L L O C  T A B L E                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Allocate the table column descripters.
 *  \param displayCol Number of colums displaying text in the middle.
 *  \result Number of columns.
 */
int allocTable (int displayCol)
{
	int totalCols = displayCol + (displayCol / 8) + 2;

	ptrDumpColumn = (COLUMN_DESC **) malloc (totalCols * sizeof (COLUMN_DESC *));
	if (ptrDumpColumn != NULL)
	{
		int i, c = 0;

		memset (ptrDumpColumn, 0, totalCols * sizeof (COLUMN_DESC *));

		ptrDumpColumn[c] = (COLUMN_DESC *)malloc (sizeof (COLUMN_DESC));
		if (ptrDumpColumn[c] != NULL)
		{
			configColumn (ptrDumpColumn[c], 8, 6, 2, 0x07, 1, "Offset");
			++c;
		}
		for (i = 0; i < displayCol; ++i)
		{
			ptrDumpColumn[c] = (COLUMN_DESC *)malloc (sizeof (COLUMN_DESC));
			if (ptrDumpColumn[c] != NULL)
			{
				char tempbuff[11];
				sprintf (tempbuff, "%02X", i);
				configColumn (ptrDumpColumn[c], 2, 2, 1, (i & 1 ? 0x06 : 0x02), 2, tempbuff);
				++c;
			}
			if ((i + 1) % 8 == 0)
			{
				ptrDumpColumn[c] = (COLUMN_DESC *)malloc (sizeof (COLUMN_DESC));
				if (ptrDumpColumn[c] != NULL)
				{
					configColumn (ptrDumpColumn[c], 1, 1, 0, 0x07, 4, " ");
					++c;
				}
			}
		}
		ptrDumpColumn[c] = (COLUMN_DESC *)malloc (sizeof (COLUMN_DESC));
		if (ptrDumpColumn[c] != NULL)
		{
			configColumn (ptrDumpColumn[c], 80, 8, 0, 0x07, 2, "Text");
			++c;
		}
		return c;
	}
	return 0;
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
	int i, found = 0, width = 80;
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
	width = displayGetWidth();

	while ((i = getopt(argc, argv, "CPqw:?")) != -1)
	{
		switch (i)
		{
		case 'C':
			displayFlags |= DISPLAY_COLOURS;
			break;

		case 'P':
			displayFlags |= DISPLAY_IN_PAGES;
			break;

		case 'q':
			displayQuiet ^= 1;
			break;

		case 'w':
			width = atoi (optarg);
			displayWidth = (width / 8) * 8;
			if (displayWidth < 8 || displayWidth > 80)
			{
				displayWidth = 16;
			}
			break;

		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	/*----------------------------------------------------------------------------------------------------------------*
     * If the size option has not been used calculate the best one for this screen size.                              *
	 *----------------------------------------------------------------------------------------------------------------*/
	if (displayWidth == -1)
	{
		displayWidth = 8;
		do
		{
			int newWidth = (displayQuiet ? (displayWidth * 3) : ((displayWidth * 4) + (displayWidth / 8) + 10));
			if (newWidth > width)
			{
				displayWidth -= 8;
				break;
			}
			displayWidth += 8;
		}
		while (displayWidth < 80);
		if (displayWidth < 8)
		{
			displayWidth = 8;
		}
	}
	displayCols = allocTable (displayWidth);

	/*----------------------------------------------------------------------------------------------------------------*
     * If no file name has been passed in then use stdin.                                                             *
	 *----------------------------------------------------------------------------------------------------------------*/
	if (optind == argc)
	{
		showStdIn ();
	}
	else
	{
		for (; optind < argc; ++optind)
		{
			found += directoryLoad (argv[optind], ONLYFILES|ONLYLINKS, NULL, &fileList);
		}

		if (found)
		{
			/*--------------------------------------------------------------------------------------------------------*
    	     * Now we can sort the directory.                                                                         *
			 *--------------------------------------------------------------------------------------------------------*/
			directorySort (&fileList);
			directoryProcess (showDir, &fileList);
		}
		else
		{
			version ();
			printf ("No files found\n");
			return 0;
		}
	}
	if (!displayQuiet && filesFound)
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
		displayTidy ();
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  F I L E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the contents of a file (it could be stdin).
 *  \param readFile File to read from.
 *  \result None.
 */
void processFile (FILE *readFile)
{
	unsigned char inBuffer[2048 + 1];
	unsigned char saveHex[4], saveChar[512];
	int j = 0, c = 1, read, filePosn = 0, l = 0;

	while ((read = fread (inBuffer, 1, 2048, readFile)) != 0)
	{
		int i = 0;
		while (i < read)
		{
			sprintf ((char *)saveHex, "%02X", ((int)inBuffer[i]) & 0xFF);
			saveChar[j++] = (inBuffer[i] < 127 && inBuffer[i] > ' ') ? inBuffer[i] : '.';
			saveChar[j] = 0;

			if (j > 1 && (j - 1) % 8 == 0)
			{
				if (!displayQuiet)
				{
					displayInColumn (c, " ");
				}
				++c;
			}
			displayInColumn (c++, "%s", saveHex);

			if (j == displayWidth)
			{
				if (!displayQuiet)
				{
					displayInColumn (0, "%08X", filePosn);
					displayInColumn (displayCols - 2, " ", saveChar);
					displayInColumn (displayCols - 1, "%s", saveChar);
				}
				displayNewLine(0);
				filePosn += displayWidth;
				j = 0;

				if (++l == displayWidth)
				{
					if (!displayQuiet)
					{
						displayBlank (0);
					}
					l = 0;
				}
				c = 1;
			}
			i++;
		}
		if (read < 2048)
		{
			break;
		}
	}
	if (j)
	{
		if (!displayQuiet)
		{
			displayInColumn (0, "%08X", filePosn);
			displayInColumn (displayCols - 2, " ", saveChar);
			displayInColumn (displayCols - 1, "%s", saveChar);
		}
		displayNewLine (0);
	}
	displayAllLines ();
	displayTidy ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  D I R                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Hex dump the file.
 *  \param file File to dump.
 *  \result 1 if all OK.
 */
int showDir (DIR_ENTRY *file)
{
	unsigned char inFile[PATH_SIZE];
	FILE *readFile;

	/*----------------------------------------------------------------------------------------------------------------*
     * If the file is a link check it points to a regular file.                                                       *
	 *----------------------------------------------------------------------------------------------------------------*/
#ifdef USE_STATX
	if (S_ISLNK (file -> fileStat.stx_mode))
#else
	if (S_ISLNK (file -> fileStat.st_mode))
#endif
	{
		mode_t type = directoryTrueLinkType (file);
		if (!S_ISREG (type))
		{
			fprintf (stderr, "ERROR: Link does not point to file\n");
			return 0;
		}
	}

	/*----------------------------------------------------------------------------------------------------------------*
     * First display a table with the file name and size.                                                             *
	 *----------------------------------------------------------------------------------------------------------------*/
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
		displayInColumn (1, displayFileSize (file -> fileStat.stx_size, (char *)inFile));
#else
		displayInColumn (1, displayFileSize (file -> fileStat.st_size, (char *)inFile));
#endif
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();
	}
	displayTidy ();

	/*----------------------------------------------------------------------------------------------------------------*
     * Open the file and display a table containing the hex dump.                                                     *
	 *----------------------------------------------------------------------------------------------------------------*/
	strcpy ((char *)inFile, file -> fullPath);
	strcat ((char *)inFile, file -> fileName);

	if ((readFile = fopen ((char *)inFile, "rb")) != NULL)
	{
		if (!displayColumnInit (displayCols, ptrDumpColumn, displayFlags))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 0;
		}

		if (!displayQuiet) displayDrawLine (0);
		if (!displayQuiet) displayHeading (0);

		processFile (readFile);
		fclose (readFile);
		++filesFound;
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
 *  \brief Process the contents of stdin.
 *  \result 0 if all OK.
 */
void showStdIn (void)
{
	/*------------------------------------------------------------------------*
     * First display a table with the file name and size.                     *
     *------------------------------------------------------------------------*/
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

	if (!displayColumnInit (displayCols, ptrDumpColumn, displayFlags))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return;
	}

	if (!displayQuiet) displayDrawLine (0);
	if (!displayQuiet) displayHeading (0);

	processFile (stdin);
	++filesFound;
}

