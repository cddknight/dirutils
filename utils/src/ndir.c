/**********************************************************************************************************************
 *                                                                                                                    *
 *  N D I R . C                                                                                                       *
 *  ===========                                                                                                       *
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
 *  \brief Chris Knight's own directory program.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#ifdef HAVE_VALUES_H
#include <values.h>
#else
#define MAXINT 2147483647
#endif
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dircmd.h>
#include <errno.h>

extern int errno;

#ifdef DOS
#define DIRCHAR '\\'
#define DIRDEF "\\*"
#else
#define DIRCHAR '/'
#define DIRDEF "\057*"		/* Make the editor look nice */
#endif

/*----------------------------------------------------------------------------*
 * Prototypes															      *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
char *quoteCopy (char *dst, char *src);

/*----------------------------------------------------------------------------*
 * Defines   															      *
 *----------------------------------------------------------------------------*/
#define ORDER_NAME		0
#define ORDER_DATE		1
#define ORDER_SIZE		2
#define ORDER_NONE		3
#define ORDER_EXTN		4
#define ORDER_OWNR		5
#define ORDER_GRUP		6
#define ORDER_LINK		7
#define ORDER_CNXT		8
#define ORDER_MD5S		9

#define SHOW_NORMAL		0
#define SHOW_WIDE		1
#define SHOW_AGE		(1 << 1)
#define SHOW_PATH		(1 << 2)
#define SHOW_MATCH		(1 << 3)
#define SHOW_RORDER 	(1 << 4)
#define SHOW_QUIET		(1 << 5)
#define SHOW_OWNER		(1 << 6)
#define SHOW_GROUP		(1 << 7)
#define SHOW_RIGHTS		(1 << 8)
#define SHOW_TYPE		(1 << 9)
#define SHOW_LINK		(1 << 10)
#define SHOW_EXTN		(1 << 11)
#define SHOW_SIZE		(1 << 12)
#define SHOW_DATE		(1 << 13)
#define SHOW_NUM_LINKS	(1 << 14)
#define SHOW_NO_MATCH	(1 << 15)
#define SHOW_BACKUP		(1 << 16)
#define SHOW_QUOTE		(1 << 17)
#define SHOW_SELINUX	(1 << 18)
#define SHOW_MD5		(1 << 19)

#define DATE_MOD		0
#define DATE_ACC		1
#define DATE_CHG		2

#define MAX_COL_DESC	15
#define MAX_W_COL_DESC	3
#define EXTRA_COLOURS	8

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
int			orderType		= 	ORDER_NAME;
int			showType		= 	SHOW_NORMAL|SHOW_SIZE|SHOW_DATE;
int			dirType			= 	ALLFILES;
long		dirsFound		= 	0;
long		filesFound		= 	0;
long		linksFound		= 	0;
long		devsFound		=	0;
long		socksFound		=	0;
long		pipesFound		=	0;
long long	totalSize		= 	0;
int			currentCol 		= 	0;
int 		maxCol 			= 	4;
int 		showDate		= 	DATE_MOD;
int			showFound		=	MAXINT;
int			sizeFormat		=	1;
char		dateType[41]	=	"Modified";
time_t		timeNow			= 	0;
char		*quoteMe		=	" *?|&;()<>#\t\\";

int			coloursAlt[MAX_COL_DESC + MAX_W_COL_DESC];	
int			colourType[EXTRA_COLOURS];
int			dirColour = 0;

/*----------------------------------------------------------------------------*
 * Column definitions                                                         *
 *----------------------------------------------------------------------------*/
#define		COL_TYPE		0
#define		COL_RIGHTS		1
#define		COL_N_LINKS		2
#define		COL_OWNER		3
#define		COL_GROUP		4
#define		COL_SIZE		5
#define		COL_DATE		6
#define		COL_DAYS		7
#define		COL_TIME		8
#define		COL_FILENAME	9
#define		COL_EXTN		10
#define		COL_ARROW		11
#define		COL_TARGET		12
#define		COL_CONTEXT		13
#define		COL_MD5			14

#define		COL_W_TYPE_L	0
#define		COL_W_FILENAME	1
#define		COL_W_TYPW_R	2

int columnTranslate[MAX_COL_DESC] = 
{
	COL_TYPE, COL_RIGHTS, COL_N_LINKS, COL_OWNER, COL_GROUP, COL_SIZE, 
	COL_DATE, COL_DAYS, COL_TIME, COL_FILENAME, COL_EXTN, COL_ARROW, 
	COL_TARGET, COL_CONTEXT, COL_MD5
};

COLUMN_DESC allColumnDescs[MAX_COL_DESC] =
{
	{	6,	6,	0,	2,	0x03,	0,					"Type",		11	},	/*  0 */
#ifdef HAVE_SYS_ACL_H
	{	11,	11,	0,	2,	0x02,	0,					"Rights",	5	},	/*  1 */
#else
	{	10,	10,	0,	2,	0x02,	0,					"Rights",	5	},	/*  1 */
#endif
	{	20,	6,	0,	2,	0x04,	COL_ALIGN_RIGHT,	"Links",	8	},	/*  2 */
	{	20,	6,	0,	2,	0x05,	0,					"Owner",	6	},	/*  3 */
	{	20,	6,	0,	2,	0x05,	0,					"Group",	7	},	/*  8 */
	{	20,	7,	0,	2,	0x06,	COL_ALIGN_RIGHT,	"Size",		2	},	/*  4 */
	{	255,12,	0,	2,	0x02,	COL_ALIGN_RIGHT,	dateType,	3	},	/*  5 */
	{	7,	4,	0,	1,	0x02,	COL_ALIGN_RIGHT,	"Days",		3	},	/*  6 */
	{	255,8,	0,	2,	0x02,	0,					"HH:MM:SS",	4	},	/*  7 */
	{	255,12,	8,	2,	0x07,	0,					"Filename",	0	},	/*  9 */
	{	20,	3,	0,	2,	0x07,	0,					"Extn",		1	},	/* 10 */
	{	2,	2,	0,	2,	0x01,	0,					NULL,		10	},	/* 11 */
	{	255,12,	0,	2,	0x07,	0,					"Target",	9	},	/* 12 */
	{	80,	8,	0,	2,	0x05,	0,					"Context",	12	},  /* 13 */
	{	33,	33,	0,	2,	0x05,	0,					"MD5 Sum",	13	},  /* 13 */
};

COLUMN_DESC wideColumnDescs[MAX_W_COL_DESC] =
{
	{	1,	1,	0,	0,	0x00,	0,					NULL,		1},		/*  0 */
	{	255,12,	0,	0,	0x00,	0,					"Filename",	0},		/*  1 */
	{	1,	1,	0,	0,	0x00,	0,					NULL,		2},		/*  2 */
};

COLUMN_DESC *ptrAllColumns[30] =
{
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],	
	&wideColumnDescs[COL_W_TYPE_L],	&wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R]
};

/*----------------------------------------------------------------------------*
 * The name of the colours in the cfg file                                    *
 *----------------------------------------------------------------------------*/
char *colourNames[] = 
{
	"colour_type",		"colour_rights",	"colour_numlinks",	"colour_owner",		"colour_group",		
	"colour_size",		"colour_date",		"colour_day",		"colour_time",		"colour_filename",	
	"colour_extn",		"colour_linkptr",	"colour_target",	"colour_context",	"colour_md5",

	"colour_wide_col1",	"colour_wide_filename", "colour_wide_col2",

	"colour_directory",	"colour_link",		"colour_base",		"colour_executable",
	"colour_read",		"colour_write",		"colour_other",		"colour_block"
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V E R S I O N                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display version information for the library.
 *  \result None.
 */
void version ()
{
	printf ("TheKnight: New Directory, Version %s\n", directoryVersion());
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
 *  \param progName The name of the program being run.
 *  \result None.
 */
void helpThem (char *progName)	
{
	version ();
	printf ("%s -[Options] [FileNames] [FileNames]...\n", progName);
	printf ("\nOptions:\n");
	printf ("         -a  . . . . . Show all files including hidden files\n");
	printf ("         -A  . . . . . Show the age of the file\n");
	printf ("         -b  . . . . . Show backup files ending with ~\n");
	printf ("         -c  . . . . . Makes the directory case sensitive\n");
	printf ("         -C  . . . . . Toggle colour display, defined in dirrc\n");
	printf ("         -d[n] . . . . Display some, n > 0 first n, n < 0 last n\n");
	printf ("         -Dc . . . . . Show the context of the file\n");
	printf ("         -Dd . . . . . Show the date of the file\n");
	printf ("         -De . . . . . Show the file extension\n");
	printf ("         -Dg . . . . . Show the group of the file\n");
	printf ("         -Dl . . . . . Show the target of the link\n");
	printf ("         -Dm . . . . . Show the MD5 checksum of the file\n");
	printf ("         -Dn . . . . . Show the number of links\n");
	printf ("         -Do . . . . . Show the owner of the file\n");
	printf ("         -Dr . . . . . Show the user rights of the file\n");
	printf ("         -Ds . . . . . Show the size of the file\n");
	printf ("         -Dt . . . . . Show the type of the file\n");
	printf ("         -m  . . . . . Show only duplicated files\n");
	printf ("         -M  . . . . . Show only files with no duplicate\n");
	printf ("         -o[c|C] . . . Order the files by context\n");
	printf ("         -o[e|E] . . . Order the files by extension\n");
	printf ("         -o[f|F] . . . Order by the file name (default)\n");
	printf ("         -o[g|G] . . . Order the files by group name\n");
	printf ("         -o[l|L] . . . Order by the number of hard links\n");
	printf ("         -o[m|M] . . . Order by the MD5 checksum\n");
	printf ("         -o[n|N] . . . Do not order, use directory order\n");
	printf ("         -o[o|O] . . . Order the files by owners name\n");
	printf ("         -o[s|S] . . . Order the files by size\n");
	printf ("         -o[t|T] . . . Order the files by time and date\n");
	printf ("         -p  . . . . . Show the full path to the file\n");
	printf ("         -q  . . . . . Quiet mode, only paths and file names\n");
	printf ("         -Q  . . . . . Quiet mode, with quoted special chars\n");
	printf ("         -r  . . . . . Recursive directory listing\n"); 
	printf ("         -sd . . . . . Show only dirs\n");
	printf ("         -sf . . . . . Show only files\n");
	printf ("         -sl . . . . . Show only links\n");
	printf ("         -sp . . . . . Show only pipes\n");
	printf ("         -ss . . . . . Show only sockets\n");
	printf ("         -sv . . . . . Show only devices\n");
	printf ("         -sx . . . . . Show only executable files\n");
	printf ("         -S  . . . . . Show the file size in full\n");
	printf ("         -ta . . . . . Show time of last access\n");
	printf ("         -tc . . . . . Show time of last status change\n");
	printf ("         -tm . . . . . Show time of last modification\n");
	printf ("         -w  . . . . . Show directory in wide format\n");
	printf ("         -W  . . . . . Ignore screen width default to 255\n");
	printf ("\nExpressions:\n");
	printf ("         & . . . . . . Logical AND, eg. %s \"c*&*c\"\n", progName);
	printf ("         | . . . . . . Logical OR,  eg. %s \"c*|d*\"\n", progName);
	printf ("         ^ . . . . . . Logical NOT, eg. %s \"c*&^*c\"\n", progName);
	displayLine ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O M M A N D  O P T I O N                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process a single command line option.
 *  \param option The option to process.
 *  \param progName The name of the program being run.
 *  \result None.
 */
void commandOption (char *option, char *progName)
{
	int j = 0, k;
	
	while (option[j])
	{
		switch (option[j++])
		{
		case 'C':
			if (dirColour == DISPLAY_COLOURS)
				dirColour = 0;
			else
			{
				for (k = 0; k < MAX_COL_DESC; k++)
				{
					allColumnDescs[k].colour = coloursAlt[k];
				}
				for (k = 0; k < MAX_W_COL_DESC; k++)
				{
					wideColumnDescs[k].colour = coloursAlt[k + MAX_COL_DESC];
				}
				dirColour = DISPLAY_COLOURS;
			}
			break;
			
		case 'o':
			switch (option[j])
			{
			case 'f':
			case 'F':
				orderType = ORDER_NAME;
				showType |= (option[j] == 'F' ? SHOW_RORDER : 0);
				j++;
				break;

			case 'e':
			case 'E':
				orderType = ORDER_EXTN;
				showType |= (option[j] == 'E' ? SHOW_RORDER : 0);
				j++;
				break;
				
			case 's':
			case 'S':
				orderType = ORDER_SIZE;
				showType |= (option[j] == 'S' ? SHOW_RORDER : 0);
				j++;
				break;
				
			case 't':
			case 'T':
				orderType = ORDER_DATE;
				showType |= (option[j] == 'T' ? SHOW_RORDER : 0);
				j++;
				break;
				
			case 'n':
			case 'N':
				orderType = ORDER_NONE;
				showType |= (option[j] == 'N' ? SHOW_RORDER : 0);
				j++;
				break;

			case 'o':
			case 'O':
				orderType = ORDER_OWNR;
				showType |= (option[j] == 'O' ? SHOW_RORDER : 0);
				j++;
				break;

			case 'g':
			case 'G':
				orderType = ORDER_GRUP;
				showType |= (option[j] == 'G' ? SHOW_RORDER : 0);
				j++;
				break;

			case 'l':
			case 'L':
				orderType = ORDER_LINK;
				showType |= (option[j] == 'L' ? SHOW_RORDER : 0);
				j++;
				break;

			case 'c':
			case 'C':
				orderType = ORDER_CNXT;
				showType |= (option[j] == 'C' ? SHOW_RORDER : 0);
				j++;
				break;

			case 'm':
			case 'M':
				orderType = ORDER_MD5S;
				showType |= (option[j] == 'M' ? SHOW_RORDER : 0);
				j++;
				break;
			}
			break;

		case 'w':
			showType &= ~SHOW_PATH;
			showType |= SHOW_WIDE;
			break;

		case 'W':
			displayForceSize (255, 80);
			break;

		case 'p':
			showType &= ~SHOW_WIDE;
			showType |= SHOW_PATH;
			break;

		case 'm':
		case 'M':
			showType |= SHOW_MATCH;
			orderType = ORDER_SIZE;
			dirType &= ~(ONLYDIRS|ONLYLINKS|ONLYDEVS|ONLYSOCKS|ONLYPIPES);
			dirType |= ONLYFILES;
			if (option[j - 1] == 'M')
				showType |= SHOW_NO_MATCH;
			break;
			
		case 'a':
			dirType ^= SHOWALL;
			break;

		case 'b':
			showType ^= SHOW_BACKUP;
			break;

		case 'c':
			dirType ^= USECASE;
			break;
			
		case 'r':
			dirType ^= RECUDIR;
			break;

		case 's':
			switch (option[j])
			{
			case 'd':
				dirType &= ~(ONLYFILES|ONLYLINKS|ONLYDEVS|ONLYSOCKS|ONLYPIPES);
				dirType |= ONLYDIRS;
				j++;
				break;

			case 'f':
				dirType &= ~(ONLYDIRS|ONLYLINKS|ONLYDEVS|ONLYSOCKS|ONLYPIPES);
				dirType |= ONLYFILES;
				j++;
				break;

			case 'l':	
				dirType &= ~(ONLYDIRS|ONLYFILES|ONLYDEVS|ONLYSOCKS|ONLYPIPES);
				dirType |= ONLYLINKS;
				j++;
				break;
				
			case 'p':	
				dirType &= ~(ONLYDIRS|ONLYLINKS|ONLYFILES|ONLYDEVS|ONLYSOCKS);
				dirType |= ONLYPIPES;
				j++;
				break;

			case 's':	
				dirType &= ~(ONLYDIRS|ONLYLINKS|ONLYFILES|ONLYDEVS|ONLYPIPES);
				dirType |= ONLYSOCKS;
				j++;
				break;
				
			case 'x':	
				dirType &= ~(ONLYDIRS|ONLYLINKS|ONLYFILES|ONLYDEVS|ONLYSOCKS|ONLYPIPES);
				dirType |= ONLYEXECS;
				j++;
				break;

			case 'v':	
				dirType &= ~(ONLYDIRS|ONLYFILES|ONLYLINKS|ONLYSOCKS|ONLYPIPES);
				dirType |= ONLYDEVS;
				j++;
				break;
			}
			break;
			
		case 'q':
			showType ^= SHOW_QUIET;
			break;

		case 'Q':
			showType ^= (SHOW_QUIET | SHOW_QUOTE);
			break;

		case 'D':
			{
				int done = 0;
				while (!done)
				{					
					switch (option[j])
					{
					case 'd':
						showType ^= SHOW_DATE;
						j++;
						break;
					case 'e':
						showType ^= SHOW_EXTN;
						j++;
						break;
					case 'o':
						showType ^= SHOW_OWNER;
						j++;
						break;
					case 'g':
						showType ^= SHOW_GROUP;
						j++;
						break;
					case 'r':
						showType ^= SHOW_RIGHTS;
						j++;
						break;
					case 's':
						showType ^= SHOW_SIZE;
						j++;
						break;
					case 't':
						showType ^= SHOW_TYPE;
						j++;
						break;
					case 'l':
						showType ^= SHOW_LINK;
						j++;
						break;
					case 'n':
						showType ^= SHOW_NUM_LINKS;
						j++;
						break;
					case 'c':
						showType ^= SHOW_SELINUX;
						j++;
						break;
					case 'm':
						showType ^= SHOW_MD5;
						j++;
						break;
					default:
						done = 1;
						break;
					}
				}
			}
			break;

		case 'A':
			showType ^= SHOW_AGE;
			break;

		case 'd':
		{
			int m = 1;
			
			k = 0;
			showFound = 0;
			while (option[j + k])
			{
				if (k == 0 && option[j] == '-')
					m = -1;
				else if (option[j + k] >= '0' && option[j + k] <= '9')
					showFound = (showFound * 10) + (option[j + k] - '0');
				else
					break;
				k++;
			}
			showFound *= m;
			j += k;
		}
		break;
		
		case 't':
			switch (option[j])
			{
			case 'm':
				strcpy (dateType, "Mofified");
				showDate = DATE_MOD;
				j++;
				break;
				
			case 'a':
				strcpy (dateType, "Accessed");
				showDate = DATE_ACC;
				j++;
				break;
				
			case 'c':
				strcpy (dateType, "Changed");
				showDate = DATE_CHG;
				j++;
				break;
			}
			break;

		case 'S':
			sizeFormat = !sizeFormat;
			break;
			
		case '?':
			helpThem(progName);
			exit (1);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  S E T T I N G S                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load the settings from the (.
 *  \param progName The name of the program being run.
 *  \result None.
 */
void loadSettings (char *progName)
{
	int i, j;
	char *home = getenv ("HOME");
	char configPath[PATH_SIZE], value[81];
	
	configLoad ("/etc/ndirrc");
	strcpy (configPath, home);
	strcat (configPath, "/.ndirrc");
	configLoad (configPath);

	for (i = 0; i < MAX_COL_DESC; i++)
	{
		configGetIntValue (colourNames[i], &coloursAlt[i]);
	}	
	for (i = 0; i < MAX_W_COL_DESC; i++)
	{
		configGetIntValue (colourNames[i + MAX_COL_DESC], &coloursAlt[i + MAX_COL_DESC]);
	}	
	for (i = 0; i < EXTRA_COLOURS; i++)
	{
		configGetIntValue (colourNames[i + MAX_COL_DESC + MAX_W_COL_DESC], &colourType[i]);
	}
	if (configGetValue ("command_line", value))
	{
		i = j = 0;
		do
		{
			if (value[i] > ' ')
			{
				configPath[j ++] = value[i];
				configPath[j] = 0;
			}
			else if (j)
			{
				commandOption (configPath, progName);
				j = 0;
			}
			i++;
		}
		while (value[i - 1] != 0);
	}
	for (i = 0; i < 4; i++)
	{
		sprintf (configPath, "date_format_%d", i + 1);
		if (configGetValue (configPath, value))
			displaySetDateFormat (value, i);
	}
	configFree ();
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
	int i = 1, found = 0, foundDir = 0;
	void *fileList = NULL;
	char defaultDir[PATH_SIZE];

	timeNow = time (NULL);
	displayGetWindowSize ();
	loadSettings (basename (argv[0]));

	while (i < argc)
	{
		if (argv[i][0] == '-')
		{
			commandOption (&argv[i][1], basename (argv[0]));
		}
		else
			foundDir = 1;

		i++;
	}
	i = 1;

    /*------------------------------------------------------------------------*
     * If there was no path then do a directory of the current dir            *
     *------------------------------------------------------------------------*/
	if (!foundDir)
	{
		if (getcwd (defaultDir, 500) == NULL)
		{
			strcpy (defaultDir, ".");
		}
		strcat (defaultDir, DIRDEF);
		found = directoryLoad (defaultDir, dirType, fileCompare, &fileList);
	}
	else
	{
        /*--------------------------------------------------------------------*
         * If we got a path then split it into a path and a file pattern to   *
		 * match files with.                                                  *
         *--------------------------------------------------------------------*/
		while (i < argc)
		{
			if (argv[i][0] != '-')
			{
				found += directoryLoad (argv[i], dirType, fileCompare, &fileList);
			}
			i++;
		}
	}
	directorySort (&fileList);
	
    /*------------------------------------------------------------------------*
	 * We now have the directory loaded into memory.                          *
     *------------------------------------------------------------------------*/
	if (found)
	{
		if (showType & SHOW_WIDE)
		{
			unsigned long longestName = queueGetFreeData (fileList);

			if (longestName > 160)
				longestName = 160;
				
			if ((maxCol = (displayGetWidth () - 1) / (longestName + 2)) < 4)
				maxCol = 4;
			if (maxCol > 10)
				maxCol = 10;
			maxCol *= 3;

			if (!displayColumnInit (maxCol, ptrAllColumns, dirColour))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 1;
			}
		}
		else if (showType & SHOW_QUIET)
		{
			ptrAllColumns[0] = &allColumnDescs [COL_FILENAME];
			if (!displayColumnInit (1, ptrAllColumns, 0))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 1;
			}
		}
		else
		{
			int colNum;
			
			for (colNum = 0; colNum < MAX_COL_DESC; colNum++)
			{
				ptrAllColumns[colNum] = &allColumnDescs [columnTranslate[colNum]];
			}
			
			if (!displayColumnInit (colNum, ptrAllColumns, DISPLAY_HEADINGS | dirColour))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 1;
			}
		}
		directoryProcess (showDir, &fileList);
	}

	if (showType & SHOW_QUIET)
	{
		if (filesFound || linksFound || dirsFound || devsFound || socksFound || pipesFound)
		{
			displaySomeLines (showFound);
			displayTidy ();
		}
	}
	else
	{
	    /*--------------------------------------------------------------------*
		 * Finally we print out the totals                                    *
	     *--------------------------------------------------------------------*/
		if (filesFound || linksFound || dirsFound || devsFound || socksFound || pipesFound)
		{
			char foundBuff[11], sizeBuff[21];
		
			displayNewLine (0);
			if (showFound != 0)
				displayDrawLine (0);
			
			if (showType & SHOW_WIDE)
			{
				displayMatchWidth ();
				displayDrawLine (DISPLAY_FIRST);
				if (filesFound)
				{
					displayInColumn (1, "Files: %s", displayCommaNumber (filesFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
					displayInColumn (1, "Size:  %s", sizeFormat ? displayFileSize (totalSize, sizeBuff) : 
							displayCommaNumber (totalSize, sizeBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (linksFound)
				{
					displayInColumn (1, "Links: %s", displayCommaNumber (linksFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (dirsFound)
				{
					displayInColumn (1, "Dirs:  %s", displayCommaNumber (dirsFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (devsFound)
				{
					displayInColumn (1, "Devs:  %s", displayCommaNumber (devsFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (socksFound)
				{
					displayInColumn (1, "Devs:  %s", displayCommaNumber (socksFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (pipesFound)
				{
					displayInColumn (1, "Devs:  %s", displayCommaNumber (pipesFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
			}
			else if (showType & SHOW_PATH)
			{
				if (filesFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Files: %s", displayCommaNumber (filesFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
					displayInColumn (columnTranslate[COL_FILENAME], "Size:  %s", sizeFormat ? displayFileSize (totalSize, sizeBuff) : 
							displayCommaNumber (totalSize, sizeBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (linksFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Links: %s", displayCommaNumber (linksFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (dirsFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Dirs:  %s", displayCommaNumber (dirsFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (devsFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Devs:  %s", displayCommaNumber (devsFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (socksFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Socks: %s", displayCommaNumber (socksFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (pipesFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Pipes: %s", displayCommaNumber (pipesFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
			}
			else
			{
				if (filesFound)
				{
					if (showType & SHOW_SIZE)
					{				
						displayInColumn (columnTranslate[COL_SIZE], sizeFormat ? displayFileSize (totalSize, sizeBuff) : 
								displayCommaNumber (totalSize, sizeBuff));
					}
					displayInColumn (columnTranslate[COL_FILENAME], "Files: %s", displayCommaNumber (filesFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (linksFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Links: %s", displayCommaNumber (linksFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (dirsFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Dirs:  %s", displayCommaNumber (dirsFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (devsFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Devs:  %s", displayCommaNumber (devsFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (socksFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Socks: %s", displayCommaNumber (socksFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (pipesFound)
				{
					displayInColumn (columnTranslate[COL_FILENAME], "Pipes: %s", displayCommaNumber (pipesFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
			}
			displaySomeLines (showFound);
			displayTidy ();
		}
		else
		{
			version ();
			printf ("     No matches found\n");
		}
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I L E  E X I S T S                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Test to see if a file exists, used for checking links.
 *  \param fileName Files name to look for.
 *  \param extraPath If the file does not include a path use this.
 *  \result 1 if the file exists and 0 if not.
 */
int fileExists (char *fileName, char *extraPath)
{
	FILE *temp;
	int fileExists = 1;
	char tempBuff[1025];
	
	if (fileName[0] == DIRCHAR)
	{
		strcpy (tempBuff, fileName);
	}
	else
	{
		strcpy (tempBuff, extraPath);
		strcat (tempBuff, fileName);
	}
	if ((temp = fopen (tempBuff, "r")) == NULL)
	{
		if (errno == 2)
			fileExists = 0;
	}
	else
		fclose (temp);

	return fileExists;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I N D  E X T N                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function to find the .
 *  \param fileName Name of the file to search for the extension.
 *  \result NULL if no .
 */
char *findExtn (char *fileName)
{
	int i = strlen (fileName);
	char *extn = NULL;

	while (i > 1)
	{
		i --;
		if (fileName[i] == DIRCHAR)
			break;
		if (fileName[i] == '.')
		{
			extn = &fileName[i];
			break;
		}
	}	
	return extn;
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
	/*------------------------------------------------------------------------*
	 * Only show files that have matches, or only show files with no match.   *
	 *------------------------------------------------------------------------*/
	if (showType & SHOW_MATCH)
	{
		if (showType & SHOW_NO_MATCH)
		{
			if (file -> match)
				return 0;
		}
		else if (!file -> match)
			return 0;
	}

	/*------------------------------------------------------------------------*
	 * Don't show files ending in ~ as they are backup files.                 *
	 *------------------------------------------------------------------------*/
	if (!(showType & SHOW_BACKUP) && !(dirType & SHOWALL))
	{
		int n = strlen (file -> fileName);
		if (n)
		{
			if (file -> fileName[n - 1] == '~')
				return 0;
		}
	}

	/*------------------------------------------------------------------------*
	 * Show the directory in wide format.                                     *
	 *------------------------------------------------------------------------*/
	if (showType & SHOW_WIDE)
	{
		char marker1[2], marker2[2];
		int colour = -1;
		
		if (S_ISLNK (file -> fileStat.st_mode))
		{
			marker1[0] = '<';
			marker2[0] = '>';
			marker1[1] = marker2[1] = 0;
			colour = colourType[1];
			linksFound ++;
		}
		else if (S_ISDIR (file -> fileStat.st_mode))
		{
			marker1[0] = '[';
			marker2[0] = ']';
			marker1[1] = marker2[1] = 0;
			colour = colourType[0];
			dirsFound ++;
		}
		else if (!file -> fileStat.st_mode || file -> fileStat.st_mode & S_IFREG)
		{
			marker1[0] = ' ';
			marker2[0] = ' ';
			marker1[1] = marker2[1] = 0;
			totalSize += file -> fileStat.st_size;
			colour = colourType[2];
			if (file -> fileStat.st_mode & 0100)
				colour |= colourType[3];
			if (file -> fileStat.st_mode & 0200)
				colour |= colourType[4];
			if (file -> fileStat.st_mode & 0400)
				colour |= colourType[5];
			filesFound ++;
		}
		else if (S_ISBLK(file -> fileStat.st_mode) || S_ISCHR(file -> fileStat.st_mode))
		{
			marker1[0] = '{';
			marker2[0] = '}';
			marker1[1] = marker2[1] = 0;
			colour = colourType[6];
			devsFound ++;			
		}
		else if (S_ISSOCK(file -> fileStat.st_mode))
		{
			marker1[0] = '{';
			marker2[0] = '}';
			marker1[1] = marker2[1] = 0;
			colour = colourType[6];
			socksFound ++;			
		}
		else if (S_ISFIFO(file -> fileStat.st_mode))
		{
			marker1[0] = '{';
			marker2[0] = '}';
			marker1[1] = marker2[1] = 0;
			colour = colourType[6];
			pipesFound ++;			
		}
		else
			return 0;
			
		displayInColumn (currentCol++, marker1);
		displayInColour (currentCol++, colour, "%s", file -> fileName);
		displayInColumn (currentCol++, marker2);
		
		if (currentCol == maxCol)
		{
			displayNewLine (0);
			currentCol = 0;
		}
	}
	
	/*------------------------------------------------------------------------*
	 * Show the directory in quiet format.                                    *
	 *------------------------------------------------------------------------*/
	else if (showType & SHOW_QUIET)
	{
		char fullName[PATH_SIZE], displayName[PATH_SIZE];

		strcpy (displayName, file -> fileName);
		if (S_ISDIR (file -> fileStat.st_mode))
			strcat (displayName, "/");

		if (showType & SHOW_QUOTE)
		{
			if (showType & SHOW_PATH)
			{
				quoteCopy (fullName, file -> fullPath);
				quoteCopy (&fullName[strlen (fullName)], displayName);
			}
			else if (file -> partPath[0])
			{
				quoteCopy (fullName, file -> partPath);
				quoteCopy (&fullName[strlen (fullName)], displayName);
			}
			else
			{
				quoteCopy (fullName, displayName);
			}
			displayInColumn (0, "%s", fullName);
		}
		else
		{
			if (showType & SHOW_PATH)
				displayInColumn (0, "%s%s", file -> fullPath, displayName);
			else if (file -> partPath[0])
				displayInColumn (0, "%s%s", file -> partPath, displayName);
			else
				displayInColumn (0, "%s", displayName);
		}
		displayNewLine (0);
		filesFound ++;
	}
	
	/*------------------------------------------------------------------------*
	 * Show the directory in normal format.                                   *
	 *------------------------------------------------------------------------*/	
	else
	{
		char fullName[PATH_SIZE], displayName[PATH_SIZE];
		char dateString[41];
		char ownerString[81];
		char groupString[81];
		char contextString[81];
		char md5String[33];
		char rightsBuff[14];
		char numBuff[21];
		int fileAge = 0;
		
		strcpy (fullName, file -> fullPath);
		strcat (fullName, file -> fileName);
		strcpy (displayName, file -> partPath);
		strcat (displayName, file -> fileName);

		switch (showDate)
		{
		case DATE_MOD:
			fileAge = file -> fileStat.st_mtime;
			break;
		case DATE_ACC:
			fileAge = file -> fileStat.st_atime;
			break;
		case DATE_CHG:
			fileAge = file -> fileStat.st_ctime;
			break;
		}
				
		/*--------------------------------------------------------------------*
	   	 * If the file is a LINK then just them it is a link.                 *
		 *--------------------------------------------------------------------*/
		if (S_ISLNK (file -> fileStat.st_mode))
		{
			int linkSize;
			char linkBuff[1025];
			
			if (showType & SHOW_TYPE)
			{
				displayInColumn (COL_TYPE, "<Link>");
			}
			if (showType & SHOW_PATH)
			{
				displayInColour (columnTranslate[COL_FILENAME], colourType[1], "%s", fullName);
				if (showType & SHOW_LINK)
				{
					linkBuff[0] = 0;
					if ((linkSize = readlink(fullName, linkBuff, 1024)) >= 0)
						linkBuff[linkSize] = 0;

					displayInColumn (columnTranslate[COL_ARROW], "->");
					if (fileExists (linkBuff, file -> fullPath))
						displayInColumn (columnTranslate[COL_TARGET], "%s", linkBuff);
					else
						displayInColour (columnTranslate[COL_TARGET], colourType[7], "%s", linkBuff);
				}
			}
			else
			{
				if (showType & SHOW_RIGHTS)
				{
					displayInColumn (columnTranslate[COL_RIGHTS], displayRightsStringACL (file, rightsBuff));
				}
				if (showType & SHOW_NUM_LINKS)
				{
					displayInColumn (columnTranslate[COL_N_LINKS], "%s", displayCommaNumber (file -> fileStat.st_nlink, numBuff));
				}
				if (showType & SHOW_OWNER)
				{
					displayInColumn (columnTranslate[COL_OWNER], "%s", displayOwnerString (file -> fileStat.st_uid, ownerString));
				}
				if (showType & SHOW_GROUP)
				{
					displayInColumn (columnTranslate[COL_GROUP], "%s", displayGroupString (file -> fileStat.st_gid, groupString));
				}
				if (showType & SHOW_SELINUX)
				{
					displayInColumn (columnTranslate[COL_CONTEXT], "%s", displayContextString (fullName, contextString));
				}
				if (showType & SHOW_EXTN)
				{
					char *extn = findExtn (displayName);
					if (extn != NULL)
					{
						*extn = 0;
						displayInColour (columnTranslate[COL_EXTN], colourType[1], "%s", extn + 1);
					}
				}
				displayInColour (columnTranslate[COL_FILENAME], colourType[1], "%s", displayName);
				if (showType & SHOW_LINK)
				{
					linkBuff[0] = 0;
					if ((linkSize = readlink(fullName, linkBuff, 1024)) >= 0)
					{
						linkBuff[linkSize] = 0;
						displayInColumn (columnTranslate[COL_ARROW], "->");

						if (fileExists (linkBuff, file -> fullPath))
							displayInColumn (columnTranslate[COL_TARGET], "%s", linkBuff);
						else
							displayInColour (columnTranslate[COL_TARGET], colourType[7], "%s", linkBuff);
					}
				}
				if (showType & SHOW_DATE)
				{				
					if (showType & SHOW_AGE)
					{
						int age = timeNow - fileAge;
						
						displayInColumn (columnTranslate[COL_DAYS], "%d", age / 86400);
						displayInColumn (columnTranslate[COL_TIME], "%2d:%02d:%02d", 
								(age / 3600) % 24,
								(age / 60) % 60,
								 age % 60);
					}
					else
						displayInColumn (columnTranslate[COL_DATE], displayDateString (fileAge, dateString));
				}
			}
			displayNewLine (0);
			linksFound ++;
		}
		
		/*--------------------------------------------------------------------*
	   	 * If the file is a directory then just show them the name.           *
		 *--------------------------------------------------------------------*/
		else if (S_ISDIR (file -> fileStat.st_mode))
		{
			if (showType & SHOW_TYPE)
			{
				displayInColumn (columnTranslate[COL_TYPE], "<Dir>");
			}
			if (showType & SHOW_PATH)
			{
				displayInColour (columnTranslate[COL_FILENAME], colourType[0], "%s/", fullName);
			}
			else
			{
				if (showType & SHOW_RIGHTS)
				{
					displayInColumn (columnTranslate[COL_RIGHTS], displayRightsStringACL (file, rightsBuff));
				}
				if (showType & SHOW_NUM_LINKS)
				{
					displayInColumn (columnTranslate[COL_N_LINKS], "%s", displayCommaNumber (file -> fileStat.st_nlink, numBuff));
				}
				if (showType & SHOW_OWNER)
				{
					displayInColumn (columnTranslate[COL_OWNER], "%s", displayOwnerString (file -> fileStat.st_uid, ownerString));
				}		
				if (showType & SHOW_GROUP)
				{
					displayInColumn (columnTranslate[COL_GROUP], "%s", displayGroupString (file -> fileStat.st_gid, groupString));
				}
				if (showType & SHOW_SELINUX)
				{
					displayInColumn (columnTranslate[COL_CONTEXT], "%s", displayContextString (fullName, contextString));
				}
				displayInColour (columnTranslate[COL_FILENAME], colourType[0], "%s/", displayName);

				if (showType & SHOW_DATE)
				{				
					if (showType & SHOW_AGE)
					{
						int age = timeNow - fileAge;
					
						displayInColumn (columnTranslate[COL_DAYS], "%d", age / 86400);
						displayInColumn (columnTranslate[COL_TIME], "%2d:%02d:%02d", 
								(age / 3600) % 24,
								(age / 60) % 60,
								 age % 60);
					}
					else
						displayInColumn (columnTranslate[COL_DATE], displayDateString (fileAge, dateString));
				}
			}
			displayNewLine (0);
			dirsFound ++;
		}

		/*--------------------------------------------------------------------*
	   	 * If the file is a device then just show them the name.              *
		 *--------------------------------------------------------------------*/
		else if (S_ISBLK(file -> fileStat.st_mode) || S_ISCHR(file -> fileStat.st_mode) ||
				S_ISSOCK(file -> fileStat.st_mode) || S_ISFIFO(file -> fileStat.st_mode))
		{
			if (showType & SHOW_TYPE)
			{
				if (S_ISBLK(file -> fileStat.st_mode))
				{
					displayInColumn (columnTranslate[COL_TYPE], "<BlkD>");
					devsFound ++;			
				}
				else if (S_ISCHR(file -> fileStat.st_mode))
				{
					displayInColumn (columnTranslate[COL_TYPE], "<ChrD>");
					devsFound ++;
				}
				else if (S_ISSOCK(file -> fileStat.st_mode))
				{
					displayInColumn (columnTranslate[COL_TYPE], "<Sckt>");
					socksFound ++;			
				}
				else if (S_ISFIFO(file -> fileStat.st_mode))
				{
					displayInColumn (columnTranslate[COL_TYPE], "<Pipe>");					
					socksFound ++;
				}
			}
			if (showType & SHOW_PATH)
			{
				displayInColour (columnTranslate[COL_FILENAME], colourType[6], "%s", fullName);
			}
			else
			{
				if (showType & SHOW_RIGHTS)
				{
					displayInColumn (columnTranslate[COL_RIGHTS], displayRightsStringACL (file, rightsBuff));
				}
				if (showType & SHOW_NUM_LINKS)
				{
					displayInColumn (columnTranslate[COL_N_LINKS], "%s", displayCommaNumber (file -> fileStat.st_nlink, numBuff));
				}
				if (showType & SHOW_OWNER)
				{
					displayInColumn (columnTranslate[COL_OWNER], "%s", displayOwnerString (file -> fileStat.st_uid, ownerString));
				}		
				if (showType & SHOW_GROUP)
				{
					displayInColumn (columnTranslate[COL_GROUP], "%s", displayGroupString (file -> fileStat.st_gid, groupString));
				}
				if (showType & SHOW_SELINUX)
				{
					displayInColumn (columnTranslate[COL_CONTEXT], "%s", displayContextString (fullName, contextString));
				}
				if (showType & SHOW_EXTN)
				{
					char *extn = findExtn (displayName);
					if (extn != NULL)
					{
						*extn = 0;
						displayInColour (columnTranslate[COL_EXTN], colourType[6], "%s", extn + 1);
					}
				}
				displayInColour (columnTranslate[COL_FILENAME], colourType[6], "%s", displayName);

				if (showType & SHOW_DATE)
				{				
					if (showType & SHOW_AGE)
					{
						int age = timeNow - fileAge;
						
						displayInColumn (columnTranslate[COL_DAYS], "%d", age / 86400);
						displayInColumn (columnTranslate[COL_TIME], "%2d:%02d:%02d", 
								(age / 3600) % 24,
								(age / 60) % 60,
								 age % 60);
					}
					else
						displayInColumn (columnTranslate[COL_DATE], displayDateString (fileAge, dateString));
				}
			}
			displayNewLine (0);
		}
		
		/*--------------------------------------------------------------------*
   		 * If the file is a normal file then show the size/date/time.         *
		 *--------------------------------------------------------------------*/
		else if (!file -> fileStat.st_mode || file -> fileStat.st_mode & S_IFREG)
		{
			int colour = colourType[2];
			
			if (file -> fileStat.st_mode & 0100)
				colour |= colourType[3];
			if (file -> fileStat.st_mode & 0200)
				colour |= colourType[4];
			if (file -> fileStat.st_mode & 0400)
				colour |= colourType[5];
				
			if (showType & SHOW_PATH)
			{
				displayInColour (columnTranslate[COL_FILENAME], colour, "%s", fullName);
			}
			else
			{
				if (showType & SHOW_RIGHTS)
				{
					displayInColumn (columnTranslate[COL_RIGHTS], displayRightsStringACL (file, rightsBuff));
				}
				if (showType & SHOW_NUM_LINKS)
				{
					displayInColumn (columnTranslate[COL_N_LINKS], "%s", displayCommaNumber (file -> fileStat.st_nlink, numBuff));
				}
				if (showType & SHOW_OWNER)
				{
					displayInColumn (columnTranslate[COL_OWNER], displayOwnerString (file -> fileStat.st_uid, ownerString));
				}
				if (showType & SHOW_GROUP)
				{
					displayInColumn (columnTranslate[COL_GROUP], displayGroupString (file -> fileStat.st_gid, groupString));
				}
				if (showType & SHOW_SELINUX)
				{
					displayInColumn (columnTranslate[COL_CONTEXT], "%s", displayContextString (fullName, contextString));
				}
				if (showType & SHOW_MD5)
				{
					displayInColumn (columnTranslate[COL_MD5], "%s", displayMD5String (file, md5String));
				}
				if (showType & SHOW_EXTN)
				{
					char *extn = findExtn (displayName);
					if (extn != NULL)
					{
						*extn = 0;
						displayInColour (columnTranslate[COL_EXTN], colour, "%s", extn + 1);
					}
				}
				displayInColour (columnTranslate[COL_FILENAME], colour, "%s", displayName);

				if (showType & SHOW_SIZE)
				{
					long long fileSize = file -> fileStat.st_size;			
					displayInColumn (columnTranslate[COL_SIZE],	sizeFormat ? displayFileSize (fileSize, numBuff) : 
							displayCommaNumber (fileSize, numBuff));
				}				
				if (showType & SHOW_DATE)
				{				
					if (showType & SHOW_AGE)
					{
						int age = timeNow - fileAge;
						
						displayInColumn (columnTranslate[COL_DAYS], "%d", age / 86400);
						displayInColumn (columnTranslate[COL_TIME], "%2d:%02d:%02d", 
								(age / 3600) % 24,
								(age / 60) % 60,
								 age % 60);
					}
					else
						displayInColumn (columnTranslate[COL_DATE],	displayDateString (fileAge, dateString));
				}
			}
			displayNewLine (0);

			totalSize += file -> fileStat.st_size;
			filesFound ++;
		}
		else
			return 0;
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

	if (orderType == ORDER_NONE)
		return showType & SHOW_RORDER ? -1 : 1;

	/*------------------------------------------------------------------------*
	 * Force the directories to the begining of the list                      *
	 *------------------------------------------------------------------------*/
	if (S_ISDIR (fileOne -> fileStat.st_mode))
	{
		if (!(S_ISDIR (fileTwo -> fileStat.st_mode)))
			return -1;
	}
	if (S_ISDIR (fileTwo -> fileStat.st_mode))
	{
		if (!(S_ISDIR (fileOne -> fileStat.st_mode)))
			return 1;
	}
	
	/*------------------------------------------------------------------------*
	 * Force the links to follow the directories                              *
	 *------------------------------------------------------------------------*/
	if (S_ISLNK (fileOne -> fileStat.st_mode))
	{
		if (!(S_ISLNK (fileTwo -> fileStat.st_mode)))
			return -1;
	}
	if (S_ISLNK (fileTwo -> fileStat.st_mode))
	{
		if (!(S_ISLNK (fileOne -> fileStat.st_mode)))
			return 1;
	}
	
	/*------------------------------------------------------------------------*
	 * Force the block devices to follow the links                            *
	 *------------------------------------------------------------------------*/
	if (S_ISBLK(fileOne -> fileStat.st_mode))
	{
		if (!(S_ISBLK(fileTwo -> fileStat.st_mode)))
			return -1;
	}
	if (S_ISBLK(fileTwo -> fileStat.st_mode))
	{
		if (!(S_ISBLK(fileOne -> fileStat.st_mode)))
			return 1;	
	}

	/*------------------------------------------------------------------------*
	 * Force the char devices to follow the block devices                     *
	 *------------------------------------------------------------------------*/
	if (S_ISCHR(fileOne -> fileStat.st_mode))
	{
		if (!(S_ISCHR(fileTwo -> fileStat.st_mode)))
			return -1;
	}
	if (S_ISCHR(fileTwo -> fileStat.st_mode))
	{
		if (!(S_ISCHR(fileOne -> fileStat.st_mode)))
			return 1;	
	}

	/*------------------------------------------------------------------------*
	 * Force the sockets to follow the char devices                           *
	 *------------------------------------------------------------------------*/
	if (S_ISSOCK (fileOne -> fileStat.st_mode))
	{
		if (!(S_ISSOCK (fileTwo -> fileStat.st_mode)))
			return -1;
	}
	if (S_ISSOCK (fileTwo -> fileStat.st_mode))
	{
		if (!(S_ISSOCK (fileOne -> fileStat.st_mode)))
			return 1;
	}
	
	/*------------------------------------------------------------------------*
	 * Force the pipes to follow the sockets                                  *
	 *------------------------------------------------------------------------*/
	if (S_ISFIFO (fileOne -> fileStat.st_mode))
	{
		if (!(S_ISFIFO (fileTwo -> fileStat.st_mode)))
			return -1;
	}
	if (S_ISFIFO (fileTwo -> fileStat.st_mode))
	{
		if (!(S_ISFIFO (fileOne -> fileStat.st_mode)))
			return 1;
	}

	switch (orderType)
	{
/*	case ORDER_NONE:
		retn = 1;
		break;
*/		
	case ORDER_SIZE:
		if (showType & SHOW_MATCH)
		{
			if (fileOne -> fileStat.st_size == fileTwo -> fileStat.st_size)
			{
				int i;
				
				if (fileOne -> md5Sum == NULL)
				{
					if ((fileOne -> md5Sum = malloc (16)) != NULL)
					{
						strcpy (fullName, fileOne -> fullPath);
						strcat (fullName, fileOne -> fileName);
						MD5File (fullName, fileOne -> md5Sum);
					}
				}
				if (fileTwo -> md5Sum == NULL)
				{
					if ((fileTwo -> md5Sum = malloc (16)) != NULL)
					{
						strcpy (fullName, fileTwo -> fullPath);
						strcat (fullName, fileTwo -> fileName);
						MD5File (fullName, fileTwo -> md5Sum);
					}
				}
				for (i = 0; i < 16 && retn == 0; ++i)
				{
					if (fileOne -> md5Sum[i] != fileTwo -> md5Sum[i])
						break;
				}
				if (i == 16)
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
		switch (showDate)
		{
		case DATE_MOD:
			retn = (fileOne -> fileStat.st_mtime > fileTwo -> fileStat.st_mtime ? -1 :
					fileOne -> fileStat.st_mtime < fileTwo -> fileStat.st_mtime ? 1 : 0);
			break;
		case DATE_ACC:
			retn = (fileOne -> fileStat.st_atime > fileTwo -> fileStat.st_atime ? -1 :
					fileOne -> fileStat.st_atime < fileTwo -> fileStat.st_atime ? 1 : 0);
			break;
		case DATE_CHG:
			retn = (fileOne -> fileStat.st_ctime > fileTwo -> fileStat.st_ctime ? -1 :
					fileOne -> fileStat.st_ctime < fileTwo -> fileStat.st_ctime ? 1 : 0);
			break;
		}		
		break;

	case ORDER_EXTN:
		{
			char *extnOne = findExtn (fileOne -> fileName);
			char *extnTwo = findExtn (fileTwo -> fileName);
		
			if (extnOne != NULL && extnTwo != NULL)
			{
				if (dirType & USECASE)
					retn = strcmp (extnOne, extnTwo);
				else
					retn = strcasecmp (extnOne, extnTwo);
			}
			else if (extnOne != NULL && extnTwo == NULL)
			{
				retn = 1;
			}
			else if (extnOne == NULL && extnTwo != NULL)
			{
				retn = -1;
			}
		}
		break;

	case ORDER_OWNR:
		{
			char ownerOne[80], ownerTwo[80];
			displayOwnerString (fileOne -> fileStat.st_uid, ownerOne);
			displayOwnerString (fileTwo -> fileStat.st_uid, ownerTwo);
			retn = strcasecmp (ownerOne, ownerTwo);
		}
		break;

	case ORDER_GRUP:
		{
			char groupOne[80], groupTwo[80];
			displayGroupString (fileOne -> fileStat.st_gid, groupOne);
			displayGroupString (fileTwo -> fileStat.st_gid, groupTwo);
			retn = strcasecmp (groupOne, groupTwo);
		}
		break;

	case ORDER_LINK:
		retn = (fileOne -> fileStat.st_nlink > fileTwo -> fileStat.st_nlink ? -1 :
				fileOne -> fileStat.st_nlink < fileTwo -> fileStat.st_nlink ? 1 : 0);
		break;

	case ORDER_CNXT:
		{
			char contextOne[81], contextTwo[81];
			char fileNameOne[PATH_SIZE], fileNameTwo[PATH_SIZE];
		
			strcpy (fileNameOne, fileOne -> fullPath);
			strcat (fileNameOne, fileOne -> fileName);
			strcpy (fileNameTwo, fileTwo -> fullPath);
			strcat (fileNameTwo, fileTwo -> fileName);
			displayContextString (fileNameOne, contextOne);
			displayContextString (fileNameTwo, contextTwo);
			retn = strcasecmp (contextOne, contextTwo);
		}
		break;
		
	case ORDER_MD5S:
		if (fileOne -> md5Sum == NULL)
		{
			if ((fileOne -> md5Sum = malloc (16)) != NULL)
			{
				strcpy (fullName, fileOne -> fullPath);
				strcat (fullName, fileOne -> fileName);
				MD5File (fullName, fileOne -> md5Sum);
			}
		}
		if (fileTwo -> md5Sum == NULL)
		{
			if ((fileTwo -> md5Sum = malloc (16)) != NULL)
			{
				strcpy (fullName, fileTwo -> fullPath);
				strcat (fullName, fileTwo -> fileName);
				MD5File (fullName, fileTwo -> md5Sum);
			}
		}
		if (fileOne -> md5Sum != NULL && fileTwo -> md5Sum != NULL)
		{
			int i;
			for (i = 0; i < 16 && retn == 0; ++i)
			{
				retn = (fileOne -> md5Sum[i] < fileTwo -> md5Sum[i] ? -1 :
						fileOne -> md5Sum[i] > fileTwo -> md5Sum[i] ? 1 : 0);
			}
		}
		break;
	}
	if (retn == 0)
	{
		char fileNameOne[PATH_SIZE], fileNameTwo[PATH_SIZE];
		
		/*--------------------------------------------------------------------*/
		if (showType & SHOW_PATH)
		{
			strcpy (fileNameOne, fileOne -> fullPath);
			strcat (fileNameOne, fileOne -> fileName);
		}
		else if (fileOne -> partPath[0])
		{
			strcpy (fileNameOne, fileOne -> partPath);
			strcat (fileNameOne, fileOne -> fileName);
		}
		else
			strcpy (fileNameOne, fileOne -> fileName);

		/*--------------------------------------------------------------------*/
		if (showType & SHOW_PATH)
		{
			strcpy (fileNameTwo, fileTwo -> fullPath);
			strcat (fileNameTwo, fileTwo -> fileName);
		}
		else if (fileTwo -> partPath[0])
		{
			strcpy (fileNameTwo, fileTwo -> partPath);
			strcat (fileNameTwo, fileTwo -> fileName);
		}
		else
			strcpy (fileNameTwo, fileTwo -> fileName);

		/*--------------------------------------------------------------------*/
		if (dirType & USECASE)
			retn = strcmp (fileNameOne, fileNameTwo);
		else
			retn = strcasecmp (fileNameOne, fileNameTwo);
	}
	if (showType & SHOW_RORDER)
	{
		if (retn < 0)
			retn = 1;
		else if (retn > 0)
			retn = -1;
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  Q U O T E  C O P Y                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Copy a string quoting special chars.
 *  \param dst Destination pointer to the string.
 *  \param src Source pointer to the string.
 *  \result A pointer to the destination.
 */
char *quoteCopy (char *dst, char *src)
{
	int i, j;

	i = j = 0;	
	while (src[i])
	{
		if (strchr (quoteMe, src[i]))
			dst[j++] = '\\';
		dst[j++] = src[i];
		dst[j] = 0;
		++i;
	}
	return dst;
}

