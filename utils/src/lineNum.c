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

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
COLUMN_DESC colNumberDescs[3] =
{
	{ 20,	1,	2,	2,	0x02,	COL_ALIGN_RIGHT,	"Line",			0 },	/* 0 */
	{ 1024, 10, 10, 0,	0x07,	0,					"Contents",		1 }		/* 1 */
};

COLUMN_DESC *ptrNumberColumn[3] =
{
	&colNumberDescs[0],
	&colNumberDescs[1]
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
	printf ("TheKnight: Show Line Numbers for text files, Version %s\n", directoryVersion());
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
	printf ("     -b . . . Do not count blank lines.\n");
	printf ("     -C . . . Display output in colour.\n");
	printf ("     -q . . . Quiet mode, only show file contents.\n");
	printf ("     -eN  . . Set the ending line number.\n");
	printf ("     -sN  . . Set the starting line number.\n");
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

	if (strcmp (directoryVersion(), VERSION) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
		exit (1);
	}

	displayInit ();
	displayGetWidth();

	while ((i = getopt(argc, argv, "bCqe:s:t:?")) != -1)
	{
		int t;

		switch (i)
		{
		case 'b':
			showBlank = 0;
			break;
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
		case 't':
			t = atoi (optarg);
			if (t > 0 && t <= 32)
			{
				tabSize = t;
			}
			break;
		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], ONLYFILES|ONLYLINKS, fileCompare, &fileList);
	}

	/*------------------------------------------------------------------------*/
	/* Now we can sort the directory.                                         */
	/*------------------------------------------------------------------------*/
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
		int linesFound = 0, terminated = 1;

		if (!displayColumnInit (2, ptrNumberColumn, displayColour))
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
			int inPos = 0, outPos = 0, curPosn = 0, nextPosn = 0, blankLine = 0;

			if (terminated)
			{
				++linesFound;
				terminated = 0;
			}
			while (inBuffer[inPos] && outPos < (8 * INBUFF_SIZE))
			{
				if (inBuffer[inPos] == ' ')
				{
					++nextPosn;
				}
				else if (inBuffer[inPos] == '\t')
				{
					nextPosn = ((nextPosn + tabSize) / tabSize) * tabSize;
				}
				else
				{
					while (curPosn < nextPosn && inBuffer[inPos] != '\n')
					{
						outBuffer[outPos++] = ' ';
						++curPosn;
					}
					if (inBuffer[inPos] >= ' ' && inBuffer[inPos] < 127)
					{
						outBuffer[outPos++] = inBuffer[inPos];
					}
					else if (inBuffer[inPos] < ' ' && inBuffer[inPos] != '\n')
					{
						outBuffer[outPos++] = '^';
						outBuffer[outPos++] = (inBuffer[inPos] + 'A');
					}
					else if (inBuffer[inPos] == '\n')
					{
						terminated = 1;
						if (showBlank)
							blankLine = 1;
					}
					nextPosn = ++curPosn;
				}
				++inPos;
			}
			if (outPos || blankLine)
			{
				if (linesFound >= startLine && linesFound <= endLine)
				{
					outBuffer[outPos] = 0;
					displayInColumn (0, "%d", linesFound);
					displayInColumn (1, "%s", outBuffer);
					displayNewLine (0);
					++linesShown;
				}
			}
		}
		fclose (readFile);
		displayAllLines ();
		displayTidy ();
	}
	return linesShown ? 1 : 0;
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
 *  \result 0, 1 or -1 depending on compare.
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

