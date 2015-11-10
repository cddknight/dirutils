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
#include <unistd.h>
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
COLUMN_DESC colDumpDescs[38] =
{
	{	8,	6,	0,	2,	0x0B,	0,					"Offset",	1	},	/* 0 */
	{	2,	2,	0,	1,	0x0A,	0,					"00",		0	},	/* 1 */
	{	2,	2,	0,	1,	0x0E,	0,					"01",		0	},	/* 2 */
	{	2,	2,	0,	1,	0x0A,	0,					"02",		0	},	/* 3 */
	{	2,	2,	0,	1,	0x0E,	0,					"03",		0	},	/* 4 */
	{	2,	2,	0,	1,	0x0A,	0,					"04",		0	},	/* 5 */
	{	2,	2,	0,	1,	0x0E,	0,					"05",		0	},	/* 6 */
	{	2,	2,	0,	1,	0x0A,	0,					"06",		0	},	/* 7 */
	{	2,	2,	0,	1,	0x0E,	0,					"07",		0	},	/* 8 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},
	{	2,	2,	0,	1,	0x0A,	0,					"08",		0	},	/* 9 */
	{	2,	2,	0,	1,	0x0E,	0,					"09",		0	},	/* 10 */
	{	2,	2,	0,	1,	0x0A,	0,					"0A",		0	},	/* 11 */
	{	2,	2,	0,	1,	0x0E,	0,					"0B",		0	},	/* 12 */
	{	2,	2,	0,	1,	0x0A,	0,					"0C",		0	},	/* 13 */
	{	2,	2,	0,	1,	0x0E,	0,					"0D",		0	},	/* 14 */
	{	2,	2,	0,	1,	0x0A,	0,					"0E",		0	},	/* 15 */
	{	2,	2,	0,	1,	0x0E,	0,					"0F",		0	},	/* 16 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},
	{	2,	2,	0,	1,	0x0A,	0,					"10",		0	},	/* 17 */
	{	2,	2,	0,	1,	0x0E,	0,					"11",		0	},	/* 18 */
	{	2,	2,	0,	1,	0x0A,	0,					"12",		0	},	/* 19 */
	{	2,	2,	0,	1,	0x0E,	0,					"13",		0	},	/* 20 */
	{	2,	2,	0,	1,	0x0A,	0,					"14",		0	},	/* 21 */
	{	2,	2,	0,	1,	0x0E,	0,					"15",		0	},	/* 22 */
	{	2,	2,	0,	1,	0x0A,	0,					"16",		0	},	/* 23 */
	{	2,	2,	0,	1,	0x0E,	0,					"17",		0	},	/* 24 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},
	{	2,	2,	0,	1,	0x0A,	0,					"18",		0	},	/* 25 */
	{	2,	2,	0,	1,	0x0E,	0,					"19",		0	},	/* 26 */
	{	2,	2,	0,	1,	0x0A,	0,					"1A",		0	},	/* 27 */
	{	2,	2,	0,	1,	0x0E,	0,					"1B",		0	},	/* 28 */
	{	2,	2,	0,	1,	0x0A,	0,					"1C",		0	},	/* 29 */
	{	2,	2,	0,	1,	0x0E,	0,					"1D",		0	},	/* 30 */
	{	2,	2,	0,	1,	0x0A,	0,					"1E",		0	},	/* 31 */
	{	2,	2,	0,	1,	0x0E,	0,					"1F",		0	},	/* 32 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},	/* 33 */
	{	32,	16,	0,	0,	0x0B,	0,					"Text",		2	},	/* 34 */
};

COLUMN_DESC *ptrDumpColumn[38] =
{
	&colDumpDescs[0],  &colDumpDescs[1],  &colDumpDescs[2],  &colDumpDescs[3],  &colDumpDescs[4],  &colDumpDescs[5],
	&colDumpDescs[6],  &colDumpDescs[7],  &colDumpDescs[8],  &colDumpDescs[9],  &colDumpDescs[10], &colDumpDescs[11],
	&colDumpDescs[12], &colDumpDescs[13], &colDumpDescs[14], &colDumpDescs[15], &colDumpDescs[16], &colDumpDescs[17],
	&colDumpDescs[18], &colDumpDescs[19], &colDumpDescs[20], &colDumpDescs[21], &colDumpDescs[22], &colDumpDescs[23],
	&colDumpDescs[24], &colDumpDescs[25], &colDumpDescs[26], &colDumpDescs[27], &colDumpDescs[28], &colDumpDescs[29],
	&colDumpDescs[30], &colDumpDescs[31], &colDumpDescs[32], &colDumpDescs[33], &colDumpDescs[34], &colDumpDescs[35],
	&colDumpDescs[36], &colDumpDescs[37]
};

COLUMN_DESC fileDescs[38] =
{
	{	60,	8,	16,	2,	0x07,	0,	"Filename",	1	},	/* 0 */
	{	20,	4,	4,	0,	0x07,	0,	"Size",		0	},	/* 1 */
};

COLUMN_DESC *ptrFileColumn[2] =
{
	&fileDescs[0],  &fileDescs[1]
};

int filesFound = 0;
int displayColour = 0;
int displayQuiet = 0;
int displayWidth = 16;

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
	printf ("    -q . . . . . Quite mode, only dump the hex codes.\n");
	printf ("    -w <size>  . Set the display size (8, 16, 24 or 32).\n");
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

	displayGetWindowSize ();

	width = displayGetWidth();
	displayWidth = (width < 77 ? 8 : width < 110 ? 16 : width < 143 ? 24 : 32);

	while ((i = getopt(argc, argv, "Cqw:?")) != -1)
	{
		switch (i) 
		{
		case 'C':
			displayColour = DISPLAY_COLOURS;
			break;
			
		case 'q':
			displayQuiet ^= 1;
			break;

		case 'w':
			width = atoi (optarg) / 8;
			if (width >= 1 && width <= 4)
			{
				displayWidth = width * 8;
				break;
			}
		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

    for (; optind < argc; ++optind)
    {
		found += directoryLoad (argv[optind], ONLYFILES, fileCompare, &fileList);
	}

    /*------------------------------------------------------------------------*
     * Now we can sort the directory.                                         *
     *------------------------------------------------------------------------*/
	directorySort (&fileList);

	if (found)
	{
	    /*--------------------------------------------------------------------*
	     * Now we can sort the directory.                                     *
	     *--------------------------------------------------------------------*/
		directorySort (&fileList);
		directoryProcess (showDir, &fileList);

		if (!displayQuiet) 
		{
			if (!displayColumnInit (2, ptrFileColumn, displayColour))
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
    /*------------------------------------------------------------------------*
     * The buffer size must be a multiple of 8,16,24 and 32.                  *
     *------------------------------------------------------------------------*/
	unsigned char inBuffer[1536 + 1], inFile[PATH_SIZE];
	unsigned char saveHex[4], saveChar[80];
	FILE *readFile;
	int j = 0, read, filePosn = 0, l = 0;

	if (!displayColumnInit (2, ptrFileColumn, displayColour))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}
	if (!displayQuiet) 
	{
		displayDrawLine (0);
		displayHeading (0);
		displayNewLine(0);
		displayInColumn (0, "%s", file -> fileName);
		displayInColumn (1,	displayFileSize (file -> fileStat.st_size, (char *)inBuffer));
		displayNewLine(DISPLAY_INFO);
		displayAllLines ();		
	}
	displayTidy ();

	strcpy ((char *)inFile, file -> fullPath);
	strcat ((char *)inFile, file -> fileName);

	if ((readFile = fopen ((char *)inFile, "rb")) != NULL)
	{
		if (!displayColumnInit (38, ptrDumpColumn, displayColour))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 0;
		}

		if (!displayQuiet) displayDrawLine (0);
		if (!displayQuiet) displayHeading (0);
		
		while ((read = fread (inBuffer, 1, 1536, readFile)) != 0)
		{
			int i = 0, c = 1;
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
						displayInColumn (36, " ", saveChar);
						displayInColumn (37, "%s", saveChar);
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
		}
		if (j)
		{
			if (!displayQuiet) 
			{
				displayInColumn (0, "%08X", filePosn);
				displayInColumn (36, " ", saveChar);
				displayInColumn (37, "%s", saveChar);
			}
			displayNewLine(0);
		}
		displayAllLines ();
		displayTidy ();
		
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
