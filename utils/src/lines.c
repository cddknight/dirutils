/**********************************************************************************************************************
 *                                                                                                                    *
 *  L I N E S . C                                                                                                     *
 *  =============                                                                                                     *
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
 *  \brief Program to count the number of lines in a file.
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dircmd.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
int showDir (DIR_ENTRY *file);

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

#define SHOW_PATH		1

int showFlags = 0;
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
	printf ("TheKnight: Count the Lines in a File, Version %s\n", directoryVersion());
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
	printf ("    -C . . . . . Display output in colour\n");
	printf ("    -c . . . . . Directory case sensitive\n");
	printf ("    -p . . . . . Show the path and filename\n");
	printf ("    -r . . . . . Search in subdirectories\n");
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

	if (strcmp (directoryVersion(), VERSION) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
		exit (1);
	}

	displayInit ();

	while ((i = getopt(argc, argv, "Ccpr?")) != -1)
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

		case 'p':
			showFlags ^= SHOW_PATH;
			break;

		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], dirType, fileCompare, &fileList);
	}

	/*------------------------------------------------------------------------*
     * Now we can sort the directory.                                         *
     *------------------------------------------------------------------------*/
	directorySort (&fileList);

	if (found)
	{
		char numBuff[15];

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
 *  S H O W  D I R                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Count the lines in a file.
 *  \param file File to count lines in.
 *  \result 1 if all OK.
 */
int showDir (DIR_ENTRY *file)
{
	char inBuffer[2048], inFile[PATH_SIZE];
	long linesFound = 0;
	FILE *readFile;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		filesFound ++;

		while (fgets (inBuffer, 2047, readFile))
			linesFound++;

		fclose (readFile);
	}
	totalLines += linesFound;

	displayInColumn (0, displayCommaNumber (linesFound, inBuffer));
	if (showFlags & SHOW_PATH)
	{
		displayInColumn (1, "%s", inFile);
	}
	else
	{
		displayInColumn (1, "%s", file -> fileName);
	}
	displayNewLine(0);

	return (linesFound ? 1 : 0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I L E  C O M P A R E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two files for sorting.
 *  \param fileOne First file.
 *  \param fileTwo Second file.
 *  \result 0, 1 or -1 depending on order.
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

