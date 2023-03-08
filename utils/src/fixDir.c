/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A S E D I R . C                                                                                                 *
 *  =================                                                                                                 *
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
#define REMOVE_BIG	1
#define TEST_MODE	2
#define SHOW_ALL	4

#define CASE_NONE	0
#define CASE_LOWER	1
#define CASE_UPPER	2
#define CASE_PROPER	4

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

static char removeChars[] = "\"\'!?);:,]}|";
static char updateChars[] = " {[(";

int	dirType = ONLYFILES;
int filesFound = 0;
int totalChanged = 0;
int dispFlags = 0;
int fixCase = CASE_NONE;

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
	strcat (fullVersion, ".X");
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
	while ((i = getopt(argc, argv, "aATXc:")) != -1)
	{
		switch (i)
		{
		case 'a':
			dirType ^= SHOWALL;
			break;

		case 'A':
			dispFlags ^= SHOW_ALL;
			break;

		case 'T':
			dispFlags ^= TEST_MODE;
			break;

		case 'X':
			dispFlags ^= REMOVE_BIG;
			break;

		case 'c':
			switch (optarg[0])
			{
			case 'u':
				fixCase = CASE_UPPER;
				break;
			case 'l':
				fixCase = CASE_LOWER;
				break;
			case 'p':
				fixCase = CASE_PROPER;
				break;
			}
			break;

		case '?':
			version ();
			printf ("%s -[options] <file names>\n\n", basename (argv[0]));
			printf ("        -X  Change letters > 126 to X.\n");
			displayLine ();
			exit (1);
		}
	}

	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], dirType, NULL, &fileList);
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
		
		if (totalChanged > 0 || (dispFlags & SHOW_ALL))
		{
			displayDrawLine (0);
			displayInColumn (0, "Changed");
			displayInColumn (1, displayCommaNumber (totalChanged, numBuff));
			displayInColumn (2, "file%c", totalChanged == 1 ? ' ' : 's');
			displayNewLine(DISPLAY_INFO);
			displayInColumn (0, "Found");
			displayInColumn (1, displayCommaNumber (found, numBuff));
			displayInColumn (2, "file%c", found	== 1 ? ' ' : 's');
			displayNewLine(DISPLAY_INFO);
			displayAllLines ();
		}
		else
		{
			version ();
			printf ("No files changed\n");
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
	int i = 0, changed, number = 0, extn = 0, nextUpper = 1;
	char inFilePath[PATH_SIZE], inFileName[PATH_SIZE], outFilePath[PATH_SIZE], outFileName[PATH_SIZE], outExtn[PATH_SIZE];
	char lastAdd = 0, *outPtr;

	strcpy (inFileName, file -> fileName);
	strcpy (inFilePath, file -> fullPath);
	strcat (inFilePath, file -> fileName);
	outPtr = &outFileName[0];
	*outPtr = outExtn[0] = 0;

	while (inFileName[i])
	{
		if (strchr (updateChars, inFileName[i]) || inFileName[i] == '_')
		{
			if (lastAdd != '_')
			{
				*outPtr++ = lastAdd = '_';
				nextUpper = 1;
			}
		}
        else if (strchr (removeChars, inFileName[i]))
        {
            ;
        }
		else if (inFileName[i] == '.')
		{
			if (extn)
			{
				strcat (outFileName, outExtn);
			}
			outPtr = &outExtn[0];
			*outPtr++ = lastAdd = '.';
			nextUpper = 1;
			extn = 1;
		}
        else if (inFileName[i] == '-')
        {
			if (lastAdd != '_' && lastAdd != '-')
			{
				*outPtr++ = lastAdd = '-';
				nextUpper = 1;
			}
        }
        else if (inFileName[i] == '#')
        {
			*outPtr++ = (fixCase == CASE_LOWER) ? 'h' : 'H';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'A' : 'a';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'S' : 's';
			*outPtr++ = lastAdd = (fixCase == CASE_UPPER) ? 'H' : 'h';
			nextUpper = 1;
        }
        else if (inFileName[i] == '+')
        {
			*outPtr++ = (fixCase == CASE_LOWER) ? 'p' : 'P';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'L' : 'l';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'U' : 'u';
			*outPtr++ = lastAdd = (fixCase == CASE_UPPER) ? 'S' : 's';
			nextUpper = 1;
        }
        else if (inFileName[i] == '*')
        {
			*outPtr++ = (fixCase == CASE_LOWER) ? 's' : 'S';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'T' : 't';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'A' : 'a';
			*outPtr++ = lastAdd = (fixCase == CASE_UPPER) ? 'R' : 'r';
			nextUpper = 1;
        }
        else if (inFileName[i] == '!')
        {
			*outPtr++ = (fixCase == CASE_LOWER) ? 'p' : 'P';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'I' : 'i';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'N' : 'n';
			*outPtr++ = lastAdd = (fixCase == CASE_UPPER) ? 'G' : 'g';
			nextUpper = 1;
        }
        else if (inFileName[i] == '&')
        {
			*outPtr++ = (fixCase == CASE_LOWER) ? 'a' : 'A';
			*outPtr++ = (fixCase == CASE_UPPER) ? 'N' : 'n';
			*outPtr++ = lastAdd = (fixCase == CASE_UPPER) ? 'D' :'d';
			nextUpper = 1;
        }
        else if (inFileName[i] == '@')
        {
			*outPtr++ = (fixCase == CASE_UPPER || fixCase == CASE_PROPER) ? 'A' :'a';
			*outPtr++ = lastAdd = (fixCase == CASE_UPPER) ? 'T' :'t';
			nextUpper = 1;
        }
        else if (inFileName[i] < ' ')
        {
			*outPtr++ = lastAdd = 'x';
        }
        else if (inFileName[i] > 126 && (dispFlags & REMOVE_BIG))
        {
			*outPtr++ = lastAdd = 'X';
        }
		else
		{
			char ch = inFileName[i];
			if (fixCase == CASE_UPPER || (fixCase == CASE_PROPER && nextUpper))
			{
				ch = toupper (ch);
				if (isalpha (ch))
				{
					nextUpper = 0;
				}
			}
			else if (fixCase == CASE_LOWER || fixCase == CASE_PROPER)
			{
				ch = tolower (ch);
				if (!isalpha (ch) && fixCase == CASE_PROPER)
				{
					nextUpper = 1;
				}
			}
			*outPtr++ = lastAdd = ch;
		}
		*outPtr = 0;
		i++;
	}

	do
	{
		char *outFilePtr = &outFileName[0];
		struct stat statbuf;

		strcpy (outFilePath, file -> fullPath);
		outFilePtr = &outFilePath[strlen (outFilePath)];
		strcat (outFilePath, outFileName);
		if (number > 0)
		{
			char numBuff[40];
			sprintf (numBuff, "_%d", number);
			strcat (outFilePath, numBuff);		
		}
		if (extn)
		{
			strcat (outFilePath, outExtn);
		}
		changed = strcmp (outFilePtr, inFileName);
		if (changed)
		{
			if (stat (outFilePath, &statbuf) != 0)
			{
				int done = 1;

				if (!(dispFlags & TEST_MODE))
				{
					done = rename (inFilePath, outFilePath);
				}
				displayInColumn (0, "%s", inFileName);
				displayInColumn (1, done == 0 ? ">" : (dispFlags & TEST_MODE) ? "T" : "X");
				displayInColumn (2, "%s", outFilePtr);
				displayNewLine (0);
				totalChanged ++;
				break;
			}
			++number;
		}
		else
		{
			if (dispFlags & SHOW_ALL)
			{
				displayInColumn (0, "%s", inFileName);
				displayInColumn (1, "=");
				displayInColumn (2, "%s", outFilePtr);
				displayNewLine (0);
			}
		}
	}
	while (changed);
	filesFound ++;

	return changed;
}

