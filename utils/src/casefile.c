/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A S E F I L E . C                                                                                               *
 *  ===================                                                                                               *
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
 *  \brief Convert the case in a file.
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dircmd.h>

/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Defines                                                                    *
 *----------------------------------------------------------------------------*/
#define CASE_LOWER	0
#define CASE_UPPER	1
#define CASE_PROPER 2

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[3] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Lines",	1	},	/* 0 */
	{	160,12, 0,	0,	0x07,	0,					"Filename", 0	},	/* 1 */
};

COLUMN_DESC *ptrChangeColumn[3] =
{
	&colChangeDescs[0],
	&colChangeDescs[1]
};

int filesFound = 0;
int totalLines = 0;
int useCase = CASE_LOWER;

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
	printf ("TheKnight: Case File Contents, Version %s\n", directoryVersion());
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
	int i = 1, found =0;

	if (strcmp (directoryVersion(), VERSION) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
		exit (1);
	}

	displayInit ();

	while ((i = getopt(argc, argv, "lup?")) != -1)
	{
		switch (i)
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
			printf ("%s -[options] <file names>\n\n", basename (argv[0]));
			printf ("        -l  To make the file names lower case.\n");
			printf ("        -u  To make the file names upper case.\n");
			printf ("        -p  To make the file names proper case.\n");
			displayLine ();
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

		if (filesFound)
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
	char inBuffer[256], outBuffer[512], inFile[PATH_SIZE], outFile[PATH_SIZE];
	int linesFixed = 0, changed = 0, i, j, bytesIn;
	FILE *readFile, *writeFile;
	char lastChar = 0, byteIn;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	strcpy (outFile, file -> fullPath);
	strcat (outFile, "csfl$$$$.000");

	if ((readFile = fopen (inFile, "rb")) != NULL)
	{
		if ((writeFile = fopen (outFile, "wb")) != NULL)
		{
			while ((bytesIn = fread (inBuffer, 1, 255, readFile)) != 0)
			{
				for (i = j = 0; i < bytesIn; i++)
				{
					byteIn = inBuffer[i];

					switch (useCase)
					{
					case CASE_UPPER:
						if (islower (byteIn))
							inBuffer[i] = toupper(byteIn);
						break;
					case CASE_LOWER:
						if (isupper (byteIn))
							inBuffer[i] = tolower(byteIn);
						break;
					case CASE_PROPER:
						if (isalpha(lastChar))
							inBuffer[i] = tolower(byteIn);
						else
							inBuffer[i] = toupper(byteIn);
						break;
					}

					if (byteIn != inBuffer[i])
						changed = 1;

					if (byteIn == 10 && changed)
					{
						linesFixed ++;
						changed = 0;
					}
					outBuffer[j++] = lastChar = inBuffer[i];
				}
				if (j)
				{
					if (!fwrite (outBuffer, j, 1, writeFile))
					{
						printf ("Error writing temp file\n");
						linesFixed = 0;
						break;
					}
				}
			}
			if (changed)
				linesFixed ++;

			fclose (writeFile);
		}
		fclose (readFile);

		if (linesFixed)
		{
			unlink (inFile);
			rename (outFile, inFile);

			displayInColumn (0, displayCommaNumber (linesFixed, inBuffer));
			displayInColumn (1, "%s", file -> fileName);
			displayNewLine (0);

			totalLines += linesFixed;
			filesFound ++;
		}
		else
			unlink (outFile);
	}
	return linesFixed ? 1 : 0;
}

