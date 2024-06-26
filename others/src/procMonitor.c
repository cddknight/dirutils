/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C  M O N I T O R . C                                                                                        *
 *  ==========================                                                                                        *
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
 *  \brief Program to process daily weather monitor files in to monthly overview.
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

/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colChangeDescs[] =
{
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Max",		1	},	/* 0 */
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Min",		1	},	/* 1 */
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Pressure", 1	},	/* 2 */
	{	160,12, 0,	3,	0x07,	0,					"Filename", 0	},	/* 3 */
	{	10, 10, 0,	3,	0x07,	COL_ALIGN_RIGHT,	"Average",	1	},	/* 4 */
};

COLUMN_DESC *ptrChangeColumn[] =
{
	&colChangeDescs[0],
	&colChangeDescs[1],
	&colChangeDescs[2],
	&colChangeDescs[3]
};

int inputMode = 0;
int outputAll = 0;
int outputMode = 0;
int filesFound = 0;
int totalProcessed = 0;
float savedValues[41];
int savedRead[41];
char linePrefix[41];

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
	printf ("TheKnight: Process lines in monitor logs, Version %s\n", directoryVersion());
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
	char fullVersion[81];

	strcpy (fullVersion, VERSION);
#ifdef USE_STATX
	strcat (fullVersion, ".1");
#else
	strcat (fullVersion, ".0");
#endif
	if (strcmp (directoryVersion(), fullVersion) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
		exit (1);
	}

	displayInit ();

	if (argc == 1)
	{
		version ();
		printf ("Enter the command: %s [-<mode>] <file pattern>\n", argv[0]);
		exit (1);
	}
	/*------------------------------------------------------------------------*
     * If we got a path then split it into a path and a file pattern to match *
     * files with.                                                            *
     *------------------------------------------------------------------------*/
	while ((i = getopt(argc, argv, "apwtT?")) != -1)
	{
		switch (i)
		{
		case 'a':
			outputAll = 1;
			break;

		case 't':
			outputMode = 2;
			break;

		case 'T':
			outputMode = 1;
			break;

		case 'w':
			ptrChangeColumn[2] = &colChangeDescs[2];
			inputMode = 0;
			break;

		case 'p':
			ptrChangeColumn[2] = &colChangeDescs[4];
			inputMode = 1;
			break;

		case '?':
			printf ("%s -[options] <file names>\n\n", basename (argv[0]));
			printf ("        -w  Process weather logs (default).\n");
			printf ("        -p  Process power logs.\n");
			printf ("        -t  Tab separated output (days).\n");
			printf ("        -T  Tab separated output (months).\n");
			return (1);
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

		if (!displayColumnInit (4, ptrChangeColumn, DISPLAY_HEADINGS))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 1;
		}
		directoryProcess (showDir, &fileList);

		displayDrawLine (0);
		displayInColumn (2, displayCommaNumber (totalProcessed, numBuff));
		displayInColumn (3, "Lines processed");
		displayNewLine(DISPLAY_INFO);
		displayInColumn (2, displayCommaNumber (filesFound, numBuff));
		displayInColumn (3, "Files read");
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
 *  P R O C E S S  V A L U E S                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the values read from one line in the source file.
 *  \param values Array of values from the file.
 *  \param readVal Array of bools to show if the value is valid.
 *  \param count Number of triplets to process (min,max,average).
 *  \param getTime Is this an hourly (getTime = 1) or monthly summary.
 *  \result True if processed OK.
 */
void processValues (float *values, int *readVal, int count, int getTime)
{
	int i;

	for (i = 0; i < count; ++i)
	{
		if (readVal[i])
		{
			/**********************************************************************************************************
             * When processing months all the values should be averaged. When processing days the monthly max and min *
             * values are calculated and the averages are averaged.                                                   *
             **********************************************************************************************************/
			int mode = (getTime ? i % 3 : 2);

			switch (mode)
			{
			case 0: /* Min */
				if (savedRead[i] == 0)
				{
					savedValues[i] = values[i];
					savedRead[i] = 1;
				}
				else if (values[i] < savedValues[i])
				{
					savedValues[i] = values[i];
				}
				break;
			case 1: /* Max */
				if (savedRead[i] == 0)
				{
					savedValues[i] = values[i];
					savedRead[i] = 1;
				}
				else if (values[i] > savedValues[i])
				{
					savedValues[i] = values[i];
				}
				break;
			default: /* Average */
				if (savedRead[i] == 0)
				{
					savedValues[i] = values[i];
				}
				else
				{
					savedValues[i] += values[i];
				}
				savedRead[i] += 1;
				break;
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  B U F F E R                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process a line read fom the input file.
 *  \param inBuffer Line read from the file.
 *  \param getTime Is this an hourly (getTime = 1) or monthly summary.
 *  \result True if processed OK.
 */
int processBuffer (char *inBuffer, int getTime)
{
	char singleVal[21];
	float tempValues[15];
	int i = 0, j = 0, k, l = 0, readVal[15], count = 0;

	singleVal[0] = linePrefix[0] = 0;
	for (k = 0; k < 15; ++k)
		readVal[k] = 0;

	while (inBuffer[i])
	{
		if (inBuffer[i] == '-' || inBuffer[i] == '.' || (inBuffer[i] >= '0' && inBuffer[i] <= '9') ||
				(count == 0 && inBuffer[i] == '/'))
		{
			if (j < 20)
			{
				if ((outputMode == 1 || outputMode == 2) && count == 0)
				{
					if (inBuffer[i] == '/')
					{
						++l;
					}
					else if (l == outputMode)
					{
						singleVal[j] = inBuffer[i];
						singleVal[++j] = 0;
					}
				}
				else
				{
					singleVal[j] = inBuffer[i];
					singleVal[++j] = 0;
				}
			}
		}
		else if (inBuffer[i] == ',' || inBuffer[i] == 13)
		{
			if (count == 0 && j)
			{
				strcpy (linePrefix, singleVal);
				if (!getTime)
				{
					++count;
				}
			}
			if (count < 17 && count > 1 && j)
			{
				tempValues[count - 2] = atof (singleVal);
				readVal[count - 2] = 1;
			}
			singleVal[j = 0] = 0;
			++count;
		}
		++i;
	}
	processValues (&tempValues[0], &readVal[0], count, getTime);
	return count;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F L U S H  B U F F E R                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Write the current values back out to the file.
 *  \param outBuffer Write here before calling function writes to the file.
 *  \result True if processed OK.
 */
int flushBuffer (char *outBuffer)
{
	int i, count = 0;

	if (outputMode == 0)
		sprintf (outBuffer, "%s,", linePrefix);
	else
		sprintf (outBuffer, "%s\t", linePrefix);

	for (i = 0; i < 15; ++i)
	{
		if (savedRead[i])
		{
			sprintf (&outBuffer[strlen (outBuffer)],
					(outputMode == 0 ? "%0.2f," : "%0.2f\t"),
					savedValues[i] / savedRead[i]);
			savedRead[i] = 0;
			++count;
		}
		savedValues[i] = 0;
	}
	sprintf (&outBuffer[strlen (outBuffer)], "%d\n", count / 3);
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  N U M B E R                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert the digits in a string to a number.
 *  \param str String to convert.
 *  \result Number found.
 */
int getNumber (char *str)
{
	int retn = 0, i = 0;

	while (str[i])
	{
		if (str[i] >= '0' && str[i] <= '9')
			retn = (retn * 10) + (str[i] - '0');
		++i;
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  D I R                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Remove CR's from LF's to make file Linux compatable.
 *  \param file File to Convert.
 *  \result 1 if file changed.
 */
int showDir (DIR_ENTRY *file)
{
	char inBuffer[256], outBuffer[256], inFile[PATH_SIZE], outFile[PATH_SIZE];
	int lines = 0, i, showCols[3] = { 1, 0, 8 }, getTime = 1;
	FILE *readFile, *writeFile;

	if (inputMode != 0) showCols[2] = 2;

	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	if (getcwd (outFile, 255) == NULL)
		strcpy (outFile, ".");
	strcat (outFile, "/");

	if ((readFile = fopen (inFile, "r")) != NULL)
	{
		if (fgets (inBuffer, 255, readFile))
		{
			if (strncmp (inBuffer, "Date,Time,", 10) != 0)
			{
				if (strncmp (inBuffer, "Date,", 5) != 0)
				{
					fclose (readFile);
					return 0;
				}
				getTime = 0;
			}
		}

		sprintf (&outFile[strlen (outFile)], "%s-%d.%s",
				(inputMode == 0 ? "monitor" : "power"),
				getNumber (file -> fileName) / 100,
				(outputMode == 0 ? "csv" : "tsv"));

		if ((writeFile = fopen (outFile, "r+")) == NULL)
		{
			if ((writeFile = fopen (outFile, "a")) != NULL)
			{
				if (outputMode == 0)
				{
					fputs ("Date,", writeFile);
				}
				else
				{
					fputs ("# Date,", writeFile);
				}
				fputs (&inBuffer[getTime ? 10 : 5], writeFile);
			}
		}
		if (writeFile != NULL)
		{
			fseek (writeFile, 0, SEEK_END);
			while (fgets (inBuffer, 255, readFile))
			{
				if (processBuffer (inBuffer, getTime))
					++totalProcessed;
				++lines;
			}
			for (i = 0; i < 3; ++i)
			{
				int col = showCols[i];

				if (savedRead[col])
					displayInColumn (i, "%0.2f", savedValues[col] / savedRead[col]);
				else
					displayInColumn (i, "-");
			}
			displayInColumn (3, "%s", file -> fileName);
			displayNewLine (0);

			if (flushBuffer (outBuffer))
			{
				fputs (outBuffer, writeFile);
			}
			fclose (writeFile);
		}
		fclose (readFile);
		++filesFound;
	}
	return lines ? 1 : 0;
}

