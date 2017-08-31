/**********************************************************************************************************************
 *                                                                                                                    *
 *  I N  Q U O T E . C                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU General Public         *
 *  License version 2 as published by the Free Software Foundation.  Note that I am not granting permission to        *
 *  redistribute or modify this under the terms of any later version of the General Public License.                   *
 *                                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the                *
 *  impliedwarranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in the file            *
 *  "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111,   *
 *  USA.                                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Print out text within quotes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dircmd.h>

#define SQ	0x27
#define	DQ	0x22

/*----------------------------------------------------------------------------*
 * Prototypes															      *
 *----------------------------------------------------------------------------*/
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[3] =
{
	{	10,	10,	0,	3,	0x07,	COL_ALIGN_RIGHT,	"Quotes",		1	},	/* 0 */
	{	160,12,	0,	3,	0x07,	0,					"Filename", 0	},	/* 1 */
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
	printf ("TheKnight: Extract Text in Quotes, Version %s\n", directoryVersion());
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

	if (strcmp (directoryVersion(), VERSION) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
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
		found += directoryLoad (argv[i++], ONLYFILES, fileCompare, &fileList);

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

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I L E  C O M P A R E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two directory names for sorting.
 *  \param fileOne First file.
 *  \param fileTwo Second file.
 *  \result The same as the result of strcmp.
 */
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo)
{
	if (fileOne -> fileStat.st_mode & S_IFDIR)
	{
		if (!(fileTwo -> fileStat.st_mode & S_IFDIR))
			return -1;
	}
	if (fileTwo -> fileStat.st_mode & S_IFDIR)
	{
		if (!(fileOne -> fileStat.st_mode & S_IFDIR))
			return 1;
	}
	return strcasecmp (fileOne -> fileName, fileTwo -> fileName);
}

