/**********************************************************************************************************************
 *                                                                                                                    *
 *  M K  T A B L E . C                                                                                                *
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
 *  \brief Display a separated file in columns.
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

#define INBUFF_SIZE		4096
#define MAX_COL			20

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*/
/* Global                                                                    */
/*----------------------------------------------------------------------------*/
COLUMN_DESC colNumberDescs[MAX_COL + 1] =
{
	{ 81,	2,	0,	2,	0x02,	0,	"1",		1 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"2",		2 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"3",		3 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"4",		4 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"5",		5 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"6",		6 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"7",		7 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"8",		8 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"9",		9 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"10",		10 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"11",		11 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"12",		12 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"13",		13 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"14",		14 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"15",		15 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"16",		16 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"17",		17 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"18",		18 },	/* 1 */
	{ 81,	2,	0,	2,	0x02,	0,	"19",		19 },	/* 0 */
	{ 81,	2,	0,	2,	0x06,	0,	"20",		20 },	/* 1 */
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
	{	60, 8,	16, 2,	0x07,	0,	"File name", 1	},	/* 0 */
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
int displayFlags = 0;
int startLine = 1;
int endLine = MAXINT;
int removeSpace = 1;
char separator = ',';
int bitMaskSet;
unsigned int bitMask[16];

void processStdin (void);

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  B I T  M A S K                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set a bit can be in the range 0 to 511.
 *  \param bit Bit to be set.
 *  \result None.
 */
void setBitMask (int bit)
{
	unsigned int mask = 1, maskNum = bit >> 5, bitNum = bit & 0x1F;

	if (maskNum < 16)
	{
		bitMask[maskNum] |= (mask << bitNum);
		bitMaskSet = 1;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  B I T  M A S K                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get whether a bit is set.
 *  \param bit Bit to check.
 *  \result 1 if it is set, 0 if not.
 */
int getBitMask (int bit)
{
	if (bitMaskSet)
	{
		unsigned int mask = 1, maskNum = bit >> 5, bitNum = bit & 0x1F;

		if (maskNum < 16)
		{
			return (bitMask[maskNum] & (mask << bitNum) ? 1 : 0);
		}
		return 0;
	}
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R M  W H I T E  S P A C E                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Remove whitespace from the beginning and end of the field.
 *  \param str String to process.
 *  \result Pointer to the new string.
 */
char *rmWhiteSpace (char *str)
{
	int start = 0, end = 0, loop = 0, out = 0;

	while (str[loop] <= ' ' && str[loop] != 0)
	{
		++loop;
	}
	start = loop;
	while (str[loop] != 0)
	{
		if (str[loop] > ' ')
		{
			end = loop;
		}
		++loop;
	}
	for (loop = start; loop <= end; ++loop)
	{
		str[out++] = str[loop];
	}
	str[out] = 0;
	return str;
}

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
	printf ("TheKnight: Display separated file, Version %s\n", directoryVersion());
	displayLine ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C  N U M B E R  R A N G E                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Allow you to input column number ranges and comma separated lists.
 *  \param value A number string that could be (1) (1-3) (1,2,3).
 *  \result None.
 */
void procNumberRange (char *value)
{
	int start = 0, end = 0, range = 0, ipos = 0, done = 0;

	while (!done)
	{
		if (value[ipos] >= '0' && value[ipos] <= '9')
		{
			if (range)
			{
				end = (end * 10) + (value[ipos] - '0');
			}
			else
			{
				start = (start * 10) + (value[ipos] - '0');
			}
		}
		else if (value[ipos] == '-')
		{
			if (range == 0)
			{
				if (start == 0)
				{
					start = 1;
				}
				range = 1;
			}
			else
			{
				start = end = range = 0;
			}
		}
		else if (value[ipos] == ',' || value[ipos] == 0)
		{
			if (start && end && range)
			{
				int l;
				for (l = start; l <= end; ++l)
				{
					setBitMask (l - 1);
				}
			}
			else if (start)
			{
				setBitMask (start - 1);
			}
			start = end = range = 0;
			if (value[ipos] == 0)
			{
				done = 1;
			}
		}
		else
		{
			start = end = range = 0;
		}
		++ipos;
	}
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
	printf ("Enter the command: %s [options] <file name>\n", basename(name));
	printf ("Options: \n");
	printf ("     -C . . . . . Display output in colour.\n");
	printf ("     -q . . . . . Quiet mode, only show file contents.\n");
	printf ("     -w . . . . . Do not remove whitespace from fields.\n");
	printf ("     -dN  . . . . Columns to display, [example 1,3-5,7].\n");
	printf ("     -sC  . . . . Set separator character [default ,].\n");
	printf ("     -bN  . . . . Set the beginning line number.\n");
	printf ("     -eN  . . . . Set the ending line number.\n");
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

	while ((i = getopt(argc, argv, "CHqwe:b:s:d:?")) != -1)
	{
		int t;

		switch (i)
		{
		case 'C':
			displayFlags |= DISPLAY_COLOURS;
			break;

		case 'H':
			displayFlags |= DISPLAY_HEADINGS;
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

		case 'b':
			t = atoi (optarg);
			if (t > 0 && t <= endLine)
			{
				startLine = t;
			}
			break;

		case 'd':
			procNumberRange (optarg);
			break;

		case 's':
			{
				int chr = 0;
				char lastChar = 0;
				while (optarg[chr])
				{
					if (lastChar == '\\')
					{
						switch (optarg[chr])
						{
						case '\\':
							separator = '\\';
							break;
						case 'b':
							separator = '\b';
							break;
						case 'f':
							separator = '\f';
							break;
						case 'a':
							separator = '\a';
							break;
						case 't':
							separator = '\t';
							break;
						case 'r':
							separator = '\r';
							break;
						case 'n':
							separator = '\n';
							break;
						case '\'':
							separator = '\'';
							break;
						case '\"':
							separator = '\"';
							break;
						case ' ':
							separator = ' ';
							break;
						}
						lastChar = 0;
					}
					else if (optarg[chr] >= ' ')
					{
						separator = optarg[chr];
						lastChar = optarg[chr];
					}
					else
					{
						lastChar = 0;
					}
					++chr;
				}
			}
			break;

		case 'w':
			removeSpace = 0;
			break;

		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	if (optind == argc)
	{
		processStdin ();
		exit (0);
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
			if (!displayColumnInit (2, ptrFileColumn, displayFlags & DISPLAY_COLOURS))
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
 *  S H O W  L I N E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Show a line read from a file or from stdin.
 *  \param inBuffer String read.
 *  \param linesRead Number of lines read so far.
 *  \result 1 if any output shown.
 */
int showLine (char *inBuffer, int linesRead)
{
	char outBuffer[INBUFF_SIZE + 1];
	int ipos = 0, opos = 0, icol = 0, ocol = 0, shown = 0;

	if (linesRead >= startLine && linesRead <= endLine)
	{
		outBuffer[0] = 0;
		while (inBuffer[ipos] != 0 && ocol < MAX_COL && icol < 512)
		{
			if (inBuffer[ipos] >= ' ' || inBuffer[ipos] == separator)
			{
				if (inBuffer[ipos] == separator)
				{
					if (getBitMask (icol))
					{
						if (opos)
						{
							if (removeSpace)
							{
								rmWhiteSpace (outBuffer);
							}
							if (outBuffer[0])
							{
								displayInColumn (ocol, "%s", outBuffer);
							}
						}
						++ocol;
					}
					outBuffer[opos = 0] = 0;
					++icol;
				}
				else
				{
					outBuffer[opos++] = inBuffer[ipos];
					outBuffer[opos] = 0;
				}
			}
			++ipos;
		}
		if (opos)
		{
			if (getBitMask (icol))
			{
				if (removeSpace)
				{
					rmWhiteSpace (outBuffer);
				}
				if (outBuffer[0])
				{
					displayInColumn (ocol, "%s", outBuffer);
				}
				++ocol;
			}
			outBuffer[opos = 0] = 0;
		}
		if (ocol)
		{
			displayNewLine (0);
			shown = 1;
		}
	}
	return shown;
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
	char inBuffer[INBUFF_SIZE + 1];
	int linesShown = 0, linesRead = 0;
	FILE *readFile;

	/*------------------------------------------------------------------------*/
	/* First display a table with the file name and size.                     */
	/*------------------------------------------------------------------------*/
	if (!displayColumnInit (2, ptrFileColumn, displayFlags & DISPLAY_COLOURS))
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
		if (!displayColumnInit (MAX_COL, ptrNumberColumn, displayFlags))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 0;
		}
		if (!displayQuiet && !(displayFlags & DISPLAY_HEADINGS))
		{
			displayDrawLine (0);
		}
		while (fgets (inBuffer, INBUFF_SIZE, readFile) != NULL)
		{
			++linesRead;
			if (showLine (inBuffer, ++linesRead))
			{
				++linesShown;
			}
		}
		fclose (readFile);
		displayAllLines ();
		displayTidy ();
	}
	if (linesShown)
	{
		++filesFound;
		return 1;
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  S T D I N                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the input from stdin.
 *  \result None.
 */
void processStdin (void)
{
	char inBuffer[INBUFF_SIZE + 1];
	int linesShown = 0, linesRead = 0;

	/*------------------------------------------------------------------------*/
	/* First display a table with the file name and size.                     */
	/*------------------------------------------------------------------------*/
	if (!displayColumnInit (1, ptrFileColumn, displayFlags & DISPLAY_COLOURS))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return;
	}
	if (!displayQuiet)
	{
		displayDrawLine (0);
		displayHeading (0);
		displayNewLine (0);
		displayInColumn (0, "stdin");
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();
	}
	displayTidy ();

	if (!displayColumnInit (MAX_COL, ptrNumberColumn, displayFlags))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return;
	}
	if (!displayQuiet && !(displayFlags & DISPLAY_HEADINGS))
	{
		displayDrawLine (0);
	}
	while (fgets (inBuffer, INBUFF_SIZE, stdin) != NULL)
	{
		++linesRead;
		if (showLine (inBuffer, ++linesRead))
		{
			++linesShown;
		}
	}
	if (linesShown)
	{
		displayDrawLine (0);
	}
	displayAllLines ();
	displayTidy ();
}
