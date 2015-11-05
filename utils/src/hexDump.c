/**********************************************************************************************************************
 *                                                                                                                    *
 *  H E X  D U M P . C                                                                                                *
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
 *  \brief Program to dump the contence of a file in Hex.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dircmd.h>

/*----------------------------------------------------------------------------*
 * Prototypes															      *
 *----------------------------------------------------------------------------*/
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 * 00 00 00 00 00 00 00 00-08 13 50 00 00 00 00 00 : ..........P.....         *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colDumpDescs[4] =
{
	{	8,	6,	6,	3,	0x0E,	COL_ALIGN_RIGHT,	"Offset",	1	},	/* 0 */
	{	95,	47,	47,	3,	0x0A,	0,					"Hex",		0	},	/* 1 */
	{	32,	16,	16,	0,	0x0B,	0,					"Text",		2	},	/* 2 */
	{	47,	47,	47,	3,	0x0A,	0,					"",			0	},	/* 1 */
};

COLUMN_DESC *ptrDumpColumn[3] =
{
	&colDumpDescs[0],
	&colDumpDescs[1],
	&colDumpDescs[2]
};

COLUMN_DESC *ptrDumpColQuiet[1] =
{
	&colDumpDescs[3],
};

int filesFound = 0;
int displayColour = 0;
int displayQuiet = 0;
int displayBig = 0;

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
	printf ("TheKnight: Hex Dump a File, Version %s\n", directoryVersion());
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

	displayGetWindowSize ();

	if (argc == 1)
	{
		version ();
		printf ("Enter the command: %s [-bqC] <filename>\n", basename (argv[0]));
		exit (1);
	}
	
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
			case 'b':
				displayBig = 1;
				break;

			case 'C':
				displayColour = DISPLAY_COLOURS;
				break;
				
			case 'q':
				displayQuiet ^= 1;
				break;
			}
		}
		else
			found += directoryLoad (argv[i], ONLYFILES, fileCompare, &fileList);

		i ++;
	}

    /*------------------------------------------------------------------------*
     * Now we can sort the directory.                                         *
     *------------------------------------------------------------------------*/
	directorySort (&fileList);

	if (found)
	{
		if (!displayQuiet)
		{
			if (!displayColumnInit (3, ptrDumpColumn, displayColour))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 1;
			}
		}
		else
		{
			if (!displayColumnInit (1, ptrDumpColQuiet, 0))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 1;
			}
		}
		directoryProcess (showDir, &fileList);
		if (!displayQuiet)
		{
			displayInColumn (1, "%d %s shown\n", filesFound, 
					filesFound == 1 ? "File" : "Files");
		}
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
 *  \brief Hex dump the file.
 *  \param file File to dump.
 *  \result 1 if all OK.
 */
int showDir (DIR_ENTRY *file)
{
	unsigned char inBuffer[1024 + 1], inFile[PATH_SIZE];
	unsigned char saveHex[160], saveChar[80];
	FILE *readFile;
	int i, j = 0, read, filePosn = 0, l = 0, width;

	width = displayBig ? 32 : 16;

	strcpy ((char *)inFile, file -> fullPath);
	strcat ((char *)inFile, file -> fileName);
	
	if ((readFile = fopen ((char *)inFile, "rb")) != NULL)
	{
		if (!displayQuiet) displayInColumn (1, "%s", file -> fileName);
		if (!displayQuiet) displayDrawLine (0);
		if (!displayQuiet) displayHeading (0);
		
		while ((read = fread (inBuffer, 1, 1024, readFile)) != 0)
		{
			i = 0;
			while (i < read)
			{
				sprintf ((char *)&saveHex[j * 3], "%c%02X", j == (width / 2) ? '-' : ' ',
					((int)inBuffer[i]) & 0xFF);
				saveChar[j++] = (inBuffer[i] < 127 && inBuffer[i] > ' ')
					? inBuffer[i] : '.';

				saveChar[j] = 0;

				if (j == width)
				{
					int c = 0;
					if (!displayQuiet) displayInColumn (c++, "%06X", filePosn);
					displayInColumn (c++, "%s", &saveHex[1]);
					if (!displayQuiet) displayInColumn (c++, "%s", saveChar);
					displayNewLine(0);

					filePosn += width;
					j = 0;
					if (++l == width)
					{
						displayBlank (0);
						l = 0;
					}
				}
				i++;
			}
		}
		if (j)
		{
			int c = 0;
			if (!displayQuiet) displayInColumn (c++, displayBig ? "%06X" : "%08X", filePosn);
			displayInColumn (c++, "%s", &saveHex[1]);
			if (!displayQuiet) displayInColumn (c++, "%s", saveChar);
			displayNewLine(0);
		}
		if (!displayQuiet) displayDrawLine (0);
		displayAllLines ();
		
		fclose (readFile);
		filesFound ++;
	}
	return 0;
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
