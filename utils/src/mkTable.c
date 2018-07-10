/**********************************************************************************************************************
 *                                                                                                                    *
 *  L I N E  N U M . C                                                                                                *
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
 *  \brief Display a text file with line numbers.
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dircmd.h>
#include <libgen.h>
#ifdef HAVE_VALUES_H
#include <values.h>
#else
#define MAXINT 2147483647
#endif

#define INBUFF_SIZE		2048
#define MAX_COL			20

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
COLUMN_DESC colNumberDescs[MAX_COL + 1] =
{
	{ 81,	5,	2,	2,	0x02,	0,	"Col1",		1 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col2",		2 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col3",		3 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col4",		4 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col5",		5 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col6",		6 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col7",		7 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col8",		8 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col9",		9 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col10",	10 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col11",	11 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col12",	12 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col13",	13 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col14",	14 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col15",	15 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col16",	16 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col17",	17 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col18",	18 },	/* 1 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col19",	19 },	/* 0 */
	{ 81,	5,	2,	2,	0x02,	0,	"Col20",	20 },	/* 1 */
};

COLUMN_DESC *ptrNumberColumn[MAX_COL + 1] =
{
	&colNumberDescs[0], &colNumberDescs[1], &colNumberDescs[2], &colNumberDescs[3], &colNumberDescs[4],
	&colNumberDescs[5], &colNumberDescs[6], &colNumberDescs[7], &colNumberDescs[8], &colNumberDescs[9],
	&colNumberDescs[10], &colNumberDescs[11], &colNumberDescs[12], &colNumberDescs[13], &colNumberDescs[14],
	&colNumberDescs[15], &colNumberDescs[16], &colNumberDescs[17], &colNumberDescs[18], &colNumberDescs[19]
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

int tabSize = 8;
int showBlank = 1;
int filesFound = 0;
int totalLines = 0;
int displayQuiet = 0;
int displayColour = 0;
int startLine = 1;
int endLine = MAXINT;
char separator = ',';

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
	printf ("TheKnight: , Version %s\n", directoryVersion());
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
	printf ("     -C . . . Display output in colour.\n");
	printf ("     -q . . . Quiet mode, only show file contents.\n");
	printf ("     -eN  . . Set the ending line number.\n");
	printf ("     -sN  . . Set the starting line number.\n");
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

	if (strcmp (directoryVersion(), VERSION) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
		exit (1);
	}

	displayInit ();
	displayGetWidth();

	while ((i = getopt(argc, argv, "Cqe:s:?")) != -1)
	{
		int t;

		switch (i)
		{
		case 'C':
			displayColour = DISPLAY_COLOURS;
			break;
		case 'q':
			displayQuiet ^= 1;
			break;
		case 'e':
			t = atoi (optarg);
			if (t >= startLine)
			{
				endLine = t;
			}
			break;
		case 's':
			t = atoi (optarg);
			if (t > 0 && t <= endLine)
			{
				startLine = t;
			}
			break;
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
		/*--------------------------------------------------------------------*/
		/* Now we can sort the directory.                                     */
		/*--------------------------------------------------------------------*/
		directorySort (&fileList);
		directoryProcess (showDir, &fileList);

		if (filesFound)
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
 *  \brief Convert the file that was found.
 *  \param file File to convert.
 *  \result 1 if file changed.
 */
int showDir (DIR_ENTRY *file)
{
	char inBuffer[INBUFF_SIZE + 1], outBuffer[(8 * INBUFF_SIZE) + 1];
	int linesShown = 0;
	FILE *readFile;

	/*------------------------------------------------------------------------*/
	/* First display a table with the file name and size.                     */
	/*------------------------------------------------------------------------*/
	if (!displayColumnInit (2, ptrFileColumn, displayColour))
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

	strcpy (inBuffer, file -> fullPath);
	strcat (inBuffer, file -> fileName);

	if ((readFile = fopen (inBuffer, "rb")) != NULL)
	{
		if (!displayColumnInit (10, ptrNumberColumn, displayColour))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 0;
		}
		if (!displayQuiet)
		{
			displayDrawLine (0);
		}
		while (fgets (inBuffer, INBUFF_SIZE, readFile) != NULL)
		{
			int i = 0, j = 0, c = 0;

			outBuffer[0] = 0;
			while (inBuffer[i] != 0 && c < 20)
			{
				if (inBuffer[i] >= ' ')
				{
					if (inBuffer[i] == separator)
					{
						if (j)
						{
							displayInColumn (c++, "%s", outBuffer);
							outBuffer[j = 0] = 0;
						}
					}
					else
					{
						outBuffer[j++] = inBuffer[i];
						outBuffer[j] = 0;
					}
				}
				++i;
			}
			if (j)
			{
				displayInColumn (c++, "%s", outBuffer);
				outBuffer[j = 0] = 0;
			}
			if (c)
			{
				displayNewLine (0);
			}
		}
		fclose (readFile);
		displayAllLines ();
		displayTidy ();
	}
	return linesShown ? 1 : 0;
}

