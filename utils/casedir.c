/******************************************************************************************************
 *                                                                                                    *
 *  C A S E D I R . C                                                                                 *
 *  =================                                                                                 *
 *                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU        *
 *  General Public License version 2 as published by the Free Software Foundation.  Note that I am    *
 *  not granting permission to redistribute or modify this under the terms of any later version of    *
 *  the General Public License.                                                                       *
 *                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even    *
 *  the impliedwarranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          *
 *  General Public License for more details.                                                          *
 *                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in     *
 *  the file "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place -       *
 *  Suite 330, Boston, MA 02111, USA.                                                                 *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @file
 *  @brief Change the case of the files in a directory.
 *  @version $Id: casedir.c 1076 2010-10-02 20:38:45Z chris $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dircmd.h>

/*----------------------------------------------------------------------*/
/* Prototypes															*/
/*----------------------------------------------------------------------*/
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------*/
/* Defines   															*/
/*----------------------------------------------------------------------*/
#define CASE_LOWER  0
#define CASE_UPPER  1
#define CASE_PROPER 2

/*----------------------------------------------------------------------*/
/* Globals                                                              */
/*----------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[3] =
{
	{	160,12,	0,	3,	0x07,	0,					"Old Name",	1	},	/* 0 */
	{	10,	3,	0,	3,	0x07,	COL_ALIGN_RIGHT,	"To",		2	},	/* 1 */
	{	160,12,	0,	0,	0x07,	0,					"New Name",	0	},	/* 2 */
};

COLUMN_DESC *ptrChangeColumn[3] =
{
	&colChangeDescs[0],
	&colChangeDescs[1],
	&colChangeDescs[2]
};

int filesFound = 0;
int totalChanged = 0;
int useCase = CASE_LOWER;

/******************************************************************************************************
 *                                                                                                    *
 *  V E R S I O N                                                                                     *
 *  =============                                                                                     *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Print the version of the application.
 *  @result None.
 */
void version (void)
{
	printf ("TheKnight: Case Directory Contents, Version %s\n", directoryVersion());
	displayLine ();
}

/******************************************************************************************************
 *                                                                                                    *
 *  M A I N                                                                                           *
 *  =======                                                                                           *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief The program starts here.
 *  @param argc The number of arguments passed to the program.
 *  @param argv Pointers to the arguments passed to the program.
 *  @result 0 (zero) if all process OK.
 */
int main (int argc, char *argv[])
{
	void *fileList = NULL;
	int i = 1, found = 0;

	displayGetWindowSize ();

    /*------------------------------------------------------------------------*
     * If we got a path then split it into a path and a file pattern to match *
     * files with.                                                            *
     *------------------------------------------------------------------------*/
	while (i < argc)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
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
				printf ("casedir -[options] <file names>\n\n");
				printf ("        -l  To make the file names lower case.\n");
				printf ("        -u  To make the file names upper case.\n");
				printf ("        -p  To make the file names proper case.\n");
				displayLine ();
				exit (1);
			}
		}
		i++;
	}
	i = 1;

	while (i < argc)
	{
		if (argv[i][0] != '-')
			found += directoryLoad (argv[i], ONLYFILES, fileCompare, &fileList);
		i++;
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
		
		displayDrawLine (0);
		displayInColumn (0, "Changed");
		displayInColumn (1, displayCommaNumber (totalChanged, numBuff));
		displayInColumn (2, "file%c", totalChanged == 1 ? ' ' : 's');
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

/******************************************************************************************************
 *                                                                                                    *
 *  S H O W  D I R                                                                                    *
 *  ==============                                                                                    *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Convert the file that was found.
 *  @param file File to convert.
 *  @result 1 if file changed.
 */
int showDir (DIR_ENTRY *file)
{
	int makeIt = (useCase == CASE_LOWER ? CASE_LOWER : CASE_UPPER);
	char inFile[PATH_SIZE], inFilename[PATH_SIZE], outFile[PATH_SIZE];
	int changed = 0, i = 0, j;

	strcpy (inFilename, file -> fileName);
	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	j = strlen (outFile);

	while (inFilename[i])
	{
		switch (makeIt)
		{
		case CASE_LOWER:
			outFile[j] = tolower (inFilename[i]);
			break;

		case CASE_UPPER:
			outFile[j] = toupper (inFilename[i]);
			break;
		}
		if (!changed)
			changed = (outFile[j] != inFilename[i]);

		if (useCase == CASE_PROPER)
			makeIt = (isalpha (outFile[j]) ? CASE_LOWER : CASE_UPPER);

		i++;
		j++;
	}
	outFile[j] = 0;

	if (changed)
	{
		displayInColumn (0, "%s", inFile);
		displayInColumn (1, "-->");
		displayInColumn (2, "%s", outFile);
		displayNewLine (0);			
		totalChanged ++;
		
		rename (inFile, outFile);
	}
	filesFound ++;

	return changed;
}

/******************************************************************************************************
 *                                                                                                    *
 *  F I L E  C O M P A R E                                                                            *
 *  ======================                                                                            *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Compare two files for sorting.
 *  @param fileOne First file.
 *  @param fileTwo Second file.
 *  @result 0, 1 or -1 depending on compare.
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
