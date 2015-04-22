/**********************************************************************************************************************
 *                                                                                                                    *
 *  P F I L E . C                                                                                                     *
 *  =============                                                                                                     *
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
 *  \brief Chris Knight's own path search program.
 */
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

#define SHOW_NORMAL	0
#define SHOW_FULL	1
#define SHOW_DOT	2
#define SHOW_PATH	4
#define SHOW_MATCH	8
#define SHOW_RORDER 16
#define SHOW_QUIET	32
#define SHOW_EXE	64

/*----------------------------------------------------------------------------*
 * Globals   															      *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colNormDescs[10] =
{
	{	5,	5,	0,	3,	0x03,	COL_ALIGN_RIGHT,	"Type",		8	},	/*  0 */
	{	10,	10,	0,	3,	0x02,	0,					"Rights",	3	},	/*  1 */
	{	20,	6,	0,	1,	0x05,	0,					"Owner",	4	},	/*  2 */
	{	20,	6,	0,	3,	0x05,	0,					"Group",	5	},	/*  3 */
	{	7,	7,	0,	3,	0x06,	COL_ALIGN_RIGHT,	"Size",		1	},	/*  4 */
	{	160,12,	0,	3,	0x02,	COL_ALIGN_RIGHT,	"Modified",	2	},	/*  5 */
	{	160,12,	0,	3,	0x07,	0,					"Filename",	0	},	/*  6 */
	{	3,	3,	0,	3,	0x01,	0,					NULL,		7	},	/*  7 */
	{	160,12,	0,	0,	0x07,	0,					"Target",	6	},	/*  8 */
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
int 		orderType	= ORDER_NONE;
int 		showType	= SHOW_NORMAL;
int			dirType		= ONLYEXECS|ONLYLINKS;
char 		findFilePath[PATH_SIZE];
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
	printf ("TheKnight: Find a File in the Path, Version %s\n", directoryVersion());
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
	int i = 1, j, found = 0;
	void *fileList = NULL;

	displayGetWindowSize ();

	if (!envPtr)
	{
		version ();
		printf ("The PATH evironment variable could not be found\n");
		exit (1);
	}

    /*------------------------------------------------------------------------*
     * Look for any command line switches                                     *
     *------------------------------------------------------------------------*/
	while (i < argc)
	{
		if (argv[i][0] == '-')
		{
			j = 1;
			while (argv[i][j])
			{
				switch (argv[i][j++])
				{
				case 'o':
					switch (argv[i][j])
					{
					case 's':
					case 'S':
						orderType = ORDER_SIZE;
						showType |= (argv[i][j] == 'S' ? SHOW_RORDER : 0);
						j++;
						break;
						
					case 't':
					case 'T':
						orderType = ORDER_DATE;
						showType |= (argv[i][j] == 'T' ? SHOW_RORDER : 0);
						j++;
						break;
						
					case 'n':
					case 'N':
						orderType = ORDER_NONE;
						showType |= (argv[i][j] == 'N' ? SHOW_RORDER : 0);
						j++;
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
					switch (argv[i][j])
					{
					case 'd':
						dirType &= ~(ONLYFILES|ONLYLINKS);
						dirType |= ONLYDIRS;
						j++;
						break;

					case 'f':
						dirType &= ~(ONLYDIRS|ONLYLINKS);
						dirType |= ONLYFILES;
						j++;
						break;

					case 'l':	
						dirType &= ~(ONLYDIRS|ONLYFILES);
						dirType |= ONLYLINKS;
						j++;
						break;
						
					case 'x':	
						dirType &= ~(ONLYDIRS|ONLYLINKS|ONLYFILES);
						dirType |= ONLYEXECS;
						j++;
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
					printf ("%s -[Options] [FileNames] [FileNames]...\n\n", basename (argv[0]));
					printf ("Options: \n");
					printf ("         -a  . . . . . Include hidden files and directories\n");
					printf ("         -c  . . . . . Makes the directory case sensitive\n");
					printf ("         -f  . . . . . Show full file details\n");
					printf ("         -o[t|T] . . . Order the files by time and date\n");
					printf ("         -o[s|S] . . . Order the files by size\n");
					printf ("         -o[n|N] . . . Do not order use directory order\n");
					printf ("         -q  . . . . . Quiet mode, only file names\n");
					printf ("         -sd . . . . . Show only dirs, not files and links\n");
					printf ("         -sf . . . . . Show only files, not dirs and links\n");
					printf ("         -sl . . . . . Show only links, not files and dirs\n");
					printf ("         -sx . . . . . Show only executable files\n");
					printf ("         -x  . . . . . Also search for .exe files\n");
					displayLine ();
					exit (1);
				}
			}
		}
		i++;
	}
	i = 1;
	
	while (i < argc)
	{
		if (argv[i][0] != '-')
		{
			int j = 0, k = 0;

			while (1)
			{
				if (!envPtr[j] || envPtr[j] == PATHSEP)
				{
					if (k)
					{
						findFilePath[k++] = DIRSEP;
						findFilePath[k] = 0;
						strcat (findFilePath, argv[i]);
						found += directoryLoad (findFilePath, dirType, fileCompare, &fileList);
								
						if (showType & SHOW_EXE)
						{
							strcat (findFilePath, ".exe");
							found += directoryLoad (findFilePath, dirType, 
									fileCompare, &fileList);
						}
						k = 0;
					}
					if (!envPtr[j++])
						break;
				}
				else
					findFilePath[k++] = envPtr[j++];
			}
		}
		i++;
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

	strcpy (fullName, file -> fullPath);
	strcat (fullName, file -> fileName);

	/*------------------------------------------------------------------------*
	 * Show the directory in simple format.                                   *
	 *------------------------------------------------------------------------*/
	if (showType & SHOW_QUIET)
	{
		if (S_ISLNK (file -> fileStat.st_mode))
		{
			showFile = 1;
		}
		else if (S_ISDIR (file -> fileStat.st_mode))
		{
			showFile = 1;
		}
		else if (!file -> fileStat.st_mode || file -> fileStat.st_mode & S_IFREG)
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
		char rightsBuff[12];
		char ownerString[81];
		char groupString[81];
		char numBuff[10];

		/*--------------------------------------------------------------------*
	   	 * If the file is a LINK then just them it is a link.                 *
		 *--------------------------------------------------------------------*/
		if (S_ISLNK (file -> fileStat.st_mode))
		{
			int linkSize;
			char linkBuff[1025];
						
			if ((linkSize = readlink(fullName, linkBuff, 1024)) >= 0)
				linkBuff[linkSize] = 0;
			else
				linkBuff[0] = 0;

			displayInColumn (0, "<Lnk>");
			displayInColumn (1, displayRightsString (file -> fileStat.st_mode, rightsBuff));
			displayInColumn (2, displayOwnerString (file -> fileStat.st_uid, ownerString));
			displayInColumn (3, displayGroupString (file -> fileStat.st_gid, groupString));
			displayInColumn (5,	displayDateString (file -> fileStat.st_mtime, dateString));
			displayInColumn (6, fullName);
			displayInColumn (7, "->");
			displayInColumn (8, linkBuff);
			displayNewLine (0);
			linksFound ++;
		}
		
		/*--------------------------------------------------------------------*
	   	 * If the file is a directory then just show them the name.           *
		 *--------------------------------------------------------------------*/
		else if (S_ISDIR (file -> fileStat.st_mode))
		{
			displayInColumn (0, "<Dir>");
			displayInColumn (1, displayRightsString (file -> fileStat.st_mode, rightsBuff));
			displayInColumn (2, displayOwnerString (file -> fileStat.st_uid, ownerString));
			displayInColumn (3, displayGroupString (file -> fileStat.st_gid, groupString));
			displayInColumn (5,	displayDateString (file -> fileStat.st_mtime, dateString));
			displayInColumn (6, fullName);
			displayNewLine (0);
			dirsFound ++;
		}
		
		/*--------------------------------------------------------------------*
   		 * If the file is a normal file then show the size/date/time.         *
		 *--------------------------------------------------------------------*/
		else if (!file -> fileStat.st_mode || file -> fileStat.st_mode & S_IFREG)
		{
			displayInColumn (1, displayRightsString (file -> fileStat.st_mode, rightsBuff));
			displayInColumn (2, displayOwnerString (file -> fileStat.st_uid, ownerString));
			displayInColumn (3, displayGroupString (file -> fileStat.st_gid, groupString));
			displayInColumn (4,	displayFileSize (file -> fileStat.st_size, numBuff));
			displayInColumn (5,	displayDateString (file -> fileStat.st_mtime, dateString));
			displayInColumn (6, fullName);
			displayNewLine (0);
			filesFound ++;

			totalSize += file -> fileStat.st_size;
		}
		else
			return 0;
	}
	/*------------------------------------------------------------------------*
	 * Show the directory in normal format.                                   *
	 *------------------------------------------------------------------------*/
	else
	{
		if (S_ISLNK (file -> fileStat.st_mode))
		{
			linksFound ++;
			showFile = 1;
		}
		else if (S_ISDIR (file -> fileStat.st_mode))
		{
			dirsFound ++;
			showFile = 1;
		}
		else if (!file -> fileStat.st_mode || file -> fileStat.st_mode & S_IFREG)
		{
			totalSize += file -> fileStat.st_size;
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
	char fullName[1024];

	switch (orderType)
	{
	case ORDER_NONE:
		retn = 1;
		break;
		
	case ORDER_SIZE:
		if (showType & SHOW_MATCH)
		{
			if (fileOne -> fileStat.st_size == fileTwo -> fileStat.st_size)
			{
				if (!fileOne -> doneCRC)
				{
					sprintf (fullName, "%s%s", fileOne -> fullPath,
							fileOne -> fileName);

					fileOne -> CRC = CRCFile (fullName);
					fileOne -> doneCRC = 1;
				}
				if (!fileTwo -> doneCRC)
				{
					sprintf (fullName, "%s%s", fileTwo -> fullPath,
							fileTwo -> fileName);

					fileTwo -> CRC = CRCFile (fullName);
					fileTwo -> doneCRC = 1;
				}
				if (fileOne -> CRC == fileTwo -> CRC)
				{
					fileOne -> match = 1;
					fileTwo -> match = 1;
				}
			}
		}
		retn = (fileOne -> fileStat.st_size > fileTwo -> fileStat.st_size ? 1 :
				fileOne -> fileStat.st_size < fileTwo -> fileStat.st_size ? -1 : 0);
		break;

	case ORDER_DATE:
		retn = (fileOne -> fileStat.st_mtime > fileTwo -> fileStat.st_mtime ? -1 :
				fileOne -> fileStat.st_mtime < fileTwo -> fileStat.st_mtime ? 1 : 0);
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
