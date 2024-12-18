/**********************************************************************************************************************
 *                                                                                                                    *
 *  L D I R . C                                                                                                       *
 *  ===========                                                                                                       *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File ldir.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of     *
 *  the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or  *
 *  (at your option) any later version.                                                                               *
 *                                                                                                                    *
 *  DirCmdUtils is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the         *
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for  *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see:           *
 *  <http://www.gnu.org/licenses/>                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**********************************************************************************************************************
 *  \file                                                                                                             *
 *  \brief Chris Knight's own directory program.                                                                      *
 **********************************************************************************************************************/
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
#ifdef HAVE_OPENSSL_EVP_H
#include <openssl/evp.h>
#define CRC_BUFF_SIZE EVP_MAX_MD_SIZE
#else
#define CRC_BUFF_SIZE 64
#endif
#ifdef HAVE_VALUES_H
#include <values.h>
#else
#define MAXINT 2147483647
#endif

#include <dircmd.h>
#include "buildDate.h"

extern int errno;

#ifdef DOS
#define DIRCHAR '\\'
#define DIRDEF "\\*"
#else
#define DIRCHAR '/'
#define DIRDEF "\057*"		/* Make the editor look nice */
#endif

/**---------------------------------------------------------------------------*
 * Prototypes															      *
 *---------------------------------------------------------------------------**/
int showDir (DIR_ENTRY *file);
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
char *quoteCopy (char *dst, char *src);
void getFileVersion (DIR_ENTRY *fileOne);

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
#define ORDER_SHAS		10
#define ORDER_INOD		11
#define ORDER_VERS		12
#define ORDER_NAVE		13
#define ORDER_WORD		14

#define SHOW_NORMAL		0
#define SHOW_WIDE		1
#define SHOW_AGE		(1 << 1)
#define SHOW_PATH		(1 << 2)
#define SHOW_MATCH		(1 << 3)
#define SHOW_RORDER		(1 << 4)
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
#define SHOW_SHA256		(1 << 20)
#define SHOW_INODE		(1 << 21)
#define SHOW_IN_AGE		(1 << 22)
#define SHOW_VERSION	(1 << 23)
#define SHOW_EXTRA		(1 << 24)

#define DATE_MOD		0
#define DATE_ACC		1
#define DATE_CHG		2
#ifdef USE_STATX
#define DATE_BTH		3
#endif

#define MAX_COL_DESC	18
#define MAX_W_COL_DESC	3
#define EXTRA_COLOURS	8

#define HELP_ALL		0
#define HELP_DATE		1
#define HELP_DISPLAY	2
#define HELP_ORDER		3
#define HELP_SHOW		4
#define HELP_TIME		5

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
int			orderType		=	ORDER_NAME;
int			showType		=	SHOW_NORMAL|SHOW_SIZE|SHOW_DATE|SHOW_EXTRA;
int			dirType			=	ALLFILES;
long		dirsFound		=	0;
long		filesFound		=	0;
long		linksFound		=	0;
long		devsFound		=	0;
long		socksFound		=	0;
long		pipesFound		=	0;
long long	totalSize		=	0;
int			currentCol		=	0;
int			maxCol			=	4;
int			showDate		=	DATE_MOD;
int			showFound		=	MAXINT;
int			sizeFormat		=	1;
int			dateFormat		=	1;
int			wordNumber		=	0;
char		dateType[41]	=	"Modified";
char		md5Type[41]		=	"MD5 Sum (hex)";
char		shaType[41]		=	"SHA256 Sum (hex)";
time_t		timeNow			=	0;
time_t		maxFileAge		=	-1;
time_t		minFileAge		=	-1;
char		*quoteMe		=	" *?|&;()<>#\t\\\"";
char		dot[3]			=	"";

int			coloursAlt[MAX_COL_DESC + MAX_W_COL_DESC];
int			colourType[EXTRA_COLOURS];
int			dirDisplayFlags = 0;
int			encode = DISPLAY_ENCODE_HEX;

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
#define		COL_SHA256		15
#define		COL_VERSION		16
#define		COL_INODE		17

#define		COL_W_TYPE_L	0
#define		COL_W_FILENAME	1
#define		COL_W_TYPW_R	2

int columnTranslate[MAX_COL_DESC] =
{
	COL_TYPE, COL_RIGHTS, COL_N_LINKS, COL_OWNER, COL_GROUP, COL_SIZE,
	COL_DATE, COL_DAYS, COL_TIME, COL_FILENAME, COL_EXTN, COL_ARROW,
	COL_TARGET, COL_CONTEXT, COL_MD5, COL_SHA256, COL_VERSION, COL_INODE
};

COLUMN_DESC allColumnDescs[MAX_COL_DESC] =
{
	{	6,	6,	0,	2,	0x03,	0,					"Type",		11	},	/*  0 */
#ifdef HAVE_SYS_ACL_H
	{	11, 11, 0,	2,	0x02,	0,					"Rights",	5	},	/*  1 */
#else
	{	10, 10, 0,	2,	0x02,	0,					"Rights",	5	},	/*  1 */
#endif
	{	20, 6,	0,	2,	0x04,	COL_ALIGN_RIGHT,	"Links",	8	},	/*  2 */
	{	20, 6,	0,	2,	0x05,	0,					"Owner",	6	},	/*  3 */
	{	20, 6,	0,	2,	0x05,	0,					"Group",	7	},	/*  8 */
	{	20, 7,	0,	2,	0x06,	COL_ALIGN_RIGHT,	"Size",		2	},	/*  4 */
	{	255,12, 0,	2,	0x02,	COL_ALIGN_RIGHT,	dateType,	3	},	/*  5 */
	{	7,	4,	0,	1,	0x02,	COL_ALIGN_RIGHT,	"Days",		3	},	/*  6 */
	{	255,8,	0,	2,	0x02,	0,					"HH:MM:SS", 4	},	/*  7 */
	{	255,12, 8,	2,	0x07,	0,					"Filename", 0	},	/*  9 */
	{	20, 3,	0,	2,	0x07,	0,					"Extn",		1	},	/* 10 */
	{	2,	2,	0,	2,	0x01,	0,					NULL,		10	},	/* 11 */
	{	255,12, 0,	2,	0x07,	0,					"Target",	9	},	/* 12 */
	{	80, 8,	0,	2,	0x05,	0,					"Context",	12	},	/* 13 */
	{	33, 33, 0,	2,	0x05,	0,					md5Type,	13	},	/* 14 */
	{	65, 65, 0,	2,	0x05,	0,					shaType,	14	},	/* 15 */
	{	200,7,	0,	2,	0x05,	0,					"Version",	15	},	/* 16 */
	{	20, 6,	0,	2,	0x04,	COL_ALIGN_RIGHT,	"iNode",	16	},	/* 17 */
};

COLUMN_DESC wideColumnDescs[MAX_W_COL_DESC] =
{
	{	1,	1,	0,	0,	0x00,	0,					NULL,		1},		/*  0 */
	{	255,12, 0,	0,	0x00,	0,					"Filename", 0},		/*  1 */
	{	1,	1,	0,	0,	0x00,	0,					NULL,		2},		/*  2 */
};

COLUMN_DESC *ptrAllColumns[30] =
{
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R],
	&wideColumnDescs[COL_W_TYPE_L], &wideColumnDescs[COL_W_FILENAME], &wideColumnDescs[COL_W_TYPW_R]
};

/*----------------------------------------------------------------------------*
 * The name of the colours in the cfg file                                    *
 *----------------------------------------------------------------------------*/
char *colourNames[] =
{
	"colour_type",		"colour_rights",	"colour_numlinks",	"colour_owner",		"colour_group",
	"colour_size",		"colour_date",		"colour_day",		"colour_time",		"colour_filename",
	"colour_extn",		"colour_linkptr",	"colour_target",	"colour_context",	"colour_md5",
	"colour_sha256",	"colour_version",	"colour_inode",

	"colour_wide_col1", "colour_wide_filename", "colour_wide_col2",

	"colour_directory", "colour_link",		"colour_base",		"colour_executable",
	"colour_read",		"colour_write",		"colour_other",		"colour_block"
};

static struct option longOptions[] =
{
	{	"all",			no_argument,		0,	'a' },
	{	"age",			no_argument,		0,	'A' },
	{	"backup",		no_argument,		0,	'b' },
	{	"base64",		no_argument,		0,	'B' },
	{	"case",			no_argument,		0,	'c' },
	{	"colour",		no_argument,		0,	'C' },
	{	"date",			required_argument,	0,	'd' },
	{	"number",		required_argument,	0,	'n' },
	{	"display",		required_argument,	0,	'D' },
	{	"epoch",		no_argument,		0,	'e' },
	{	"matching",		no_argument,		0,	'm' },
	{	"unique",		no_argument,		0,	'M' },
	{	"order",		required_argument,	0,	'o' },
	{	"path",			no_argument,		0,	'p' },
	{	"pages",		no_argument,		0,	'P' },
	{	"quiet",		no_argument,		0,	'q' },
	{	"quote",		no_argument,		0,	'Q' },
	{	"recursive",	no_argument,		0,	'r' },
	{	"recurlink",	no_argument,		0,	'R' },
	{	"show",			required_argument,	0,	's' },
	{	"size",			no_argument,		0,	'S' },
	{	"thousep",		no_argument,		0,	't' },
	{	"time",			required_argument,	0,	'T' },
	{	"version",		no_argument,		0,	'v' },
	{	"nocvs",		no_argument,		0,	'V' },
	{	"wide",			no_argument,		0,	'w' },
	{	"width",		required_argument,	0,	'W' },
	{	"word",			required_argument,	0,	'x' },
	{	"extra",		no_argument,		0,	'X' },
	{	"help",			no_argument,		0,	'?' },
	{	0,				0,					0,	0	}
};

typedef struct
{
	int flag;
	char letter;
	const char *word;
}
CONFIG_WORD;

CONFIG_WORD configWords[] =
{
	{	3,	'c',	"context"	},
	{	3,	'd',	"date"		},
	{	3,	'e',	"extn"		},
	{	1,	'f',	"file"		},
	{	3,	'g',	"group"		},
	{	3,	'h',	"SHA"		},
	{	3,	'i',	"inode"		},
	{	3,	'v',	"ver"		},
	{	3,	'V',	"namever"	},
	{	3,	'l',	"links"		},
	{	3,	'm',	"MD5"		},
	{	1,	'n',	"none"		},
	{	3,	'o',	"owner"		},
	{	3,	's',	"size"		},
	{	3,	'w',	"word"		},
	{	1,	'r',	"rev"		},
	{	2,	'n',	"num"		},
	{	2,	't',	"type"		},
	{	2,	'r',	"rights"	},
	{	4,	'd',	"dirs"		},
	{	4,	'f',	"files"		},
	{	4,	'l',	"links"		},
	{	4,	'p',	"pipes"		},
	{	4,	's',	"sockets"	},
	{	4,	'v',	"devices"	},
	{	4,	'x',	"execs"		},
	{	8,	'a',	"accessed"	},
	{	8,	'c',	"changed"	},
	{	8,	'm',	"modified"	},
#ifdef USE_STATX
	{	8,	'b',	"birth"		},
#endif
	{	0,	'\0',	""			},
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I N D  W O R D                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Look in the list of words for any matching for this menu.
 *  \param flag Flag for the current option.
 *  \param word Word to look for.
 *  \result Pointer to a word description.
 */
CONFIG_WORD *findWord (int flag, char *word)
{
	int i = 0;
	while (configWords[i].flag)
	{
		if (flag & configWords[i].flag)
		{
			if (strncmp (configWords[i].word, word, strlen (configWords[i].word) + 1) == 0)
			{
				break;
			}
		}
		++i;
	}
	return &configWords[i];
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A L L  V A L I D                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Check to see if word contains only valid options.
 *  \param flag Which options to check.
 *  \param word Word containing options.
 *  \result 1 if they are all valid.
 */
int allValid (int flag, char *word)
{
	int i = 0, j = 0, retn = 1;
	while (word[j] && retn == 1)
	{
		i = 0;
		while (configWords[i].flag)
		{
			if (flag & configWords[i].flag)
			{
				if (configWords[i].letter == word[j])
					break;
			}
			++i;
		}
		if (configWords[i].letter != word[j])
		{
			retn = 0;
		}
		else
		{
			++j;
		}
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V E R S I O N                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display version information for the library.
 *  \param showLine ????.
 *  \result None.
 */
void version (int showLine)
{
	printf ("TheKnight: Linux Directory, Version: %s, Built: %s\n", VERSION, buildDate);
	if (showLine)
	{
		displayLine ();
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
 *  \param progName The name of the program being run.
 *  \param flags What help to show.
 *  \result None.
 */
void helpThem (char *progName, int flags)
{
	version (1);
	printf ("%s -[Options] [FileNames] [FileNames]...\n", progName);
	printf ("\nOptions:\n");
	if (flags == 0)
	{
		printf ("     --all . . . . . . . . . -a  . . . . . Show all files including hidden files.\n");
		printf ("     --age . . . . . . . . . -A  . . . . . Show the age of the file.\n");
		printf ("     --backup  . . . . . . . -b  . . . . . Show backup files ending with ~.\n");
		printf ("     --base64  . . . . . . . -B  . . . . . Encode checksums in base64.\n");
		printf ("     --case  . . . . . . . . -c  . . . . . Should the sort be case sensitive.\n");
		printf ("     --colour  . . . . . . . -C  . . . . . Toggle colour display, defined in dirrc.\n");
	}
	if (flags == 0 || flags == HELP_DATE)	/* Date */
	{
		printf ("     --date accessed . . . . -da . . . . . Show time of last access.\n");
#ifdef USE_STATX
		printf ("     --date birth  . . . . . -db . . . . . Show time of file creation.\n");
#endif
		printf ("     --date changed  . . . . -dc . . . . . Show time of last status change.\n");
		printf ("     --date modified . . . . -dm . . . . . Show time of last modification.\n");
	}
	if (flags == 0 || flags == HELP_DISPLAY)	/* Display */
	{
		printf ("     --display context . . . -Dc . . . . . Show the context of the file.\n");
		printf ("     --display date  . . . . -Dd . . . . . Show the date of the file.\n");
		printf ("     --display extn  . . . . -De . . . . . Show the file extension.\n");
		printf ("     --display group . . . . -Dg . . . . . Show the group of the file.\n");
		printf ("     --display inode . . . . -Di . . . . . Show the inode of the file.\n");
		printf ("     --display links . . . . -Dl . . . . . Show the target of the link.\n");
		printf ("     --display MD5 . . . . . -Dm . . . . . Show the MD5 checksum of the file.\n");
		printf ("     --display SHA . . . . . -Dh . . . . . Show the SHA256 checksum of the file.\n");
		printf ("     --display num . . . . . -Dn . . . . . Show the number of links.\n");
		printf ("     --display owner . . . . -Do . . . . . Show the owner of the file.\n");
		printf ("     --display rights  . . . -Dr . . . . . Show the user rights of the file.\n");
		printf ("     --display size  . . . . -Ds . . . . . Show the size of the file.\n");
		printf ("     --display type  . . . . -Dt . . . . . Show the type of the file.\n");
		printf ("     --display ver . . . . . -Dv . . . . . Show the version from file name.\n");
	}
	if (flags == 0)
	{
		printf ("     --epoch . . . . . . . . -e  . . . . . Show date in epoch with milli seconds.\n");
		printf ("     --matching  . . . . . . -m  . . . . . Show only duplicated files.\n");
		printf ("     --unique  . . . . . . . -M  . . . . . Show only files with no duplicate.\n");
		printf ("     --number #  . . . . . . -n# . . . . . Display some, # > 0 first #, # < 0 last n.\n");
	}
	if (flags == 0 || flags == HELP_ORDER)	/* Order */
	{
		printf ("     --order context . . . . -oc . . . . . Order the files by context.\n");
		printf ("     --order date  . . . . . -od . . . . . Order the files by time and date.\n");
		printf ("     --order extn  . . . . . -oe . . . . . Order the files by extension.\n");
		printf ("     --order file  . . . . . -of . . . . . Order by the file name (default).\n");
		printf ("     --order group . . . . . -og . . . . . Order the files by group name.\n");
		printf ("     --order SHA . . . . . . -oh . . . . . Order by the SHA256 checksum.\n");
		printf ("     --order inode . . . . . -oi . . . . . Order by the iNode number.\n");
		printf ("     --order links . . . . . -ol . . . . . Order by the number of hard links.\n");
		printf ("     --order MD5 . . . . . . -om . . . . . Order by the MD5 checksum.\n");
		printf ("     --order none  . . . . . -on . . . . . Do not order, use directory order.\n");
		printf ("     --order owner . . . . . -oo . . . . . Order the files by owners name.\n");
		printf ("     --order size  . . . . . -os . . . . . Order the files by size.\n");
		printf ("     --order rev . . . . . . -or . . . . . Reverse the current sort order.\n");
		printf ("     --order ver . . . . . . -ov . . . . . Order by numbers in the file name.\n");
		printf ("     --order namever . . . . -oV . . . . . Order by name then numbers.\n");
		printf ("     --order word  . . . . . -ow . . . . . Order by word selected by -x.\n");
	}
	if (flags == 0)
	{
		printf ("     --path  . . . . . . . . -p  . . . . . Show the full path to the file.\n");
		printf ("     --pages . . . . . . . . -P  . . . . . Stop at the end of each page (q to quit).\n");
		printf ("     --quiet . . . . . . . . -q  . . . . . Quiet mode, only paths and file names.\n");
		printf ("     --quote . . . . . . . . -Q  . . . . . Quote special chars.\n");
		printf ("     --recursive . . . . . . -r  . . . . . Recursive directory listing.\n");
		printf ("     --recurlink . . . . . . -R  . . . . . Recursive including links.\n");
	}
	if (flags == 0 || flags == HELP_SHOW)	/* Show */
	{
		printf ("     --show dirs . . . . . . -sd . . . . . Show only directories.\n");
		printf ("     --show files  . . . . . -sf . . . . . Show only files.\n");
		printf ("     --show links  . . . . . -sl . . . . . Show only links.\n");
		printf ("     --show pipes  . . . . . -sp . . . . . Show only pipes.\n");
		printf ("     --show sockets  . . . . -ss . . . . . Show only sockets.\n");
		printf ("     --show devices  . . . . -sv . . . . . Show only devices.\n");
		printf ("     --show execs  . . . . . -sx . . . . . Show only executable files.\n");
	}
	if (flags == 0)
	{
		printf ("     --size  . . . . . . . . -S  . . . . . Show the file size in full.\n");
		printf ("     --thousep . . . . . . . -t  . . . . . Do not display the thousand seperator.\n");
	}
	if (flags == 0 || flags == HELP_TIME)	/* Time */
	{
		printf ("     --time g{time}  . . . . -Tg{time} . . Show when file age > #d#h#m#s.\n");
		printf ("     --time l{time}  . . . . -Tl{time} . . Show when file age < #d#h#m#s.\n");
	}
	if (flags == 0)
	{
		printf ("     --version . . . . . . . -v  . . . . . Show version information.\n");
		printf ("     --nocvs . . . . . . . . -V  . . . . . Do not show version control directories.\n");
		printf ("     --wide  . . . . . . . . -w  . . . . . Show directory in wide format.\n");
		printf ("     --width # . . . . . . . -W# . . . . . Ignore screen width default to 255.\n");
		printf ("     --word #  . . . . . . . -x# . . . . . Which word to start word sort from.\n");
		printf ("     --extra . . . . . . . . -X  . . . . . Turn on and off directory extras.\n");
		printf ("     --help  . . . . . . . . -?  . . . . . Show this help message.\n");
		printf ("\nExpressions:\n");
		printf ("     & . . . . . . . . Logical AND, eg. %s \"c*&*c\"\n", progName);
		printf ("     | . . . . . . . . Logical OR,  eg. %s \"c*|d*\"\n", progName);
		printf ("     ^ . . . . . . . . Logical NOT, eg. %s \"c*&^*c\"\n", progName);
	}
	displayLine ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P A R S E  T I M E                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a string in the format XXhXXmXXs in to seconds.
 *  \param timeStr String to convert.
 *  \param len Return the number of character used.
 *  \result The number of seconds specified in the string.
 */
time_t parseTime (char *timeStr, int *len)
{
	unsigned int l = 0, done = 0, used = 0;
	time_t running = 0, current = 0;

	if (len != NULL)
	{
		*len = 0;
	}
	while (timeStr[l] && !done)
	{
		switch (timeStr[l])
		{
		case 'd':
			running += (current * (24 * 60 * 60));
			current = 0;
			used = l + 1;
			break;
		case 'h':
			running += (current * (60 * 60));
			current = 0;
			used = l + 1;
			break;
		case 'm':
			running += (current * 60);
			current = 0;
			used = l + 1;
			break;
		case 's':
			running += current;
			current = 0;
			used = l + 1;
			break;
		default:
			if (timeStr[l] >= '0' && timeStr[l] <= '9')
			{
				current = (current * 10) + (timeStr[l] - '0');
			}
			else
			{
				done = 1;
				continue;
			}
			break;
		}
		++l;
	}
	if (len != NULL)
	{
		*len = used;
	}
	return running;
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
 *  \param optionVal The value that goes with the option.
 *  \param progName The name of the program being run.
 *  \result None.
 */
void commandOption (char option, char *optionVal, char *progName)
{
	switch (option)
	{
	case 'C':
		if (dirDisplayFlags & DISPLAY_COLOURS)
		{
			dirDisplayFlags &= ~DISPLAY_COLOURS;
		}
		else
		{
			int k;
			for (k = 0; k < MAX_COL_DESC; k++)
			{
				allColumnDescs[k].colour = coloursAlt[k];
			}
			for (k = 0; k < MAX_W_COL_DESC; k++)
			{
				wideColumnDescs[k].colour = coloursAlt[k + MAX_COL_DESC];
			}
			dirDisplayFlags |= DISPLAY_COLOURS;
		}
		break;

	case 'o':
		if (optionVal != NULL)
		{
			char letter = 0;
			int all = 0, j = 0;
			CONFIG_WORD *found = findWord (1, optionVal);

			if (found -> letter)
			{
				letter = found -> letter;
			}
			else if ((all = allValid (1, optionVal)) == 0)
			{
				helpThem (progName, HELP_ORDER);
				exit(1);
			}
			do
			{
				if (all)
				{
					letter = optionVal[j];
				}
				switch (letter)
				{
				case 'f':
					orderType = ORDER_NAME;
					break;
				case 'e':
					orderType = ORDER_EXTN;
					break;
				case 's':
					orderType = ORDER_SIZE;
					break;
				case 'd':
					orderType = ORDER_DATE;
					break;
				case 'n':
					orderType = ORDER_NONE;
					break;
				case 'o':
					orderType = ORDER_OWNR;
					break;
				case 'g':
					orderType = ORDER_GRUP;
					break;
				case 'l':
					orderType = ORDER_LINK;
					break;
				case 'c':
					orderType = ORDER_CNXT;
					break;
				case 'm':
					orderType = ORDER_MD5S;
					break;
				case 'h':
					orderType = ORDER_SHAS;
					break;
				case 'i':
					orderType = ORDER_INOD;
					break;
				case 'r':
					showType |= SHOW_RORDER;
					break;
				case 'v':
					orderType = ORDER_VERS;
					break;
				case 'V':
					orderType = ORDER_NAVE;
					break;
				case 'w':
					orderType = ORDER_WORD;
					break;
				}
			}
			while (optionVal[++j] != 0 && all);
		}
		break;

	case 'w':
		showType &= ~SHOW_PATH;
		showType |= SHOW_WIDE;
		break;

	case 'W':
		if (optionVal != NULL)
		{
			int width = 0, j = 0, k = 0;
			while (optionVal[j + k])
			{
				if (optionVal[j + k] >= '0' && optionVal[j + k] <= '9')
					width = (width * 10) + (optionVal[j + k] - '0');
				else
					break;
				++k;
			}
			j += k;
			displayForceSize (width == 0 ? 255 : width, 80);
		}
		break;

	case 'p':
		showType &= ~SHOW_WIDE;
		showType |= SHOW_PATH;
		break;

	case 'P':
		dirDisplayFlags ^= DISPLAY_IN_PAGES;
		break;

	case 't':
		dirDisplayFlags ^= DISPLAY_NO_THOUSEP;
		break;

	case 'm':
	case 'M':
		showType |= SHOW_MATCH;
		orderType = ORDER_SIZE;
		dirType &= ~(ONLYDIRS|ONLYLINKS|ONLYDEVS|ONLYSOCKS|ONLYPIPES);
		dirType |= ONLYFILES;
		if (option == 'M')
			showType |= SHOW_NO_MATCH;
		break;

	case 'T':
		if (optionVal != NULL)
		{
			int len = 0, j = 0;
			showType |= SHOW_IN_AGE;
			while (j < strlen (optionVal))
			{
				switch (optionVal[j++])
				{
				case 'l':
					maxFileAge = timeNow - parseTime(&optionVal[j], &len);
					break;
				case 'g':
					minFileAge = timeNow - parseTime(&optionVal[j], &len);
					break;
				default:
					helpThem (progName, HELP_TIME);
					exit (1);
				}
				j += len;
			}
		}
		break;

	case 'a':
		dirType ^= SHOWALL;
		break;

	case 'b':
		showType ^= SHOW_BACKUP;
		break;

	case 'B':
		if (encode == DISPLAY_ENCODE_BASE64)
		{
			strcpy (md5Type, "MD5 Sum (hex)");
			strcpy (shaType, "SHA256 Sum (hex)");
			encode = DISPLAY_ENCODE_HEX;
		}
		else
		{
			strcpy (md5Type, "MD5 Sum (base64)");
			strcpy (shaType, "SHA256 Sum (base64)");
			encode = DISPLAY_ENCODE_BASE64;
		}
		break;

	case 'c':
		dirType ^= USECASE;
		break;

	case 'e':
		dateFormat = !dateFormat;
		break;

	case 'r':
		dirType ^= RECUDIR;
		break;

	case 'R':
		dirType ^= RECULINK;
		break;

	case 's':
		if (optionVal != NULL)
		{
			char letter = 0;
			int all = 0, j = 0;
			CONFIG_WORD *found = findWord (4, optionVal);

			if (found -> letter)
			{
				letter = found -> letter;
			}
			else if ((all = allValid (4, optionVal)) == 0)
			{
				helpThem (progName, HELP_SHOW);
				exit(1);
			}
			dirType &= ~(ONLYDIRS|ONLYFILES|ONLYLINKS|ONLYDEVS|ONLYSOCKS|ONLYPIPES|ONLYEXECS);
			do
			{
				if (all)
				{
					letter = optionVal[j];
				}
				switch (letter)
				{
				case 'd':
					dirType |= ONLYDIRS;
					break;
				case 'f':
					dirType |= ONLYFILES;
					break;
				case 'l':
					dirType |= ONLYLINKS;
					break;
				case 'p':
					dirType |= ONLYPIPES;
					break;
				case 's':
					dirType |= ONLYSOCKS;
					break;
				case 'x':
					dirType |= ONLYEXECS;
					break;
				case 'v':
					dirType |= ONLYDEVS;
					break;
				}
			}
			while (optionVal[++j] != 0 && all);
		}
		break;

	case 'q':
		showType ^= SHOW_QUIET;
		break;

	case 'Q':
		showType ^= SHOW_QUOTE;
		break;

	case 'D':
		if (optionVal != NULL)
		{
			char letter = 0;
			int all = 0, j = 0;
			CONFIG_WORD *found = findWord (2, optionVal);

			if (found -> letter)
			{
				letter = found -> letter;
			}
			else if ((all = allValid (2, optionVal)) == 0)
			{
				helpThem (progName, HELP_DISPLAY);
				exit(1);
			}
			do
			{
				if (all)
				{
					letter = optionVal[j];
				}
				switch (letter)
				{
				case 'd':
					showType ^= SHOW_DATE;
					break;
				case 'e':
					showType ^= SHOW_EXTN;
					break;
				case 'o':
					showType ^= SHOW_OWNER;
					break;
				case 'g':
					showType ^= SHOW_GROUP;
					break;
				case 'r':
					showType ^= SHOW_RIGHTS;
					break;
				case 's':
					showType ^= SHOW_SIZE;
					break;
				case 't':
					showType ^= SHOW_TYPE;
					break;
				case 'l':
					showType ^= SHOW_LINK;
					break;
				case 'n':
					showType ^= SHOW_NUM_LINKS;
					break;
				case 'c':
					showType ^= SHOW_SELINUX;
					break;
				case 'm':
					showType ^= SHOW_MD5;
					break;
				case 'h':
					showType ^= SHOW_SHA256;
					break;
				case 'i':
					showType ^= SHOW_INODE;
					break;
				case 'v':
					showType ^= SHOW_VERSION;
					break;
				}
			}
			while (optionVal[++j] != 0 && all);
		}
		break;

	case 'A':
		showType ^= SHOW_AGE;
		break;

	case 'n':
		if (optionVal != NULL)
		{
			int m = 1, k = 0;
			showFound = 0;
			while (optionVal[k])
			{
				if (k == 0 && optionVal[k] == '-')
					m = -1;
				else if (optionVal[k] >= '0' && optionVal[k] <= '9')
					showFound = (showFound * 10) + (optionVal[k] - '0');
				else
					break;
				k++;
			}
			showFound *= m;
		}
		break;

	case 'd':
		if (optionVal != NULL)
		{
			char letter = 0;
			int all = 0, j = 0;
			CONFIG_WORD *found = findWord (8, optionVal);

			if (found -> letter)
			{
				letter = found -> letter;
			}
			else if ((all = allValid (8, optionVal)) == 0)
			{
				helpThem (progName, HELP_DATE);
				exit(1);
			}
			do
			{
				if (all)
				{
					letter = optionVal[j];
				}
				switch (letter)
				{
				case 'm':
					strcpy (dateType, "Mofified");
					showDate = DATE_MOD;
					break;
				case 'a':
					strcpy (dateType, "Accessed");
					showDate = DATE_ACC;
					break;
#ifdef USE_STATX
				case 'b':
					strcpy (dateType, "Birth");
					showDate = DATE_BTH;
					break;
#endif
				case 'c':
					strcpy (dateType, "Changed");
					showDate = DATE_CHG;
					break;
				}
			}
			while (optionVal[++j] != 0 && all);
		}
		break;

	case 'S':
		sizeFormat = !sizeFormat;
		break;

	case 'v':
		version (0);
		exit (0);

	case 'V':
		dirType ^= HIDEVERCTL;
		break;

	case 'x':
		if (optionVal != NULL)
		{
			int word = 0, k = 0, s = 1;
			if (optionVal[k] == '-')
			{
				s = -1;
				++k;
			}
			while (optionVal[k])
			{
				if (optionVal[k] >= '0' && optionVal[k] <= '9')
					word = (word * 10) + (optionVal[k] - '0');
				else
					break;
				++k;
			}
			wordNumber = word * s;
		}
		break;

	case 'X':
		showType ^= SHOW_EXTRA;
		break;

	case '?':
		helpThem(progName, HELP_ALL);
		exit (1);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  S E T T I N G S                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load the settings from the config files, /etc/ldirrc then HOME/.
 *  \param progName The name of the program being run.
 *  \result None.
 */
void loadSettings (char *progName)
{
	int i, j;
	char *home = getenv ("HOME");
	char configPath[PATH_SIZE], value[81];

	configLoad ("/etc/ldirrc");
	strcpy (configPath, home);
	strcat (configPath, "/.ldirrc");
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
	if (configGetValue ("command_line", value, 80))
	{
		i = j = 0;
		do
		{
			if (value[i] > ' ')
			{
				configPath[j++] = value[i];
				configPath[j] = 0;
			}
			else if (j)
			{
				commandOption (configPath[0], &configPath[1], progName);
				j = 0;
			}
			i++;
		}
		while (value[i - 1] != 0);
		if (j)
		{
			commandOption (configPath[0], &configPath[1], progName);
		}
	}
	for (i = 0; i < 4; i++)
	{
		sprintf (configPath, "date_format_%d", i + 1);
		if (configGetValue (configPath, value, 80))
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
	int found = 0, foundDir = 0, opt;
	void *fileList = NULL;
	char defaultDir[PATH_SIZE], fullVersion[81];

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

	timeNow = time (NULL);
	displayInit ();
	loadSettings (basename (argv[0]));

	while (1)
	{
		/*--------------------------------------------------------------------*
		 * getopt_long stores the option index here.                          *
	     *--------------------------------------------------------------------*/
		int optionIndex = 0;

		opt = getopt_long (argc, argv, "aAbBcCd:D:emMn:o:pPqQrRs:StT:vVwW:x:X?", longOptions, &optionIndex);

		/*--------------------------------------------------------------------*
		 * Detect the end of the options.                                     *
	     *--------------------------------------------------------------------*/
		if (opt == -1) break;

		switch (opt)
		{
		case 'd':
		case 'D':
		case 'o':
		case 's':
		case 'n':
		case 'T':
		case 'W':
		case 'x':
		case 'X':
			commandOption (opt, optarg, basename (argv[0]));
			break;

		default:
			commandOption (opt, NULL, basename (argv[0]));
			break;

		}
	}

	/*------------------------------------------------------------------------*
	 * Print any remaining command line arguments (not options).              *
     *------------------------------------------------------------------------*/
	while (optind < argc)
	{
		found += directoryLoad (argv[optind++], dirType, fileCompare, &fileList);
		foundDir = 1;
	}
	if (!foundDir)
	{
		if (getcwd (defaultDir, 500) == NULL)
		{
			strcpy (defaultDir, ".");
		}
		strcat (defaultDir, DIRDEF);
		found = directoryLoad (defaultDir, dirType, fileCompare, &fileList);
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

			if (!displayColumnInit (maxCol, ptrAllColumns, dirDisplayFlags))
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
			int colNum, flags = (showType & SHOW_EXTRA) ? DISPLAY_HEADINGS : 0;

			for (colNum = 0; colNum < MAX_COL_DESC; colNum++)
			{
				ptrAllColumns[colNum] = &allColumnDescs [columnTranslate[colNum]];
			}

			if (!displayColumnInit (colNum, ptrAllColumns, flags | dirDisplayFlags))
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
			if (showFound != 0 && showType & SHOW_EXTRA)
				displayDrawLine (0);

			if (showType & SHOW_WIDE)
			{
				displayMatchWidth ();
				if (showType & SHOW_EXTRA)
					displayDrawLine (DISPLAY_FIRST);
				if (filesFound && showType & SHOW_EXTRA)
				{
					displayInColumn (1, "Files: %s", displayCommaNumber (filesFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
					displayInColumn (1, "Size:  %s", sizeFormat ? displayFileSize (totalSize, sizeBuff) :
							displayCommaNumber (totalSize, sizeBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (showType & SHOW_EXTRA)
				{
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
			}
			else if (showType & SHOW_PATH && showType & SHOW_EXTRA)
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
				if (filesFound && showType & SHOW_EXTRA)
				{
					if (showType & SHOW_SIZE)
					{
						displayInColumn (columnTranslate[COL_SIZE], sizeFormat ? displayFileSize (totalSize, sizeBuff) :
								displayCommaNumber (totalSize, sizeBuff));
					}
					displayInColumn (columnTranslate[COL_FILENAME], "Files: %s", displayCommaNumber (filesFound, foundBuff));
					displayNewLine(DISPLAY_INFO);
				}
				if (showType & SHOW_EXTRA)
				{
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
			}
			displaySomeLines (showFound);
			displayTidy ();
		}
		else
		{
			version (1);
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
	long fileAge = 0, fileAgeMS = 0;
#ifdef USE_STATX
	mode_t stMode = file -> fileStat.stx_mode;
	uid_t stUId = file -> fileStat.stx_uid;
	gid_t stGId = file -> fileStat.stx_gid;
	ino_t stINo = file -> fileStat.stx_ino;
	nlink_t stNLink = file -> fileStat.stx_nlink;
	off_t stSize = file -> fileStat.stx_size;
#else
	mode_t stMode = file -> fileStat.st_mode;
	uid_t stUId = file -> fileStat.st_uid;
	gid_t stGId = file -> fileStat.st_gid;
	ino_t stINo = file -> fileStat.st_ino;
	nlink_t stNLink = file -> fileStat.st_nlink;
	off_t stSize = file -> fileStat.st_size;
#endif
	if (!dot[0])
	{
#ifdef RADIXCHAR
		strncpy (dot, nl_langinfo(RADIXCHAR), 2);
#else
		strcpy (dot, ".");
#endif
	}

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
     * Don't show files or directories that are used for version control.     *
	 *------------------------------------------------------------------------*/
	if (dirType & HIDEVERCTL)
	{
		if (strcmp (file -> fileName, "CVS") == 0)
			return 0;
		if (strcmp (file -> fileName, ".git") == 0)
			return 0;
		if (strcmp (file -> fileName, ".svn") == 0)
			return 0;
	}

	switch (showDate)
	{
#ifdef USE_STATX
		case DATE_MOD:
			fileAge = file -> fileStat.stx_mtime.tv_sec;
			fileAgeMS = file -> fileStat.stx_mtime.tv_nsec / 1000000;
			break;
		case DATE_ACC:
			fileAge = file -> fileStat.stx_atime.tv_sec;
			fileAgeMS = file -> fileStat.stx_atime.tv_nsec / 1000000;
			break;
		case DATE_CHG:
			fileAge = file -> fileStat.stx_ctime.tv_sec;
			fileAgeMS = file -> fileStat.stx_ctime.tv_nsec / 1000000;
			break;
		case DATE_BTH:
			fileAge = file -> fileStat.stx_btime.tv_sec;
			fileAgeMS = file -> fileStat.stx_btime.tv_nsec / 1000000;
			break;
#else
		case DATE_MOD:
			fileAge = file -> fileStat.st_mtim.tv_sec;
			fileAgeMS = file -> fileStat.st_mtim.tv_nsec / 1000000;
			break;
		case DATE_ACC:
			fileAge = file -> fileStat.st_atim.tv_sec;
			fileAgeMS = file -> fileStat.st_atim.tv_nsec / 1000000;
			break;
		case DATE_CHG:
			fileAge = file -> fileStat.st_ctim.tv_sec;
			fileAgeMS = file -> fileStat.st_ctim.tv_nsec / 1000000;
			break;
#endif
	}

	/*------------------------------------------------------------------------*
	 * Don't show files or directories that are outside time values.          *
     *------------------------------------------------------------------------*/
	if (showType & SHOW_IN_AGE)
	{
		if (maxFileAge != -1 && fileAge < maxFileAge)		/* Too old */
			return 0;
		if (minFileAge != -1 && fileAge > minFileAge)		/* Too young */
			return 0;
	}

	/*------------------------------------------------------------------------*
     * Show the directory in wide format.                                     *
     *------------------------------------------------------------------------*/
	if (showType & SHOW_WIDE)
	{
		char marker1[2], marker2[2], displayName[PATH_SIZE];
		int colour = -1;

		if (S_ISLNK (stMode))
		{
			marker1[0] = '<';
			marker2[0] = '>';
			marker1[1] = marker2[1] = 0;
			colour = colourType[1];
			linksFound ++;
		}
		else if (S_ISDIR (stMode))
		{
			marker1[0] = '[';
			marker2[0] = ']';
			marker1[1] = marker2[1] = 0;
			colour = colourType[0];
			dirsFound ++;
		}
		else if (!stMode || stMode)
		{
			marker1[0] = ' ';
			marker2[0] = ' ';
			marker1[1] = marker2[1] = 0;
#ifdef USE_STATX
			totalSize += file -> fileStat.stx_size;
#else
			totalSize += file -> fileStat.st_size;
#endif
			colour = colourType[2];
			if (stMode & 0100)
				colour |= colourType[3];
			if (stMode & 0200)
				colour |= colourType[4];
			if (stMode & 0400)
				colour |= colourType[5];
			filesFound ++;
		}
		else if (S_ISBLK(stMode) || S_ISCHR(stMode))
		{
			marker1[0] = '{';
			marker2[0] = '}';
			marker1[1] = marker2[1] = 0;
			colour = colourType[6];
			devsFound ++;
		}
		else if (S_ISSOCK(stMode))
		{
			marker1[0] = '{';
			marker2[0] = '}';
			marker1[1] = marker2[1] = 0;
			colour = colourType[6];
			socksFound ++;
		}
		else if (S_ISFIFO(stMode))
		{
			marker1[0] = '{';
			marker2[0] = '}';
			marker1[1] = marker2[1] = 0;
			colour = colourType[6];
			pipesFound ++;
		}
		else
			return 0;

		if (showType & SHOW_QUOTE)
			quoteCopy (displayName, file -> fileName);
		else
			strcpy (displayName, file -> fileName);

		if (S_ISDIR (stMode))
		{
			strcat (displayName, "/");
		}
		displayInColumn (currentCol++, marker1);
		displayInColour (currentCol++, colour, "%s", displayName);
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
		if (S_ISDIR (stMode))
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
		char md5String[65];
		char shaString[65];
		char verString[201];
		char rightsBuff[14];
		char numBuff[41];

		if (showType & SHOW_QUOTE)
		{
			quoteCopy (fullName, file -> fullPath);
			quoteCopy (&fullName[strlen (fullName)], file -> fileName);
			quoteCopy (displayName, file -> partPath);
			quoteCopy (&displayName[strlen (displayName)], file -> fileName);
		}
		else
		{
			strcpy (fullName, file -> fullPath);
			strcat (fullName, file -> fileName);
			strcpy (displayName, file -> partPath);
			strcat (displayName, file -> fileName);
		}

		switch (showDate)
		{
#ifdef USE_STATX
		case DATE_MOD:
			fileAge = file -> fileStat.stx_mtime.tv_sec;
			fileAgeMS = file -> fileStat.stx_mtime.tv_nsec / 1000000;
			break;
		case DATE_ACC:
			fileAge = file -> fileStat.stx_atime.tv_sec;
			fileAgeMS = file -> fileStat.stx_atime.tv_nsec / 1000000;
			break;
		case DATE_CHG:
			fileAge = file -> fileStat.stx_ctime.tv_sec;
			fileAgeMS = file -> fileStat.stx_ctime.tv_nsec / 1000000;
			break;
		case DATE_BTH:
			fileAge = file -> fileStat.stx_btime.tv_sec;
			fileAgeMS = file -> fileStat.stx_btime.tv_nsec / 1000000;
			break;
#else
		case DATE_MOD:
			fileAge = file -> fileStat.st_mtim.tv_sec;
			fileAgeMS = file -> fileStat.st_mtim.tv_nsec / 1000000;
			break;
		case DATE_ACC:
			fileAge = file -> fileStat.st_atim.tv_sec;
			fileAgeMS = file -> fileStat.st_atim.tv_nsec / 1000000;
			break;
		case DATE_CHG:
			fileAge = file -> fileStat.st_ctim.tv_sec;
			fileAgeMS = file -> fileStat.st_ctim.tv_nsec / 1000000;
			break;
#endif
		}

		/*--------------------------------------------------------------------*
         * If the file is a LINK then just them it is a link.                 *
         *--------------------------------------------------------------------*/
		if (S_ISLNK (stMode))
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
					displayInColumn (columnTranslate[COL_N_LINKS], "%s", displayCommaNumber (stNLink, numBuff));
				}
				if (showType & SHOW_OWNER)
				{
					displayInColumn (columnTranslate[COL_OWNER], "%s", displayOwnerString (stUId, ownerString));
				}
				if (showType & SHOW_GROUP)
				{
					displayInColumn (columnTranslate[COL_GROUP], "%s", displayGroupString (stUId, groupString));
				}
				if (showType & SHOW_SELINUX)
				{
					displayInColumn (columnTranslate[COL_CONTEXT], "%s", displayContextString (fullName, contextString));
				}
				if (showType & SHOW_INODE)
				{
					displayInColumn (columnTranslate[COL_INODE], "%s", displayCommaNumber (stINo, numBuff));
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
					if (dateFormat)
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
						{
							displayInColumn (columnTranslate[COL_DATE], displayDateString (fileAge, dateString));
						}
					}
					else
					{
						displayInColumn (columnTranslate[COL_DATE], displayCommaNumber (fileAge, numBuff));
						displayInColumn (columnTranslate[COL_DATE], "%s%03d", dot, fileAgeMS);
					}
				}
				if (showType & SHOW_VERSION)
				{
					if (file -> fileVer == NULL)
					{
						getFileVersion (file);
					}
					displayInColumn (columnTranslate[COL_VERSION], "%s", displayVerString (file, verString));
				}
			}
			displayNewLine (0);
			linksFound ++;
		}

		/*--------------------------------------------------------------------*
         * If the file is a directory then just show them the name.           *
         *--------------------------------------------------------------------*/
		else if (S_ISDIR (stMode))
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
					displayInColumn (columnTranslate[COL_N_LINKS], "%s", displayCommaNumber (stNLink, numBuff));
				}
				if (showType & SHOW_OWNER)
				{
					displayInColumn (columnTranslate[COL_OWNER], "%s", displayOwnerString (stUId, ownerString));
				}
				if (showType & SHOW_GROUP)
				{
					displayInColumn (columnTranslate[COL_GROUP], "%s", displayGroupString (stGId, groupString));
				}
				if (showType & SHOW_SELINUX)
				{
					displayInColumn (columnTranslate[COL_CONTEXT], "%s", displayContextString (fullName, contextString));
				}
				if (showType & SHOW_INODE)
				{
					displayInColumn (columnTranslate[COL_INODE], "%s", displayCommaNumber (stINo, numBuff));
				}
				displayInColour (columnTranslate[COL_FILENAME], colourType[0], "%s/", displayName);

				if (showType & SHOW_DATE)
				{
					if (dateFormat)
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
						{
							displayInColumn (columnTranslate[COL_DATE], displayDateString (fileAge, dateString));
						}
					}
					else
					{
						displayInColumn (columnTranslate[COL_DATE], displayCommaNumber (fileAge, numBuff));
						displayInColumn (columnTranslate[COL_DATE], "%s%03d", dot, fileAgeMS);
					}
				}
				if (showType & SHOW_VERSION)
				{
					if (file -> fileVer == NULL)
					{
						getFileVersion (file);
					}
					displayInColumn (columnTranslate[COL_VERSION], "%s", displayVerString (file, verString));
				}
			}
			displayNewLine (0);
			dirsFound ++;
		}

		/*--------------------------------------------------------------------*
         * If the file is a device then just show them the name.              *
         *--------------------------------------------------------------------*/
		else if (S_ISBLK(stMode) || S_ISCHR(stMode) || S_ISSOCK(stMode) || S_ISFIFO(stMode))
		{
			if (showType & SHOW_TYPE)
			{
				if (S_ISBLK(stMode))
				{
					displayInColumn (columnTranslate[COL_TYPE], "<BlkD>");
					devsFound ++;
				}
				else if (S_ISCHR(stMode))
				{
					displayInColumn (columnTranslate[COL_TYPE], "<ChrD>");
					devsFound ++;
				}
				else if (S_ISSOCK(stMode))
				{
					displayInColumn (columnTranslate[COL_TYPE], "<Sckt>");
					socksFound ++;
				}
				else if (S_ISFIFO(stMode))
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
					displayInColumn (columnTranslate[COL_N_LINKS], "%s", displayCommaNumber (stNLink, numBuff));
				}
				if (showType & SHOW_OWNER)
				{
					displayInColumn (columnTranslate[COL_OWNER], "%s", displayOwnerString (stUId, ownerString));
				}
				if (showType & SHOW_GROUP)
				{
					displayInColumn (columnTranslate[COL_GROUP], "%s", displayGroupString (stGId, groupString));
				}
				if (showType & SHOW_SELINUX)
				{
					displayInColumn (columnTranslate[COL_CONTEXT], "%s", displayContextString (fullName, contextString));
				}
				if (showType & SHOW_INODE)
				{
					displayInColumn (columnTranslate[COL_INODE], "%s", displayCommaNumber (stINo, numBuff));
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
					if (dateFormat)
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
						{
							displayInColumn (columnTranslate[COL_DATE], displayDateString (fileAge, dateString));
						}
					}
					else
					{
						displayInColumn (columnTranslate[COL_DATE], displayCommaNumber (fileAge, numBuff));
						displayInColumn (columnTranslate[COL_DATE], "%s%03d", dot, fileAgeMS);
					}
				}
				if (showType & SHOW_VERSION)
				{
					if (file -> fileVer == NULL)
					{
						getFileVersion (file);
					}
					displayInColumn (columnTranslate[COL_VERSION], "%s", displayVerString (file, verString));
				}
			}
			displayNewLine (0);
		}

		/*--------------------------------------------------------------------*
         * If the file is a normal file then show the size/date/time.         *
         *--------------------------------------------------------------------*/
		else if (!stMode || stMode & S_IFREG)
		{
			int colour = colourType[2];

			if (stMode & 0100)
				colour |= colourType[3];
			if (stMode & 0200)
				colour |= colourType[4];
			if (stMode & 0400)
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
					displayInColumn (columnTranslate[COL_N_LINKS], "%s", displayCommaNumber (stNLink, numBuff));
				}
				if (showType & SHOW_OWNER)
				{
					displayInColumn (columnTranslate[COL_OWNER], displayOwnerString (stUId, ownerString));
				}
				if (showType & SHOW_GROUP)
				{
					displayInColumn (columnTranslate[COL_GROUP], displayGroupString (stGId, groupString));
				}
				if (showType & SHOW_SELINUX)
				{
					displayInColumn (columnTranslate[COL_CONTEXT], "%s", displayContextString (fullName, contextString));
				}
				if (showType & SHOW_MD5)
				{
					displayInColumn (columnTranslate[COL_MD5], "%s", displayMD5String (file, md5String, encode));
				}
				if (showType & SHOW_SHA256)
				{
					displayInColumn (columnTranslate[COL_SHA256], "%s", displaySHA256String (file, shaString, encode));
				}
				if (showType & SHOW_VERSION)
				{
					if (file -> fileVer == NULL)
					{
						getFileVersion (file);
					}
					displayInColumn (columnTranslate[COL_VERSION], "%s", displayVerString (file, verString));
				}
				if (showType & SHOW_INODE)
				{
					displayInColumn (columnTranslate[COL_INODE], "%s", displayCommaNumber (stINo, numBuff));
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
					long long fileSize = stSize;
					displayInColumn (columnTranslate[COL_SIZE], sizeFormat ? displayFileSize (fileSize, numBuff) :
							displayCommaNumber (fileSize, numBuff));
				}
				if (showType & SHOW_DATE)
				{
					if (dateFormat)
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
						{
							displayInColumn (columnTranslate[COL_DATE], displayDateString (fileAge, dateString));
						}
					}
					else
					{
						displayInColumn (columnTranslate[COL_DATE], displayCommaNumber (fileAge, numBuff));
						displayInColumn (columnTranslate[COL_DATE], "%s%03d", dot, fileAgeMS);
					}
				}
			}
			displayNewLine (0);

			totalSize += stSize;
			filesFound ++;
		}
		else
			return 0;
	}
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  F I L E  V E R S I O N                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the file version from the file name.
 *  \param fileOne File to get the version for.
 *  \result None.
 */
void getFileVersion (DIR_ENTRY *fileOne)
{
	if (fileOne -> fileVer == NULL)
	{
		fileOne -> fileVer = (struct dirFileVerInfo *)malloc (sizeof (struct dirFileVerInfo));

		if (fileOne -> fileVer != NULL)
		{
			int j = 0, l = 0;
			char fileStart[PATH_SIZE], lastChar = 0, *filePtr = fileOne -> fileName;

			memset (fileOne -> fileVer, 0, sizeof (struct dirFileVerInfo));
			fileStart[0] = 0;

			while (filePtr[j])
			{
				if (l == 0)
				{
					if (filePtr[j] < '0' || filePtr[j] > '9')
					{
						fileStart[j] = filePtr[j];
						fileStart[j + 1] = 0;
					}
					else
					{
						fileOne -> fileVer -> verVals[0] = 0;
						++l;
					}
				}
				if (l > 0 && l < 20)
				{
					if (filePtr[j] < '0' || filePtr[j] > '9')
					{
						if (lastChar >= '0' && lastChar <= '9')
						{
							fileOne -> fileVer -> verVals[++l] = 0;
						}
					}
					else
					{
						fileOne -> fileVer -> verVals[0] = l + 1;
						fileOne -> fileVer -> verVals[l] =
								(fileOne -> fileVer -> verVals[l] * 10) + (filePtr[j] - '0');
					}
				}
				lastChar = filePtr[j++];
			}
			fileOne -> fileVer -> fileStart = (char *)malloc (strlen (fileStart) + 1);
			if (fileOne -> fileVer -> fileStart != NULL)
			{
				strcpy (fileOne -> fileVer -> fileStart, fileStart);
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O M P A R E  N A M E S                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two file names.
 *  \param nameOne First name.
 *  \param nameTwo Second name.
 *  \param useCase Should the compare be case sensitive.
 *  \result -1, 0 or 1 depending on compare.
 */
int compareNames (char *nameOne, char *nameTwo, int useCase)
{
	return (useCase ?
			strcmp (nameOne, nameTwo) :
			strcasecmp (nameOne, nameTwo));
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I N D  F I L E  W O R D  R E V                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Search for a word backwards.
 *  \param name Full file name.
 *  \param len Length of the filename.
 *  \param wordNum Word to find.
 *  \result Pointer if found, NULL if not.
 */
char *findFileWordRev (char *name, int len, int wordNum)
{
	int i, lastType = 0, word = 0;

	for (i = len - 1; i >= 0; --i)
	{
		if (name[i] >= 'A' && name[i] <= 'Z')
		{
			if (lastType == 3)
			{
				++word;
			}
			lastType = 1;
		}
		else if (name[i] >= 'a' && name[i] <= 'z')
		{
			if (lastType == 1 || lastType == 3)
			{
				++word;
			}
			lastType = 2;
		}
		else if (name[i] >= '0' && name[i] <= '9')
		{
			if (lastType == 1 || lastType == 2)
			{
				++word;
			}
			lastType = 3;
		}
		else
		{
			if (lastType == 1 || lastType == 2 || lastType == 3)
			{
				++word;
			}
			lastType = 4;
		}

		if (word >= wordNum)
		{
			return &name[i + 1];
		}
	}
	return NULL;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I N D  F I L E  W O R D                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Find the start of the nth word in a filename.
 *  \param name Filename to search.
 *  \result None if word not found, pointer to word if found.
 */
char *findFileWord (char *name)
{
	int i, len = strlen (name), lastType = 0, word = 0;

	if (wordNumber < 0)
	{
		return findFileWordRev (name, len, wordNumber * -1);
	}

	for (i = 0; i < len; ++i)
	{
		if (name[i] >= 'A' && name[i] <= 'Z')
		{
			if (lastType == 2 || lastType == 3 || lastType == 4)
			{
				++word;
			}
			lastType = 1;
		}
		else if (name[i] >= 'a' && name[i] <= 'z')
		{
			if (lastType == 3 || lastType == 4)
			{
				++word;
			}
			lastType = 2;
		}
		else if (name[i] >= '0' && name[i] <= '9')
		{
			if (lastType == 1 || lastType == 2 || lastType == 4)
			{
				++word;
			}
			lastType = 3;
		}
		else
		{
			lastType = 4;
		}

		if (word >= wordNumber)
		{
			return &name[i];
		}
	}
	return NULL;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O M P A R E  V E R S I O N S                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two file versions.
 *  \param fileVerOne First file version.
 *  \param fileVerTwo Second file version.
 *  \result -1, 0 or 1 depending on compare.
 */
int compareVersions (struct dirFileVerInfo *fileVerOne, struct dirFileVerInfo *fileVerTwo)
{
	int retn = 0, i;

	for (i = 1; i < 21 && retn == 0; ++i)
	{
		retn = (fileVerOne -> verVals[i] == fileVerTwo -> verVals[i] ? 0 :
				fileVerOne -> verVals[i] < fileVerTwo -> verVals[i] ? -1 : 1);
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O M P A R E  F I L E  V E R S I O N                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two file versions.
 *  \param fileOne First file to compare.
 *  \param fileTwo Second file to compare.
 *  \param useCase Use case when comparing names.
 *  \result 0 match, -1 less, 1 greater.
 */
int compareFileVersion (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo, int useCase)
{
	int retn = 0;

	if (fileOne -> fileVer == NULL)
	{
		getFileVersion (fileOne);
	}
	if (fileTwo -> fileVer == NULL)
	{
		getFileVersion (fileTwo);
	}
	if (fileOne -> fileVer == NULL || fileTwo -> fileVer == NULL)
	{
		return retn;
	}

	if (orderType == ORDER_NAVE)
	{
		retn = compareNames (fileOne -> fileVer -> fileStart, fileTwo -> fileVer -> fileStart, useCase);
	}
	if (retn == 0)
	{
		retn = compareVersions (fileOne -> fileVer, fileTwo -> fileVer);
	}
	if (orderType == ORDER_VERS && retn == 0)
	{
		retn = compareNames (fileOne -> fileVer -> fileStart, fileTwo -> fileVer -> fileStart, useCase);
	}
	if (retn == 0)
	{
		retn = compareNames (fileOne -> fileName, fileTwo -> fileName, useCase);
	}
	return retn;
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
#ifdef USE_STATX
	mode_t stModeOne = fileOne -> fileStat.stx_mode;
	mode_t stModeTwo = fileTwo -> fileStat.stx_mode;
	uid_t stUIdOne = fileOne -> fileStat.stx_uid;
	uid_t stUIdTwo = fileTwo -> fileStat.stx_uid;
	gid_t stGIdOne = fileOne -> fileStat.stx_gid;
	gid_t stGIdTwo = fileTwo -> fileStat.stx_gid;
	ino_t stINoOne = fileOne -> fileStat.stx_ino;
	ino_t stINoTwo = fileTwo -> fileStat.stx_ino;
	nlink_t stNLinkOne = fileOne -> fileStat.stx_nlink;
	nlink_t stNLinkTwo = fileTwo -> fileStat.stx_nlink;
	off_t stSizeOne = fileOne -> fileStat.stx_size;
	off_t stSizeTwo = fileTwo -> fileStat.stx_size;
#else
	mode_t stModeOne = fileOne -> fileStat.st_mode;
	mode_t stModeTwo = fileTwo -> fileStat.st_mode;
	uid_t stUIdOne = fileOne -> fileStat.st_uid;
	uid_t stUIdTwo = fileTwo -> fileStat.st_uid;
	gid_t stGIdOne = fileOne -> fileStat.st_gid;
	gid_t stGIdTwo = fileTwo -> fileStat.st_gid;
	ino_t stINoOne = fileOne -> fileStat.st_ino;
	ino_t stINoTwo = fileTwo -> fileStat.st_ino;
	nlink_t stNLinkOne = fileOne -> fileStat.st_nlink;
	nlink_t stNLinkTwo = fileTwo -> fileStat.st_nlink;
	off_t stSizeOne = fileOne -> fileStat.st_size;
	off_t stSizeTwo = fileTwo -> fileStat.st_size;
#endif

	if (orderType == ORDER_NONE)
		return showType & SHOW_RORDER ? -1 : 1;

	/*------------------------------------------------------------------------*
     * Force the directories to the begining of the list                      *
     *------------------------------------------------------------------------*/
	if (S_ISDIR (stModeOne))
	{
		if (!(S_ISDIR (stModeTwo)))
			return -1;
	}
	if (S_ISDIR (stModeTwo))
	{
		if (!(S_ISDIR (stModeOne)))
			return 1;
	}

	/*------------------------------------------------------------------------*
     * Force the links to follow the directories                              *
     *------------------------------------------------------------------------*/
	if (S_ISLNK (stModeOne))
	{
		if (!(S_ISLNK (stModeTwo)))
			return -1;
	}
	if (S_ISLNK (stModeTwo))
	{
		if (!(S_ISLNK (stModeOne)))
			return 1;
	}

	/*------------------------------------------------------------------------*
     * Force the block devices to follow the links                            *
     *------------------------------------------------------------------------*/
	if (S_ISBLK(stModeOne))
	{
		if (!(S_ISBLK(stModeTwo)))
			return -1;
	}
	if (S_ISBLK(stModeTwo))
	{
		if (!(S_ISBLK(stModeOne)))
			return 1;
	}

	/*------------------------------------------------------------------------*
     * Force the char devices to follow the block devices                     *
     *------------------------------------------------------------------------*/
	if (S_ISCHR(stModeOne))
	{
		if (!(S_ISCHR(stModeTwo)))
			return -1;
	}
	if (S_ISCHR(stModeTwo))
	{
		if (!(S_ISCHR(stModeOne)))
			return 1;
	}

	/*------------------------------------------------------------------------*
     * Force the sockets to follow the char devices                           *
     *------------------------------------------------------------------------*/
	if (S_ISSOCK (stModeOne))
	{
		if (!(S_ISSOCK (stModeTwo)))
			return -1;
	}
	if (S_ISSOCK (stModeTwo))
	{
		if (!(S_ISSOCK (stModeOne)))
			return 1;
	}

	/*------------------------------------------------------------------------*
     * Force the pipes to follow the sockets                                  *
     *------------------------------------------------------------------------*/
	if (S_ISFIFO (stModeOne))
	{
		if (!(S_ISFIFO (stModeTwo)))
			return -1;
	}
	if (S_ISFIFO (stModeTwo))
	{
		if (!(S_ISFIFO (stModeOne)))
			return 1;
	}

	switch (orderType)
	{
	case ORDER_SIZE:
		if (showType & SHOW_MATCH)
		{
			if (stSizeOne == stSizeTwo)
			{
				int i;

				if (fileOne -> sha256Sum == NULL)
				{
					if ((fileOne -> sha256Sum = malloc (CRC_BUFF_SIZE)) != NULL)
					{
						strcpy (fullName, fileOne -> fullPath);
						strcat (fullName, fileOne -> fileName);
						SHA256File (fullName, fileOne -> sha256Sum);
					}
				}
				if (fileTwo -> sha256Sum == NULL)
				{
					if ((fileTwo -> sha256Sum = malloc (CRC_BUFF_SIZE)) != NULL)
					{
						strcpy (fullName, fileTwo -> fullPath);
						strcat (fullName, fileTwo -> fileName);
						SHA256File (fullName, fileTwo -> sha256Sum);
					}
				}
				for (i = 0; i < 32; ++i)
				{
					if (fileOne -> sha256Sum != NULL && fileTwo -> sha256Sum != NULL)
					{
						if (fileOne -> sha256Sum[i] != fileTwo -> sha256Sum[i])
							break;
					}
				}
				if (i == 32)
				{
					fileOne -> match = 1;
					fileTwo -> match = 1;
				}
			}
		}
		retn = (stSizeOne > stSizeTwo ? 1 : stSizeOne < stSizeTwo ? -1 : 0);
		break;

	case ORDER_DATE:
		switch (showDate)
		{
#ifdef USE_STATX
		case DATE_MOD:
			retn = (fileOne -> fileStat.stx_mtime.tv_sec > fileTwo -> fileStat.stx_mtime.tv_sec ? -1 :
					fileOne -> fileStat.stx_mtime.tv_sec < fileTwo -> fileStat.stx_mtime.tv_sec ? 1 :
					fileOne -> fileStat.stx_mtime.tv_nsec > fileTwo -> fileStat.stx_mtime.tv_nsec ? -1 :
					fileOne -> fileStat.stx_mtime.tv_nsec < fileTwo -> fileStat.stx_mtime.tv_nsec ? 1 : 0);
			break;
		case DATE_ACC:
			retn = (fileOne -> fileStat.stx_atime.tv_sec > fileTwo -> fileStat.stx_atime.tv_sec ? -1 :
					fileOne -> fileStat.stx_atime.tv_sec < fileTwo -> fileStat.stx_atime.tv_sec ? 1 :
					fileOne -> fileStat.stx_atime.tv_nsec > fileTwo -> fileStat.stx_atime.tv_nsec ? -1 :
					fileOne -> fileStat.stx_atime.tv_nsec < fileTwo -> fileStat.stx_atime.tv_nsec ? 1 : 0);
			break;
		case DATE_CHG:
			retn = (fileOne -> fileStat.stx_ctime.tv_sec > fileTwo -> fileStat.stx_ctime.tv_sec ? -1 :
					fileOne -> fileStat.stx_ctime.tv_sec < fileTwo -> fileStat.stx_ctime.tv_sec ? 1 :
					fileOne -> fileStat.stx_ctime.tv_nsec > fileTwo -> fileStat.stx_ctime.tv_nsec ? -1 :
					fileOne -> fileStat.stx_ctime.tv_nsec < fileTwo -> fileStat.stx_ctime.tv_nsec ? 1 : 0);
			break;
		case DATE_BTH:
			retn = (fileOne -> fileStat.stx_btime.tv_sec > fileTwo -> fileStat.stx_btime.tv_sec ? -1 :
					fileOne -> fileStat.stx_btime.tv_sec < fileTwo -> fileStat.stx_btime.tv_sec ? 1 :
					fileOne -> fileStat.stx_btime.tv_nsec > fileTwo -> fileStat.stx_btime.tv_nsec ? -1 :
					fileOne -> fileStat.stx_btime.tv_nsec < fileTwo -> fileStat.stx_btime.tv_nsec ? 1 : 0);
			break;
#else
		case DATE_MOD:
			retn = (fileOne -> fileStat.st_mtim.tv_sec > fileTwo -> fileStat.st_mtim.tv_sec ? -1 :
					fileOne -> fileStat.st_mtim.tv_sec < fileTwo -> fileStat.st_mtim.tv_sec ? 1 :
					fileOne -> fileStat.st_mtim.tv_nsec > fileTwo -> fileStat.st_mtim.tv_nsec ? -1 :
					fileOne -> fileStat.st_mtim.tv_nsec < fileTwo -> fileStat.st_mtim.tv_nsec ? 1 : 0);
			break;
		case DATE_ACC:
			retn = (fileOne -> fileStat.st_atim.tv_sec > fileTwo -> fileStat.st_atim.tv_sec ? -1 :
					fileOne -> fileStat.st_atim.tv_sec < fileTwo -> fileStat.st_atim.tv_sec ? 1 :
					fileOne -> fileStat.st_atim.tv_nsec > fileTwo -> fileStat.st_atim.tv_nsec ? -1 :
					fileOne -> fileStat.st_atim.tv_nsec < fileTwo -> fileStat.st_atim.tv_nsec ? 1 : 0);
			break;
		case DATE_CHG:
			retn = (fileOne -> fileStat.st_ctim.tv_sec > fileTwo -> fileStat.st_ctim.tv_sec ? -1 :
					fileOne -> fileStat.st_ctim.tv_sec < fileTwo -> fileStat.st_ctim.tv_sec ? 1 :
					fileOne -> fileStat.st_ctim.tv_nsec > fileTwo -> fileStat.st_ctim.tv_nsec ? -1 :
					fileOne -> fileStat.st_ctim.tv_nsec < fileTwo -> fileStat.st_ctim.tv_nsec ? 1 : 0);
			break;
#endif
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
			displayOwnerString (stUIdOne, ownerOne);
			displayOwnerString (stUIdTwo, ownerTwo);
			retn = strcasecmp (ownerOne, ownerTwo);
		}
		break;

	case ORDER_GRUP:
		{
			char groupOne[80], groupTwo[80];
			displayGroupString (stGIdOne, groupOne);
			displayGroupString (stGIdTwo, groupTwo);
			retn = strcasecmp (groupOne, groupTwo);
		}
		break;

	case ORDER_LINK:
		retn = (stNLinkOne > stNLinkTwo ? -1 : stNLinkOne < stNLinkTwo ? 1 : 0);
		break;

	case ORDER_INOD:
		retn = (stINoOne > stINoTwo ? -1 : stINoOne < stINoTwo ? 1 : 0);
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
			if ((fileOne -> md5Sum = malloc (CRC_BUFF_SIZE)) != NULL)
			{
				strcpy (fullName, fileOne -> fullPath);
				strcat (fullName, fileOne -> fileName);
				MD5File (fullName, fileOne -> md5Sum);
			}
		}
		if (fileTwo -> md5Sum == NULL)
		{
			if ((fileTwo -> md5Sum = malloc (CRC_BUFF_SIZE)) != NULL)
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

	case ORDER_SHAS:
		if (fileOne -> sha256Sum == NULL)
		{
			if ((fileOne -> sha256Sum = malloc (CRC_BUFF_SIZE)) != NULL)
			{
				strcpy (fullName, fileOne -> fullPath);
				strcat (fullName, fileOne -> fileName);
				SHA256File (fullName, fileOne -> sha256Sum);
			}
		}
		if (fileTwo -> sha256Sum == NULL)
		{
			if ((fileTwo -> sha256Sum = malloc (CRC_BUFF_SIZE)) != NULL)
			{
				strcpy (fullName, fileTwo -> fullPath);
				strcat (fullName, fileTwo -> fileName);
				SHA256File (fullName, fileTwo -> sha256Sum);
			}
		}
		if (fileOne -> sha256Sum != NULL && fileTwo -> sha256Sum != NULL)
		{
			int i;
			for (i = 0; i < 32 && retn == 0; ++i)
			{
				retn = (fileOne -> sha256Sum[i] < fileTwo -> sha256Sum[i] ? -1 :
						fileOne -> sha256Sum[i] > fileTwo -> sha256Sum[i] ? 1 : 0);
			}
		}
		break;

	case ORDER_NAVE:
	case ORDER_VERS:
		retn = compareFileVersion (fileOne, fileTwo, dirType & USECASE);
		break;

	case ORDER_WORD:
		{
			char *nameOne = findFileWord (fileOne -> fileName);
			char *nameTwo = findFileWord (fileTwo -> fileName);
			if (nameOne != NULL && nameTwo != NULL)
			{
				retn = compareNames (nameOne, nameTwo, dirType & USECASE);
			}
			else
			{
				retn = nameOne != NULL ? 1 : nameTwo != NULL ? -1 : retn;
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
		retn = compareNames (fileNameOne, fileNameTwo, dirType & USECASE);
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

	dst[i = j = 0] = 0;
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

