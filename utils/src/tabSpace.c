/**********************************************************************************************************************
 *                                                                                                                    *
 *  T A B  S P A C E . C                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU General Public         *
 *  License version 2 as published by the Free Software Foundation.  Note that I am not granting permission to        *
 *  redistribute or modify this under the terms of any later version of the General Public License.                   *
 *                                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied        *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in the file            *
 *  "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111,   *
 *  USA.                                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Convert between tabs and spaces.
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
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Changed",	1	},	/* 0 */
	{	160,12, 0,	0,	0x07,	0,					"Filename", 0	},	/* 1 */
};

COLUMN_DESC *ptrChangeColumn[3] =
{
	&colChangeDescs[0],
	&colChangeDescs[1]
};

int tabSize = 8;
int debugMode = 0;
int spaceToTab = 0;
int modComment = 0;
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
	printf ("TheKnight: Convert Tabs and Spaces, Version: %s, Built: %s\n", VERSION, buildDate);
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
	printf ("     -c . . . Modify comments as well as code.\n");
	printf ("     -s . . . Convert spaces to tabs, default is tabs to spaces.\n");
	printf ("     -tN  . . Set the desired tab size, defaults to 8.\n");
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

	while ((i = getopt(argc, argv, "cdst:?")) != -1)
	{
		switch (i)
		{
		case 'c':
			modComment = 1;
			break;

		case 'd':
			debugMode = 1;
			break;

		case 's':
			spaceToTab = 1;
			break;

		case 't':
			tabSize = atoi (optarg);
			if (tabSize <= 0)
			{
				fprintf (stderr, "Tab size must be greater than 0.\n");
				helpThem (argv[0]);
				exit (1);
			}
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
 *  T E S T  C H A R                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Test to see if we are going into or out of a comment.
 *  \param thisChar The last character read from the file.
 *  \param lastChar The character before this one (zero on first read).
 *  \param current Current state.
 *  \param line Current line number.
 *  \result New state.
 */
int testChar (char thisChar, char lastChar, int current, int line)
{
	int retn = current;
	static int isDouble = 0;

	if (current)
	{
		if (isDouble)
		{
			lastChar = 0;
			isDouble = 0;
		}
		else if (thisChar == '\\' && lastChar == '\\' && (current == 1 || current == 2))
		{
			isDouble = 1;
		}
		if (current == 1 && thisChar == '\'' && lastChar != '\\') retn = 0;
		else if (current == 2 && thisChar == '"' && lastChar != '\\') retn = 0;
		else if (current == 3 && thisChar == '\n') retn = 0;
		else if (current == 4 && thisChar == '/' && lastChar == '*') retn = 0;
		if (retn == 0) isDouble = 0;
	}
	else
	{
		if (thisChar == '\'') retn = 1;
		else if (thisChar == '"') retn = 2;
		else if (!modComment)
		{
			if (thisChar == '/' && lastChar == '/') retn = 3;
			else if (thisChar == '*' && lastChar == '/') retn = 4;
		}
	}
	if (debugMode && retn != current)
	{
		char lastC[5], thisC[5];
		if (lastChar <= ' ' || lastChar >= 127)
			sprintf (lastC, "%02X", lastChar & 0xFF);
		else
			sprintf (lastC, "%c", lastChar);

		if (thisChar <= ' ' || thisChar >= 127)
			sprintf (thisC, "%02X", thisChar & 0xFF);
		else
			sprintf (thisC, "%c", thisChar);

		printf ("Line: %d, Last mode: %d, New mode: %d, (%s)(%s)\n", line + 1, current, retn, lastC, thisC);
	}
	return retn;
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
	char inBuffer[1025], outBuffer[4096], inFile[PATH_SIZE], outFile[PATH_SIZE], lastChar = 0;
	int linesFixed = 0, linesRead = 0, inComOrQuo = 0;
	FILE *readFile, *writeFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	strcat (outFile, "modtab$$$.000");

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		if ((writeFile = fopen (outFile, "wb")) != NULL)
		{
			int blankLineCount = 0;
			while (fgets (inBuffer, 1024, readFile) != NULL)
			{
				int inPos = 0, outPos = 0, curPosn = 0, nextPosn = 0;
				++linesRead;
				while (inBuffer[inPos])
				{
					if (inBuffer[inPos] == ' ' && !inComOrQuo)
					{
						++nextPosn;
					}
					else if (inBuffer[inPos] == '\t' && !inComOrQuo)
					{
						nextPosn = ((nextPosn + tabSize) / tabSize) * tabSize;
					}
					else
					{
						/**********************************************************************************************/
						/* Do not add white space if the next character is \n.                                        */
						/**********************************************************************************************/
						if (inBuffer[inPos] != '\n')
						{
							if (spaceToTab)
							{
								int one = (curPosn + 1 == nextPosn ? 1 : 0);
								while (curPosn < nextPosn)
								{
									if (((curPosn + tabSize) / tabSize) * tabSize <= nextPosn && !one)
									{
										outBuffer[outPos++] = '\t';
										curPosn = ((curPosn + tabSize) / tabSize) * tabSize;
									}
									else
									{
										outBuffer[outPos++] = ' ';
										++curPosn;
									}
								}
							}
							else
							{
								while (curPosn < nextPosn)
								{
									outBuffer[outPos++] = ' ';
									++curPosn;
								}
							}
							blankLineCount = 0;
							outBuffer[outPos++] = inBuffer[inPos];
							nextPosn = ++curPosn;
						}
						else if (outPos == 0)
						{
							++blankLineCount;
						}
						inComOrQuo = testChar (inBuffer[inPos], lastChar, inComOrQuo, linesRead);
					}
					/**************************************************************************************************/
					/* If line out is too big, just output the original line.                                         */
					/**************************************************************************************************/
					if (outPos + nextPosn > 4090)
					{
						fwrite (inBuffer, strlen(inBuffer), 1, writeFile);
						outPos = 0;
						break;
					}
					lastChar = inBuffer[inPos];
					++inPos;
				}
				if (outPos || blankLineCount == 1)
				{
					outBuffer[outPos++] = '\n';
					outBuffer[outPos] = 0;
					fwrite (outBuffer, outPos, 1, writeFile);
					if (strncmp (outBuffer, inBuffer, outPos))
					{
						++linesFixed;
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
			++filesFound;
		}
		else
		{
			unlink (outFile);
		}
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

