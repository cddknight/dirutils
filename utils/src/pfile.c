/**********************************************************************************************************************
 *                                                                                                                    *
 *  P F I L E . C                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File pfile.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of    *
 *  the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or  *
 *  (at your option) any later version.                                                                               *
 *                                                                                                                    *
 *  DirCmdUtils is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the         *
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for  *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see            *
 *  <http://www.gnu.org/licenses/>.                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Chris Knight's own path search program.
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

/*----------------------------------------------------------------------------*
 * Prototypes															      *
 *----------------------------------------------------------------------------*/
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Defines   															      *
 *----------------------------------------------------------------------------*/
#define ORDER_NONE	0
#define ORDER_NAME	1
#define ORDER_DATE	2
#define ORDER_SIZE	3

#define SHOW_NORMAL 0
#define SHOW_FULL	1
#define SHOW_DOT	2
#define SHOW_PATH	4
#define SHOW_RORDER 16
#define SHOW_QUIET	32
#define SHOW_EXE	64

/*----------------------------------------------------------------------------*
 * Globals   															      *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colNormDescs[10] =
{
	{	5,	5,	0,	3,	0x03,	COL_ALIGN_RIGHT,	"Type",		8	},	/*  0 */
#ifdef HAVE_SYS_ACL_H
	{	11, 11, 0,	3,	0x02,	0,					"Rights",	3	},	/*  1 */
#else
	{	10, 10, 0,	3,	0x02,	0,					"Rights",	3	},	/*  1 */
#endif
	{	20, 6,	0,	1,	0x05,	0,					"Owner",	4	},	/*  2 */
	{	20, 6,	0,	3,	0x05,	0,					"Group",	5	},	/*  3 */
	{	7,	7,	0,	3,	0x06,	COL_ALIGN_RIGHT,	"Size",		1	},	/*  4 */
	{	160,12, 0,	3,	0x02,	COL_ALIGN_RIGHT,	"Modified", 2	},	/*  5 */
	{	160,12, 0,	3,	0x07,	0,					"Filename", 0	},	/*  6 */
	{	3,	3,	0,	3,	0x01,	0,					NULL,		7	},	/*  7 */
	{	160,12, 0,	0,	0x07,	0,					"Target",	6	},	/*  8 */
};

COLUMN_DESC *ptrFullColumn[10] =
{
	&colNormDescs[0],
	&colNormDescs[1],
	&colNormDescs[2],
	&colNormDescs[3],
	&colNormDescs[4],
	&colNormDescs[5],
	&colNormDescs[6],
	&colNormDescs[7],
	&colNormDescs[8]
};

COLUMN_DESC *ptrNormalColumn[1] =
{
	&colNormDescs[6]
};

long		dirsFound	= 0;
long		filesFound	= 0;
long		linksFound	= 0;
long long	totalSize	= 0;
int			orderType	= ORDER_NONE;
int			showType	= SHOW_NORMAL;
int			dirType		= ONLYEXECS|ONLYLINKS;
char		findFilePath[PATH_SIZE];
int			displayColour = 0;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V E R S I O N                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display version information.
 *  \result Nothing.
 */
void version ()
{
	printf ("TheKnight: Find a File in the Path, Version: %s, Built: %s\n", VERSION, buildDate);
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
	char *envPtr = getenv ("PATH");
	int c, found = 0;
	void *fileList = NULL;
	char fullVersion[81];

	static struct option long_options[] =
	{
		{ "all", no_argument, 0, 'a' },
		{ "case", no_argument, 0, 'c' },
		{ "colour", no_argument, 0, 'C' },
		{ "full", no_argument, 0, 'f' },
		{ "order", required_argument, 0, 'o' },
		{ "quiet", no_argument, 0, 'q' },
		{ "show", required_argument, 0, 's' },
		{ "exe", no_argument, 0, 'x' },
		{ "help", no_argument, 0, '?' },
		{0, 0, 0, 0}
	};

	strcpy (fullVersion, VERSION);
#ifdef USE_STATX
	strcat (fullVersion, ".1");
#else
	strcat (fullVersion, ".0");
#endif
	if (strcmp (directoryVersion(), fullVersion) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), fullVersion);
		exit (1);
	}

	displayInit ();

	if (!envPtr)
	{
		version ();
		printf ("The PATH evironment variable could not be found\n");
		exit (1);
	}

	while (1)
	{
		/*--------------------------------------------------------------------*
		 * getopt_long stores the option index here.                          *
	     *--------------------------------------------------------------------*/
		int option_index = 0;

		c = getopt_long (argc, argv, "acCfo:qs:x?", long_options, &option_index);

		/*--------------------------------------------------------------------*
		 * Detect the end of the options.                                     *
	     *--------------------------------------------------------------------*/
		if (c == -1) break;

		switch (c)
		{
		case 'o':
			switch (optarg[0])
			{
			case 's':
			case 'S':
				orderType = ORDER_SIZE;
				showType |= (optarg[0] == 'S' ? SHOW_RORDER : 0);
				break;

			case 't':
			case 'T':
				orderType = ORDER_DATE;
				showType |= (optarg[0] == 'T' ? SHOW_RORDER : 0);
				break;

			case 'n':
			case 'N':
				orderType = ORDER_NONE;
				showType |= (optarg[0] == 'N' ? SHOW_RORDER : 0);
				break;
			}
			break;

		case 'a':
			showType |= SHOW_DOT;
			break;

		case 'c':
			dirType |= USECASE;
			break;

		case 'C':
			displayColour ^= DISPLAY_COLOURS;
			break;

		case 'f':
			showType |= SHOW_FULL;
			break;

		case 's':
			switch (optarg[0])
			{
			case 'd':
				dirType &= ~(ONLYFILES|ONLYLINKS);
				dirType |= ONLYDIRS;
				break;

			case 'f':
				dirType &= ~(ONLYDIRS|ONLYLINKS);
				dirType |= ONLYFILES;
				break;

			case 'l':
				dirType &= ~(ONLYDIRS|ONLYFILES);
				dirType |= ONLYLINKS;
				break;

			case 'x':
				dirType &= ~(ONLYDIRS|ONLYLINKS|ONLYFILES);
				dirType |= ONLYEXECS;
				break;
			}
			break;

		case 'q':
			showType |= SHOW_QUIET;
			break;

		case 'x':
			showType |= SHOW_EXE;
			break;

		case '?':
			version ();
			printf ("%s -[Options] [FileName] [FileName]...\n\n", basename (argv[0]));
			printf ("Options: \n");
			printf ("         --all  . . . . -a  . . . Include hidden files and directories\n");
			printf ("         --case . . . . -c  . . . Makes the directory case sensitive\n");
			printf ("         --colour . . . -C  . . . Show in colour\n");
			printf ("         --full . . . . -f  . . . Show full file details\n");
			printf ("         --order t  . . -oT . . . Order the files by time and date\n");
			printf ("         --order s  . . -oS . . . Order the files by size\n");
			printf ("         --order n  . . -oN . . . Do not order use find order\n");
			printf ("         --quiet  . . . -q  . . . Quiet mode, only file names\n");
			printf ("         --show d . . . -sd . . . Show only dirs, not files and links\n");
			printf ("         --show f . . . -sf . . . Show only files, not dirs and links\n");
			printf ("         --show l . . . -sl . . . Show only links, not files and dirs\n");
			printf ("         --show x . . . -sx . . . Show only executable files\n");
			printf ("         --exe  . . . . -x  . . . Also search for .exe files\n");
			printf ("         --help . . . . -?  . . . Show this help message\n");
			displayLine ();
			exit (1);
		}
	}

	while (optind < argc)
	{
		int k = 0, j = 0;
		while (1)
		{
			if (!envPtr[j] || envPtr[j] == PATHSEP)
			{
				if (k)
				{
					findFilePath[k++] = DIRSEP;
					findFilePath[k] = 0;
					strcat (findFilePath, argv[optind]);
					found += directoryLoad (findFilePath, dirType, fileCompare, &fileList);

					if (showType & SHOW_EXE)
					{
						strcat (findFilePath, ".exe");
						found += directoryLoad (findFilePath, dirType, fileCompare, &fileList);
					}
					k = 0;
				}
				if (!envPtr[j++])
					break;
			}
			else
			{
				findFilePath[k++] = envPtr[j++];
			}
		}
		++optind;
	}

	if (found)
	{
		/*--------------------------------------------------------------------*
	     * Now we can sort the directory.                                     *
	     *--------------------------------------------------------------------*/
		directorySort (&fileList);

		if (showType & SHOW_FULL)
		{
			if (!displayColumnInit (9, ptrFullColumn, displayColour | DISPLAY_HEADINGS))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 1;
			}
		}
		else
		{
			if (!displayColumnInit (1, ptrNormalColumn, displayColour | DISPLAY_HEADINGS))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 1;
			}
		}
		directoryProcess (showDir, &fileList);
	}

	if (!(showType & SHOW_QUIET))
	{
		if (filesFound || linksFound || dirsFound)
		{
			char foundBuff[11], sizeBuff[11];

			displayNewLine (0);
			displayDrawLine (0);

			if (showType & SHOW_FULL)
			{
				if (filesFound)
				{
					displayInColumn (1, "%10s", displayCommaNumber (filesFound, foundBuff));
					displayInColumn (2, filesFound == 1 ? "File" : "Files");
					displayInColumn (4, displayFileSize (totalSize, sizeBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (linksFound)
				{
					displayInColumn (1, "%10s", displayCommaNumber (linksFound, foundBuff));
					displayInColumn (2, linksFound == 1 ? "Link" : "Links");
					displayNewLine(DISPLAY_INFO);
				}
				if (dirsFound)
				{
					displayInColumn (1, "%10s", displayCommaNumber (dirsFound, foundBuff));
					displayInColumn (2, dirsFound == 1 ? "Dir" : "Dirs");
					displayNewLine(DISPLAY_INFO);
				}
			}
			else
			{
				if (filesFound)
				{
					displayInColumn (0, "%6s  %s", displayCommaNumber (filesFound, foundBuff),
							filesFound == 1 ? "File" : "Files");
					displayNewLine(DISPLAY_INFO);
					displayInColumn (0, "%7s Total size", displayFileSize (totalSize, sizeBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (linksFound)
				{
					displayInColumn (0, "%6s  %s", displayCommaNumber (linksFound, foundBuff),
							linksFound == 1 ? "Link" : "Links");
					displayNewLine(DISPLAY_INFO);
				}
				if (dirsFound)
				{
					displayInColumn (0, "%5s  %s", displayCommaNumber (dirsFound, foundBuff),
							dirsFound == 1 ? "Dir" : "Dirs");
					displayNewLine(DISPLAY_INFO);
				}
			}
			displayAllLines ();
			displayTidy ();
		}
		else
		{
			version ();
			printf ("No matches found\n");
		}
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
 *  \brief Called back by process dir to show the directory.
 *  \param file Information about the file to show.
 *  \result 1 if the file was shown.
 */
int showDir (DIR_ENTRY *file)
{
	int showFile = 0;
	char fullName[1025];

	if (file -> fileName[0] == '.' && !(showType & SHOW_DOT))
		return 0;

#ifdef USE_STATX
	mode_t stMode = file -> fileStat.stx_mode;
	uid_t stUId = file -> fileStat.stx_uid;
	gid_t stGId = file -> fileStat.stx_gid;
	off_t stSize = file -> fileStat.stx_size;
	time_t stMTime = file -> fileStat.stx_mtime.tv_sec;
#else
	mode_t stMode = file -> fileStat.st_mode;
	uid_t stUId = file -> fileStat.st_uid;
	gid_t stGId = file -> fileStat.st_gid;
	off_t stSize = file -> fileStat.st_size;
	time_t stMTime = file -> fileStat.st_mtime;
#endif

	strcpy (fullName, file -> fullPath);
	strcat (fullName, file -> fileName);

	/*------------------------------------------------------------------------*
	 * Show the directory in simple format.                                   *
	 *------------------------------------------------------------------------*/
	if (showType & SHOW_QUIET)
	{
		if (S_ISLNK (stMode))
		{
			showFile = 1;
		}
		else if (S_ISDIR (stMode))
		{
			showFile = 1;
		}
		else if (!stMode || stMode & S_IFREG)
		{
			showFile = 1;
		}
		if (showFile)
		{
			printf ("%s\n", fullName);
		}
	}
	/*------------------------------------------------------------------------*
	 * Show the directory in full format.                                     *
	 *------------------------------------------------------------------------*/
	else if (showType & SHOW_FULL)
	{
		char dateString[41];
		char rightsBuff[14];
		char ownerString[81];
		char groupString[81];
		char numBuff[10];

		/*--------------------------------------------------------------------*
	   	 * If the file is a LINK then just them it is a link.                 *
		 *--------------------------------------------------------------------*/
		if (S_ISLNK (stMode))
		{
			int linkSize;
			char linkBuff[1025];

			if ((linkSize = readlink(fullName, linkBuff, 1024)) >= 0)
				linkBuff[linkSize] = 0;
			else
				linkBuff[0] = 0;

			displayInColumn (0, "<Lnk>");
			displayInColumn (1, displayRightsStringACL (file, rightsBuff));
			displayInColumn (2, displayOwnerString (stUId, ownerString));
			displayInColumn (3, displayGroupString (stGId, groupString));
			displayInColumn (5, displayDateString (stMTime, dateString));
			displayInColumn (6, fullName);
			displayInColumn (7, "->");
			displayInColumn (8, linkBuff);
			displayNewLine (0);
			linksFound ++;
		}

		/*--------------------------------------------------------------------*
	   	 * If the file is a directory then just show them the name.           *
		 *--------------------------------------------------------------------*/
		else if (S_ISDIR (stMode))
		{
			displayInColumn (0, "<Dir>");
			displayInColumn (1, displayRightsStringACL (file, rightsBuff));
			displayInColumn (2, displayOwnerString (stUId, ownerString));
			displayInColumn (3, displayGroupString (stGId, groupString));
			displayInColumn (5, displayDateString (stMTime, dateString));
			displayInColumn (6, fullName);
			displayNewLine (0);
			dirsFound ++;
		}

		/*--------------------------------------------------------------------*
   		 * If the file is a normal file then show the size/date/time.         *
		 *--------------------------------------------------------------------*/
		else if (!stMode || stMode & S_IFREG)
		{
			displayInColumn (1, displayRightsStringACL (file, rightsBuff));
			displayInColumn (2, displayOwnerString (stUId, ownerString));
			displayInColumn (3, displayGroupString (stGId, groupString));
			displayInColumn (4, displayFileSize (stSize, numBuff));
			displayInColumn (5, displayDateString (stMTime, dateString));
			displayInColumn (6, fullName);
			displayNewLine (0);
			filesFound ++;

			totalSize += stSize;
		}
		else
			return 0;
	}
	/*------------------------------------------------------------------------*
	 * Show the directory in normal format.                                   *
	 *------------------------------------------------------------------------*/
	else
	{
		if (S_ISLNK (stMode))
		{
			linksFound ++;
			showFile = 1;
		}
		else if (S_ISDIR (stMode))
		{
			dirsFound ++;
			showFile = 1;
		}
		else if (!stMode || stMode & S_IFREG)
		{
			totalSize += stSize;
			filesFound ++;
			showFile = 1;
		}
		if (showFile)
		{
			displayInColumn (0, fullName);
			displayNewLine (0);
		}
	}
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I L E  C O M P A R E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called back by load dir to sort the files.
 *  \param fileOne First file.
 *  \param fileTwo Second file to compare first with.
 *  \result 1, 0 or -1 depending on order.
 */
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo)
{
	int retn = 0;
#ifdef USE_STATX
	off_t stSizeOne = fileOne -> fileStat.stx_size;
	off_t stSizeTwo = fileTwo -> fileStat.stx_size;
	time_t stMTimeOne = fileOne -> fileStat.stx_mtime.tv_sec;
	time_t stMTimeTwo = fileTwo -> fileStat.stx_mtime.tv_sec;
#else
	off_t stSizeOne = fileOne -> fileStat.st_size;
	off_t stSizeTwo = fileTwo -> fileStat.st_size;
	time_t stMTimeOne = fileOne -> fileStat.st_mtime;
	time_t stMTimeTwo = fileTwo -> fileStat.st_mtime;
#endif

	switch (orderType)
	{
	case ORDER_NONE:
		retn = 1;
		break;

	case ORDER_SIZE:
		retn = (stSizeOne > stSizeTwo ? 1 : stSizeOne < stSizeTwo ? -1 : 0);
		break;

	case ORDER_DATE:
		retn = (stMTimeOne > stMTimeTwo ? -1 : stMTimeOne < stMTimeTwo ? 1 : 0);
		break;
	}
	if (retn == 0)
		retn = strcasecmp (fileOne -> fileName, fileTwo -> fileName);

	if (showType & SHOW_RORDER)
	{
		if (retn == -1)
			retn = 1;
		else if (retn == 1)
			retn = -1;
	}
	return retn;
}
