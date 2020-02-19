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
#include "config.h"
#define _GNU_SOURCE
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/fcntl.h>
#include <getopt.h>
#ifdef HAVE_VALUES_H
#include <values.h>
#else
#define MAXINT 2147483647
#endif

#include <dircmd.h>
#include "buildDate.h"

/**********************************************************************************************************************
 * Defines                                                                                                            *
 **********************************************************************************************************************/
#define INBUFF_SIZE		4096
#define MAX_COL			40
#define MASK_COLS		0
#define MASK_ROWS		1
#define MASK_RIGHT		2
#define MASK_COUNT		3
#define MASK_SIZE		1024

/**********************************************************************************************************************
 * Prototypes                                                                                                         *
 **********************************************************************************************************************/
int showDir (DIR_ENTRY *file);
void showStdIn (void);

/**********************************************************************************************************************
 * Global                                                                                                             *
 **********************************************************************************************************************/
COLUMN_DESC **ptrNumberColumn;

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
int removeSpace = 1;
int lineHeading = 0;
char separator = ',';
int bitMaskSet[MASK_COUNT];
unsigned int bitMask[MASK_COUNT][MASK_SIZE];

/**********************************************************************************************************************
 * Command line params                                                                                                *
 **********************************************************************************************************************/
static struct option longOptions[] =
{
	{	"align",		required_argument,	0,	'a' },
	{	"colour",		no_argument,		0,	'C' },
	{	"incol",		no_argument,		0,	'n' },
	{	"outcol",		no_argument,		0,	'N' },
	{	"pages",		no_argument,		0,	'P' },
	{	"headers",		no_argument,		0,	'h' },
	{	"quiet",		no_argument,		0,	'q' },
	{	"white",		no_argument,		0,	'w' },
	{	"columns",		required_argument,	0,	'c' },
	{	"rows",			required_argument,	0,	'r' },
	{	"separator",	required_argument,	0,	's' },
	{	0,				0,					0,	0	}
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  B I T  M A S K                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set a bit can be in the range 0 to 511.
 *  \param bitSet Which bit set to use, rows or columns.
 *  \param bit Bit to be set.
 *  \result None.
 */
int setBitMask (int bitSet, int bit)
{
	unsigned int mask = 1, maskNum = bit >> 5, bitNum = bit & 0x1F;

	if (maskNum < MASK_SIZE && bitSet < MASK_SIZE)
	{
		bitMask[bitSet][maskNum] |= (mask << bitNum);
		bitMaskSet[bitSet] = 1;
		return 1;
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  B I T  M A S K                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get whether a bit is set.
 *  \param bitSet Which bit set to use, rows or columns.
 *  \param bit Bit to check.
 *  \result 1 if it is set, 0 if not.
 */
int getBitMask (int bitSet, int bit, int def)
{
	if (bitMaskSet[bitSet])
	{
		unsigned int mask = 1, maskNum = bit >> 5, bitNum = bit & 0x1F;

		if (maskNum < MASK_SIZE && bitSet < MASK_SIZE)
		{
			return (bitMask[bitSet][maskNum] & (mask << bitNum) ? 1 : 0);
		}
		return 0;
	}
	return def;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A L L O C  T A B L E                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Allocate the table column descripters.
 *  \result Number of columns.
 */
int allocTable ()
{
	ptrNumberColumn = (COLUMN_DESC **) malloc ((MAX_COL + 1) * sizeof (COLUMN_DESC *));

	if (ptrNumberColumn != NULL)
	{
		int i, c = 0;

		memset (ptrNumberColumn, 0, (MAX_COL + 1) * sizeof (COLUMN_DESC *));
		for (i = 0; i < MAX_COL; ++i)
		{
			ptrNumberColumn[i] = (COLUMN_DESC *)malloc (sizeof (COLUMN_DESC));
			if (ptrNumberColumn[i] != NULL)
			{
				ptrNumberColumn[i] -> maxWidth = 81;
				ptrNumberColumn[i] -> minWidth = 2;
				ptrNumberColumn[i] -> startWidth = 0 ;
				ptrNumberColumn[i] -> gap = 2;
				ptrNumberColumn[i] -> colour = (i & 1 ? 0x06 : 0x02);
				ptrNumberColumn[i] -> attrib = (getBitMask (MASK_RIGHT, i, 0) ? COL_ALIGN_RIGHT : 0);
				ptrNumberColumn[i] -> priority = i;
				ptrNumberColumn[i] -> heading = (char *)malloc (21);
				sprintf (ptrNumberColumn[i] -> heading, "%d", i + 1);
				++c;
			}
		}
		ptrNumberColumn[c] = NULL;
		return c;
	}
	return 0;
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
	printf ("TheKnight: Display separated file, Version: %s, Built: %s\n", directoryVersion(), buildDate);
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
 *  \param bitSet Which bit set to use, rows or columns.
 *  \param value A number string that could be (1) (1-3) (3-) (-3) (1,2,3).
 *  \result None.
 */
void procNumberRange (int bitSet, char *value)
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
			/* Got here with 1-2 or -2 */
			if (start && end && range)
			{
				int l;
				for (l = start; l <= end; ++l)
				{
					setBitMask (bitSet, l - 1);
				}
			}
			/* Got here with 1- */
			else if (start && range)
			{
				int l = start;
				while (setBitMask (bitSet, l - 1))
				{
					++l;
				}
			}
			/* Got here with 1 */
			else if (start)
			{
				setBitMask (bitSet, start - 1);
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
	printf ("     --align # . . . . . -a#  . . . Right align a column.\n");
	printf ("     --colour  . . . . . -C . . . . Display output in colour.\n");
	printf ("     --incol . . . . . . -n . . . . Show input column numbers in header.\n");
	printf ("     --outcol  . . . . . -N . . . . Show output column numbers in header.\n");
	printf ("     --pages . . . . . . -P . . . . Stop the the end of each page.\n");
	printf ("                                    (Not available when processing stdin)\n");
	printf ("     --headers . . . . . -h . . . . Use first line to generate header.\n");
	printf ("     --quiet . . . . . . -q . . . . Quiet mode, only show file contents.\n");
	printf ("     --white . . . . . . -w . . . . Do not remove whitespace from fields.\n");
	printf ("     --columns # . . . . -c#  . . . Columns to display, [example 1,3-5,7].\n");
	printf ("     --rows #  . . . . . -r#  . . . Rows to display, [example 1,3-5,7].\n");
	printf ("     --separator \"c\" . . -sc  . . . Set separator character [default ,].\n");
	printf ("     --help  . . . . . . -? . . . . Display this help message.\n");
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
	int found = 0;
	char fullVersion[81];

	strcpy (fullVersion, VERSION);
#ifdef USE_STATX
	strcat (fullVersion, ".X");
#endif
	if (strcmp (directoryVersion(), fullVersion) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
		exit (1);
	}

	displayInit ();
	displayGetWidth ();

	while (1)
	{
		/**************************************************************************************************************
		 * getopt_long stores the option index here.                                                                  *
		 **************************************************************************************************************/
		int optionIndex = 0;
		int c = getopt_long (argc, argv, "a:CNPhnqwc:r:s:?", longOptions, &optionIndex);

		/**************************************************************************************************************
		 * Detect the end of the options.                                                                             *
		 **************************************************************************************************************/
		if (c == -1) break;

		switch (c)
		{
		case 'a':
			procNumberRange (MASK_RIGHT, optarg);
			break;

		case 'C':
			displayFlags |= DISPLAY_COLOURS;
			break;

		case 'h':
		case 'n':
			lineHeading = (c == 'h' ? 1 : 2);
		case 'N':
			displayFlags |= DISPLAY_HEADINGS;
			break;

		case 'P':
			displayFlags |= DISPLAY_IN_PAGES;
			break;

		case 'q':
			displayQuiet ^= 1;
			break;

		case 'c':
			procNumberRange (MASK_COLS, optarg);
			break;

		case 'r':
			procNumberRange (MASK_ROWS, optarg);
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

	allocTable ();
	if (optind == argc)
	{
		showStdIn ();
		exit (0);
	}
	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], ONLYFILES|ONLYLINKS, NULL, &fileList);
	}

	if (found)
	{
		/**************************************************************************************************************
		 * Now we can sort the directory.                                                                             *
		 **************************************************************************************************************/
		directorySort (&fileList);
		directoryProcess (showDir, &fileList);

		if (filesFound && !displayQuiet)
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
 *  S H O W  D A T A                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function to display in a column.
 *  \param row Which line are we showing (it coulb be line 1 for the headings).
 *  \param col Which column to display in.
 *  \param outBuffer The buffer to display.
 *  \result None.
 */
void showData (int row, int col, int trueCol, char *outBuffer)
{
	int showLine = 1;

	if (removeSpace)
	{
		rmWhiteSpace (outBuffer);
	}
	if (row == 1 && lineHeading)
	{
		char tempBuff[15];
		char *outText = outBuffer;

		if (ptrNumberColumn[col] -> heading != NULL)
		{
			free (ptrNumberColumn[col] -> heading);
		}
		if (lineHeading == 1)
		{
			showLine = 0;
		}
		else
		{
			sprintf (tempBuff, "%d", trueCol + 1);
			outText = tempBuff;
		}
		if (outText[0])
		{
			ptrNumberColumn[col] -> heading = (char *)malloc (strlen (outText) + 1);
			if (ptrNumberColumn[col] -> heading != NULL)
			{
				strcpy (ptrNumberColumn[col] -> heading, outText);
				displayUpdateHeading (col, ptrNumberColumn[col] -> heading);
			}
		}
	}
	if (outBuffer[0] && showLine)
	{
		displayInColumn (col, "%s", outBuffer);
	}
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

	if (getBitMask (MASK_ROWS, linesRead - 1, 1) || (linesRead == 1 && lineHeading))
	{
		outBuffer[0] = 0;
		while (inBuffer[ipos] != 0 && ocol < MAX_COL && icol < 512)
		{
			if (inBuffer[ipos] >= ' ' || inBuffer[ipos] == separator)
			{
				if (inBuffer[ipos] == separator)
				{
					if (getBitMask (MASK_COLS, icol, 1))
					{
						if (opos)
						{
							showData (linesRead, ocol, icol, outBuffer);
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
		if (getBitMask (MASK_COLS, icol, 1))
		{
			if (opos)
			{
				showData (linesRead, ocol, icol, outBuffer);
				++ocol;
			}
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

	/******************************************************************************************************************
	 * First display a table with the file name and size.                                                             *
	 ******************************************************************************************************************/
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
#ifdef USE_STATX
		displayInColumn (1, displayFileSize (file -> fileStat.stx_size, (char *)inBuffer));
#else
		displayInColumn (1, displayFileSize (file -> fileStat.st_size, (char *)inBuffer));
#endif
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();
	}
	displayTidy ();

	strncpy (inBuffer, file -> fullPath, INBUFF_SIZE);
	strncat (inBuffer, file -> fileName, INBUFF_SIZE - strlen (inBuffer));

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
void showStdIn (void)
{
	char inBuffer[INBUFF_SIZE + 1];
	int linesShown = 0, linesRead = 0;

	/******************************************************************************************************************
	 * First display a table with the file name and size.                                                             *
	 ******************************************************************************************************************/
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

	if (!displayColumnInit (MAX_COL, ptrNumberColumn, (displayFlags & ~DISPLAY_IN_PAGES)))
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
