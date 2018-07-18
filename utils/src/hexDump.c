/**********************************************************************************************************************
 *                                                                                                                    *
 *  H E X  D U M P . C                                                                                                *
 *  ==================                                                                                                *
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
 *  \brief Program to dump the contence of a file in Hex.
 */
#include <config.h>
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
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 * 00 00 00 00 00 00 00 00-08 13 50 00 00 00 00 00 : ..........P.....         *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colDumpDescs[] =
{
	{	8,	6,	0,	2,	0x07,	0,					"Offset",	1	},	/* 0    0  */
	{	2,	2,	0,	1,	0x0A,	0,					"00",		0	},	/* 1    1  */
	{	2,	2,	0,	1,	0x0E,	0,					"01",		0	},	/* 2    2  */
	{	2,	2,	0,	1,	0x0A,	0,					"02",		0	},	/* 3    3  */
	{	2,	2,	0,	1,	0x0E,	0,					"03",		0	},	/* 4    4  */
	{	2,	2,	0,	1,	0x0A,	0,					"04",		0	},	/* 5    5  */
	{	2,	2,	0,	1,	0x0E,	0,					"05",		0	},	/* 6    6  */
	{	2,	2,	0,	1,	0x0A,	0,					"06",		0	},	/* 7    7  */
	{	2,	2,	0,	1,	0x0E,	0,					"07",		0	},	/* 8    8  */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},	/*      9  */
	{	2,	2,	0,	1,	0x0A,	0,					"08",		0	},	/* 9    10 */
	{	2,	2,	0,	1,	0x0E,	0,					"09",		0	},	/* 10   11 */
	{	2,	2,	0,	1,	0x0A,	0,					"0A",		0	},	/* 11   12 */
	{	2,	2,	0,	1,	0x0E,	0,					"0B",		0	},	/* 12   13 */
	{	2,	2,	0,	1,	0x0A,	0,					"0C",		0	},	/* 13   14 */
	{	2,	2,	0,	1,	0x0E,	0,					"0D",		0	},	/* 14   15 */
	{	2,	2,	0,	1,	0x0A,	0,					"0E",		0	},	/* 15   16 */
	{	2,	2,	0,	1,	0x0E,	0,					"0F",		0	},	/* 16   17 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},	/*      18 */
	{	2,	2,	0,	1,	0x0A,	0,					"10",		0	},	/* 17   19 */
	{	2,	2,	0,	1,	0x0E,	0,					"11",		0	},	/* 18   20 */
	{	2,	2,	0,	1,	0x0A,	0,					"12",		0	},	/* 19   21 */
	{	2,	2,	0,	1,	0x0E,	0,					"13",		0	},	/* 20   22 */
	{	2,	2,	0,	1,	0x0A,	0,					"14",		0	},	/* 21   23 */
	{	2,	2,	0,	1,	0x0E,	0,					"15",		0	},	/* 22   24 */
	{	2,	2,	0,	1,	0x0A,	0,					"16",		0	},	/* 23   25 */
	{	2,	2,	0,	1,	0x0E,	0,					"17",		0	},	/* 24   26 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},	/*      27 */
	{	2,	2,	0,	1,	0x0A,	0,					"18",		0	},	/* 25   28 */
	{	2,	2,	0,	1,	0x0E,	0,					"19",		0	},	/* 26   29 */
	{	2,	2,	0,	1,	0x0A,	0,					"1A",		0	},	/* 27   30 */
	{	2,	2,	0,	1,	0x0E,	0,					"1B",		0	},	/* 28   31 */
	{	2,	2,	0,	1,	0x0A,	0,					"1C",		0	},	/* 29   32 */
	{	2,	2,	0,	1,	0x0E,	0,					"1D",		0	},	/* 30   33 */
	{	2,	2,	0,	1,	0x0A,	0,					"1E",		0	},	/* 31   34 */
	{	2,	2,	0,	1,	0x0E,	0,					"1F",		0	},	/* 32   35 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},	/*      36 */
	{	2,	2,	0,	1,	0x0A,	0,					"20",		0	},	/* 33   37 */
	{	2,	2,	0,	1,	0x0E,	0,					"21",		0	},	/* 34   38 */
	{	2,	2,	0,	1,	0x0A,	0,					"22",		0	},	/* 35   39 */
	{	2,	2,	0,	1,	0x0E,	0,					"23",		0	},	/* 36   40 */
	{	2,	2,	0,	1,	0x0A,	0,					"24",		0	},	/* 37   41 */
	{	2,	2,	0,	1,	0x0E,	0,					"25",		0	},	/* 38   42 */
	{	2,	2,	0,	1,	0x0A,	0,					"26",		0	},	/* 39   43 */
	{	2,	2,	0,	1,	0x0E,	0,					"27",		0	},	/* 40   44 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},	/*      45 */
	{	2,	2,	0,	1,	0x0A,	0,					"28",		0	},	/* 41   46 */
	{	2,	2,	0,	1,	0x0E,	0,					"29",		0	},	/* 42   47 */
	{	2,	2,	0,	1,	0x0A,	0,					"2A",		0	},	/* 43   48 */
	{	2,	2,	0,	1,	0x0E,	0,					"2B",		0	},	/* 44   49 */
	{	2,	2,	0,	1,	0x0A,	0,					"2C",		0	},	/* 45   50 */
	{	2,	2,	0,	1,	0x0E,	0,					"2D",		0	},	/* 46   51 */
	{	2,	2,	0,	1,	0x0A,	0,					"2E",		0	},	/* 47   52 */
	{	2,	2,	0,	1,	0x0E,	0,					"2F",		0	},	/* 48   53 */
	{	1,	1,	0,	0,	0x0B,	0,					"",			0	},	/*      54 */
	{	32, 16, 0,	0,	0x07,	0,					"Text",		2	},	/* 49   55 */
};

COLUMN_DESC *ptrDumpColumn[] =
{
	&colDumpDescs[0],  &colDumpDescs[1],  &colDumpDescs[2],	 &colDumpDescs[3],	&colDumpDescs[4],  &colDumpDescs[5],
	&colDumpDescs[6],  &colDumpDescs[7],  &colDumpDescs[8],	 &colDumpDescs[9],	&colDumpDescs[10], &colDumpDescs[11],
	&colDumpDescs[12], &colDumpDescs[13], &colDumpDescs[14], &colDumpDescs[15], &colDumpDescs[16], &colDumpDescs[17],
	&colDumpDescs[18], &colDumpDescs[19], &colDumpDescs[20], &colDumpDescs[21], &colDumpDescs[22], &colDumpDescs[23],
	&colDumpDescs[24], &colDumpDescs[25], &colDumpDescs[26], &colDumpDescs[27], &colDumpDescs[28], &colDumpDescs[29],
	&colDumpDescs[30], &colDumpDescs[31], &colDumpDescs[32], &colDumpDescs[33], &colDumpDescs[34], &colDumpDescs[35],
	&colDumpDescs[36], &colDumpDescs[37], &colDumpDescs[38], &colDumpDescs[39], &colDumpDescs[40], &colDumpDescs[41],
	&colDumpDescs[42], &colDumpDescs[43], &colDumpDescs[44], &colDumpDescs[45], &colDumpDescs[46], &colDumpDescs[47],
	&colDumpDescs[48], &colDumpDescs[49], &colDumpDescs[50], &colDumpDescs[51], &colDumpDescs[52], &colDumpDescs[53],
	&colDumpDescs[54], &colDumpDescs[55]
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

int filesFound = 0;
int displayFlags = 0;
int displayQuiet = 0;
int displayWidth = -1;

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
	printf ("    -P . . . . . Display output in pages.\n");
	printf ("    -q . . . . . Quite mode, only dump the hex codes.\n");
	printf ("    -w <size>  . Set the display size (8, 16, 24, 32, 40 or 48).\n");
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

	if (strcmp (directoryVersion(), VERSION) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
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
			if (width == 8 || width == 16 || width == 24 || width == 32 || width == 40 || width == 48)
			{
				displayWidth = width;
				break;
			}
		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], ONLYFILES|ONLYLINKS, NULL, &fileList);
	}

	if (found)
	{
		if (displayWidth == -1)
		{
			displayWidth = displayQuiet ?
					(width < 48 ? 8 : width <  72 ? 16 : width < 96	 ? 24 : width < 120 ? 32 : width < 144 ? 40 : 48):
					(width < 77 ? 8 : width < 110 ? 16 : width < 143 ? 24 : width < 176 ? 32 : width < 209 ? 40 : 48);
		}

		/*--------------------------------------------------------------------*
         * Now we can sort the directory.                                     *
         *--------------------------------------------------------------------*/
		directorySort (&fileList);
		directoryProcess (showDir, &fileList);

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
	unsigned char inBuffer[2048 + 1], inFile[PATH_SIZE];
	unsigned char saveHex[4], saveChar[80];
	FILE *readFile;
	int j = 0, c = 1, read, filePosn = 0, l = 0;

	/*------------------------------------------------------------------------*
     * If the file is a link check it points to a regular file.               *
     *------------------------------------------------------------------------*/
	if (S_ISLNK (file -> fileStat.st_mode))
	{
		mode_t type = directoryTrueLinkType (file);
		if (!S_ISREG (type))
		{
			fprintf (stderr, "ERROR: Link does not point to file\n");
			return 0;
		}
	}

	/*------------------------------------------------------------------------*
     * First display a table with the file name and size.                     *
     *------------------------------------------------------------------------*/
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
		displayInColumn (1, displayFileSize (file -> fileStat.st_size, (char *)inBuffer));
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();
	}
	displayTidy ();

	/*------------------------------------------------------------------------*
     * Open the file and display a table containing the hex dump.             *
     *------------------------------------------------------------------------*/
	strcpy ((char *)inFile, file -> fullPath);
	strcat ((char *)inFile, file -> fileName);

	if ((readFile = fopen ((char *)inFile, "rb")) != NULL)
	{
		if (!displayColumnInit (56, ptrDumpColumn, displayFlags))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 0;
		}

		if (!displayQuiet) displayDrawLine (0);
		if (!displayQuiet) displayHeading (0);

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
						displayInColumn (54, " ", saveChar);
						displayInColumn (55, "%s", saveChar);
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
				displayInColumn (54, " ", saveChar);
				displayInColumn (55, "%s", saveChar);
			}
			displayNewLine (0);
		}
		displayAllLines ();
		displayTidy ();

		fclose (readFile);
		++filesFound;
	}
	return 0;
}

