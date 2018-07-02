/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y . C                                                                                                 *
 *  =================                                                                                                 *
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
 *  \brief Functions to display information on a console.
 */
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#ifdef HAVE_SELINUX_SELINUX_H
#include <selinux/selinux.h>
#endif
#include <pwd.h>
#include <grp.h>
#include <locale.h>
#include <langinfo.h>
#include <errno.h>
#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#endif

#include "dircmd.h"

#define ONE_DAY (24 * 60 * 60)

#define ROW_NORMAL_LINE			0
#define ROW_DISPLAY_LINE		1
#define ROW_DISPLAY_BLANK		2
#define ROW_DISPLAY_HEADING		3
#define ROW_DISPLAY_INFO		4
#define TEMP_BUFF_SIZE			1024

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
static int ncols = -1, nrows = -1;
static int displayStartLine = 0;
static int displayEndLine = MAXINT;
static int displayLines = 0;
static time_t timeNow;
static time_t timeDay;
static time_t timeWeek;
static time_t timeOld;

char dateFormats[4][41] =
{
	"Today %k:%M:%S",			/*  Today hh:mm:ss */
	"%e %a %k:%M:%S",			/* dd Day hh:mm:ss */
	"%e/%b %k:%M:%S",			/* dd/Mon hh:mm:ss */
	"%e/%b/%Y(%P)"				/* dd/Mon/year(pp) */
};

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
typedef struct _fullColDesc
{
	int maxWidth;
	int minWidth;
	int attrib;
	int priority;
	int colour;
	int gap;

	int maxSize;
	int displaySize;
	char *heading;
}
FULL_COLUMN_DESC;

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
typedef struct _rowDesc
{
	int rowType;
	char *colString[MAX_COLUMNS];
	int colColour[MAX_COLUMNS];
}
ROW_DESC;

static FULL_COLUMN_DESC *fullColDesc[MAX_COLUMNS];
static int columnCount;

static ROW_DESC *currentRow;
static void *rowQueue;

static int displayOptions;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  I N I T                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Initialise the display, replaces window size, fixes locale.
 *  \result None.
 */
void displayInit()
{
	setlocale(LC_CTYPE, "");
	setlocale(LC_TIME, "");
	setlocale(LC_NUMERIC, "");

	displayGetWindowSize();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  L I N E  C H A R                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw a line of the specified character across the screen.
 *  \param lineChar Character to display.
 *  \result None.
 */
void displayLineChar (char lineChar)
{
	int i = 0, cols = displayGetWidth();

	for (; i < cols; i ++)
	{
		putchar (lineChar);
	}
	putchar ('\n');
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  L I N E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw a line of dashes across the screen.
 *  \result None.
 */
void displayLine ()
{
	displayLineChar ('-');
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  T H E  T I M E S                                                                                           *
 *  =======================                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the current time, used when making the date.
 *  \result None.
 */
static void getTheTimes ()
{
	struct tm *tempTm;

	timeNow = time(NULL);
	tempTm = localtime (&timeNow);

	if (tempTm != NULL)
	{
		tempTm -> tm_hour = 0;
		tempTm -> tm_min  = 0;
		tempTm -> tm_sec  = 0;

		timeDay = mktime (tempTm);

		tempTm -> tm_mday -= 7;
		timeWeek = mktime (tempTm);

		tempTm -> tm_mday = 1;
		tempTm -> tm_mon  -= 6;

		timeOld = mktime (tempTm);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  C O M M A  N U M B E R                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a number into a string with thousand seperators.
 *  \param number Number to convert into the string.
 *  \param outString Output the number to this string.
 *  \result Pointer to the outString.
 */
char *displayCommaNumber (long long number, char *outString)
{
	int i, j = 20, x = 0;
	char inBuff[21], outBuff[21], sep[3];

#ifdef THOUSEP
	strncpy (sep, nl_langinfo(THOUSEP), 2);
#else
	strcpy (sep, ",");
#endif

	sprintf (inBuff, "%lld", number);
	i = strlen (inBuff);
	outBuff[j--] = 0;

	while (i--)
	{
		if (x && !(x % 3) && sep[0])
			outBuff[j--] = sep[0];

		outBuff[j--] = inBuff[i];
		x ++;
	}
	strcpy (outString, &outBuff[j + 1]);
	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  D A T E  S T R I N G                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Make a date string out of a time_t.
 *  \param showDate Date in time_t format to convert to a string.
 *  \param outString Output the date to this string.
 *  \result Pointer to the outString.
 */
char *displayDateString (time_t showDate, char *outString)
{
	struct tm *tmShow;

	if (timeDay == 0)
		getTheTimes ();

	tmShow = localtime(&showDate);

	if (showDate >= timeDay)
	{
		strftime(outString, 80, dateFormats[0], tmShow);
	}
	else if (showDate >= timeWeek)
	{
		strftime(outString, 80, dateFormats[1], tmShow);
	}
	else if (showDate >= timeOld)
	{
		strftime(outString, 80, dateFormats[2], tmShow);
	}
	else
	{
		strftime(outString, 80, dateFormats[3], tmShow);
	}
	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  S E T  D A T E  F O R M A T                                                                        *
 *  ==========================================                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Save the date time format to use.
 *  \param format Format to use for date time.
 *  \param which When to use this format.
 *  \result None.
 */
void displaySetDateFormat (char *format, int which)
{
	if (strlen (format) <= 40 && which < 4 && which >= 0)
		strcpy (dateFormats[which], format);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  F I L E  S I Z E                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Make a file size string, converts to K, M .
 *  \param size Size to convert to a string.
 *  \param outString Output the size to this string.
 *  \result Pointer to the outString.
 */
char *displayFileSize (long long size, char *outString)
{
	int i = 0, l;
	static char sizeTypes[] = "BKMGTPEZY?";

	while (size >= (long long)10240 && i < 9)
	{
		i++;
		size >>= 10;
	}

	displayCommaNumber (size, outString);
	l = strlen (outString);
	outString[l++] = sizeTypes[i];
	outString[l] = 0;

	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  R I G H T S  S T R I N G                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Make a rights string from file rights.
 *  \param userRights User right for the file to convert.
 *  \param outString Output the rights to this string.
 *  \result Pointer to the outString.
 */
char *displayRightsString (int userRights, char *outString)
{
	int i, j = 9, saveRights = userRights;

	strcpy (outString, "----------");

	if (S_ISSOCK(userRights))
		outString[0] = 's';
	else if (S_ISLNK(userRights))
		outString[0] = 'l';
	else if (S_ISFIFO(userRights))
		outString[0] = 'f';
	else if (S_ISBLK(userRights))
		outString[0] = 'b';
	else if (S_ISCHR(userRights))
		outString[0] = 'c';
	else if (S_ISDIR(userRights))
		outString[0] = 'd';

	for (i = 0; i < 3; i++)
	{
		switch (i)
		{
		case 0:
			outString[j] = (saveRights & S_ISVTX) ? (userRights & 1 ? 't' : 'T') : (userRights & 1 ? 'x' : '-');
			break;
		case 1:
			outString[j] = (saveRights & S_ISGID) ? (userRights & 1 ? 's' : 'S') : (userRights & 1 ? 'x' : '-');
			break;
		case 2:
			outString[j] = (saveRights & S_ISUID) ? (userRights & 1 ? 's' : 'S') : (userRights & 1 ? 'x' : '-');
			break;
		}
		j--;

		if (userRights & 2)
			outString[j] = 'w';
		j--;

		if (userRights & 4)
			outString[j] = 'r';
		j--;

		userRights >>= 3;
	}

	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  R I G H T S  S T R I N G  A C L                                                                    *
 *  ==============================================                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Same as the displayRightsString but adds an ACL marker like ls.
 *  \param file File we are looking at.
 *  \param outString Output the attribute string here.
 *  \result Pointer to the string.
 */
char *displayRightsStringACL (DIR_ENTRY *file, char *outString)
{
#ifdef HAVE_SYS_ACL_H
	acl_t acl;
	char fullName[1024];
#endif

	displayRightsString (file -> fileStat.st_mode, outString);

#ifdef HAVE_SYS_ACL_H
	outString[10] = '.';
	outString[11] = 0;

	strcpy (fullName, file -> fullPath);
	strcat (fullName, file -> fileName);
	if ((acl = acl_get_file (fullName, ACL_TYPE_ACCESS)) != NULL)
	{
		acl_entry_t entry;
		acl_tag_t tag;
		int entryId = ACL_FIRST_ENTRY;

		while (acl_get_entry(acl, entryId, &entry) == 1)
		{
			if (acl_get_tag_type (entry, &tag) != -1)
			{
				if (tag == ACL_USER || tag == ACL_GROUP)
				{
					outString[10] = '+';
					break;
				}
			}
			entryId = ACL_NEXT_ENTRY;
		}
		acl_free (acl);
	}
#endif
	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  O W N E R  S T R I N G                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the owners name from there ID.
 *  \param ownerID The ID of the owner to get the name of.
 *  \param outString Output the owner name to this string.
 *  \result Pointer to the outString.
 */
char *displayOwnerString (int ownerID, char *outString)
{
	struct passwd *pwd;

	if ((pwd = getpwuid(ownerID)) != NULL)
		sprintf(outString, "%s", pwd -> pw_name);
	else
		sprintf(outString, "%d", ownerID);

	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  G R O U P  S T R I N G                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the group name from its ID.
 *  \param groupID The ID of the group to get the name of.
 *  \param outString Output the group name to this string.
 *  \result Pointer to the outString.
 */
char *displayGroupString (int groupID, char *outString)
{
	struct group *grp;

	if ((grp = getgrgid(groupID)) != NULL)
		sprintf(outString, "%s", grp -> gr_name);
	else
		sprintf(outString, "%d", groupID);

	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  C O N T E X T  S T R I N G                                                                         *
 *  =========================================                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the context for a given file.
 *  \param fullpath The file to get the context of.
 *  \param outString Save the context here.
 *  \result Pointer to the saved context.
 */
char *displayContextString (char *fullpath, char *outString)
{
#ifdef HAVE_SELINUX_SELINUX_H
	security_context_t getContext;
	int size = lgetfilecon (fullpath, &getContext);
	char *ptr = (char *)getContext;

	if (size > 0)
	{
		strncpy (outString, ptr, 80);
		outString[80] = 0;
		freecon (getContext);
	}
	else
	{
		if (errno == ENODATA)
			strcpy (outString, "Context not found");
		else if (errno == ENOTSUP)
			strcpy (outString, "Context not setup");
		else
			strcpy (outString, "Context not supported");
	}
#else
	strcpy (outString, "Context not supported");
#endif
	return outString;
}

static char hexCharVals[] = "0123456789abcdef";

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  E N C O D E  H E X                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Encode a binary buffer into hex.
 *  \param inBuff Buffer to be encoded.
 *  \param outBuff Output buffer, it need to be 100% bigger than the input.
 *  \param len Size of the input buffer.
 *  \result String length of the output buffer.
 */
int displayEncodeHex (unsigned char *inBuff, char *outBuff, int len)
{
	int i;
	for (i = 0; i < len; ++i)
	{
		unsigned int ch = inBuff[i];
		outBuff[i * 2] = hexCharVals[(ch >> 4) & 0x0F];
		outBuff[(i * 2) + 1] = hexCharVals[ch & 0x0F];
	}
	outBuff[i * 2] = 0;
	return i * 2;
}

/*                       |123456789ABCDEF|123456789ABCDEF|123456789ABCDEF|123456789ABCDEF|1 */
/*                       |123456789|123456789|123456789|123456789|123456789|123456789|12345 */
char base64CharVals[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=!";

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  E N C O D E  B A S E 6 4                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Encode a binary buffer into base64.
 *  \param inBuff Buffer to be encoded.
 *  \param outBuff Output buffer, it need to be 50% bigger than the input.
 *  \param len Size of the input buffer.
 *  \result String length of the output buffer.
 */
int displayEncodeBase64 (unsigned char *inBuff, char *outBuff, int len)
{
	int i, j = 0, k = 0;

	while (j < len)
	{
		int todo = len - j >= 3 ? 3 : len - j;
		char outVal[4] = { 0, 0, 0, 0 };

		outVal[0] = (inBuff[j] & 0xFC) >> 2;
		outVal[1] = (inBuff[j] & 0x03) << 4;
		if (todo > 1)
		{
			outVal[1] |= (inBuff[j + 1] & 0xF0) >> 4;
			outVal[2] = (inBuff[j + 1] & 0x0F) << 2;
			if (todo > 2)
			{
				outVal[2] |= (inBuff[j + 2] & 0xC0) >> 6;
				outVal[3] = inBuff[j + 2] & 0x03F;
			}
			else
			{
				outVal[3] = 64;
			}
		}
		else
		{
			outVal[2] = 64;
			outVal[3] = 64;
		}
		for (i = 0; i < 4; ++i)
		{
			outBuff[k++] = base64CharVals[(int)outVal[i]];
		}
		outBuff[k] = 0;
		j += 3;
	}
	return k;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  M D 5 S T R I N G                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the MD5 checksum for a file.
 *  \param file File to checksum (may have been done).
 *  \param outString Output the string here .
 *  \param encode String to encode.
 *  \result None.
 */
char *displayMD5String (DIR_ENTRY *file, char *outString, int encode)
{
	if (file -> md5Sum == NULL)
	{
		if ((file -> md5Sum = malloc (17)) != NULL)
		{
			char fullName[1024];

			strcpy (fullName, file -> fullPath);
			strcat (fullName, file -> fileName);
			if (!MD5File (fullName, file -> md5Sum))
			{
				free (file -> md5Sum);
				file -> md5Sum = NULL;
			}
		}
	}
	outString[0] = 0;
	if (file -> md5Sum)
	{
		switch (encode)
		{
		case DISPLAY_ENCODE_BASE64:
			displayEncodeBase64 (file -> md5Sum, outString, 16);
			break;

		default:
			displayEncodeHex (file -> md5Sum, outString, 16);
			break;
		}
	}
	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  S H A 2 5 6 S T R I N G                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the SHA256 checksum for a file.
 *  \param file File to checksum (may have been done).
 *  \param outString Output the string here .
 *  \param encode String to encode.
 *  \result None.
 */
char *displaySHA256String (DIR_ENTRY *file, char *outString, int encode)
{
	if (file -> sha256Sum == NULL)
	{
		if ((file -> sha256Sum = malloc (33)) != NULL)
		{
			char fullName[1024];

			strcpy (fullName, file -> fullPath);
			strcat (fullName, file -> fileName);
			if (!SHA256File (fullName, file -> sha256Sum))
			{
				free (file -> sha256Sum);
				file -> sha256Sum = NULL;
			}
		}
	}
	outString[0] = 0;
	if (file -> sha256Sum != NULL)
	{
		switch (encode)
		{
		case DISPLAY_ENCODE_BASE64:
			displayEncodeBase64 (file -> sha256Sum, outString, 32);
			break;

		default:
			displayEncodeHex (file -> sha256Sum, outString, 32);
			break;
		}
	}
	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  V E R  S T R I N G                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display any version information about this file.
 *  \param file File being displayed.
 *  \param outString Output to this file.
 *  \result Pointer to version text.
 */
char *displayVerString (DIR_ENTRY *file, char *outString)
{
	int i;
	char tempStr[21];

	outString[0] = 0;
	if (file -> fileVer != NULL)
	{
		for (i = 1; i < file -> fileVer -> verVals[0]; ++i)
		{
			sprintf (tempStr, (i > 1 ? ".%d" : "%d"),  file -> fileVer -> verVals[i]);
			if (strlen (outString) + strlen (tempStr) < 200)
			{
				strcat (outString, tempStr);
			}
		}
	}
	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  G E T  W I D T H                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the width of the screen.
 *  \result The width of the screen, terminal display.
 */
int displayGetWidth ()
{
	return (ncols == -1 ? 80 : ncols);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  G E T  D E P T H                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the depth of the screen.
 *  \result The depth of the screen, terminal display.
 */
int displayGetDepth ()
{
	return (nrows == -1 ? 24 : nrows);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  G E T  W I N D O W  S I Z E                                                                        *
 *  ==========================================                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the terminal size to be read later.
 *  \result None.
 */
void displayGetWindowSize ()
{
	char *env;
	int foundSize = 0;

#ifdef TIOCGWINSZ

	struct winsize ws;

	if (!(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 &&
			ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == -1 &&
			ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1))
	{
		foundSize = 1;
	}

	if (foundSize &&
		ws.ws_col > 0	&& ws.ws_row > 0 &&
		ws.ws_col < 512 && ws.ws_row < 512)
	{
		nrows = ws.ws_row;
		ncols = ws.ws_col;
	}
	else
	{
		foundSize = 0;
	}

#endif

	if (!foundSize)
	{
		if ((env = getenv("COLUMNS")) != NULL)
		{
			ncols = atoi (env);
		}
		if (ncols < 1 || ncols > 512)
		{
			ncols = 80;
		}
		if ((env = getenv("LINES")) != NULL)
		{
			nrows = atoi (env);
		}
		if (nrows < 1 || nrows > 512)
		{
			nrows = 24;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  F O R C E  S I Z E                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Force to screen size to a value.
 *  \param cols Number of columns to set.
 *  \param rows Number of rows to set.
 *  \result None.
 */
void displayForceSize (int cols, int rows)
{
	nrows = rows;
	ncols = cols;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  C O L U M N  I N I T                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Initialise the structures for displaying columns.
 *  \param colCount Number of columns to be displayed.
 *  \param colDesc Descriptions of the columns to display.
 *  \param options Any other options for the columns, like display .
 *  \result 0 if the init failed, 1 if OK.
 */
int displayColumnInit (int colCount, COLUMN_DESC *colDesc[], int options)
{
	if (colCount > MAX_COLUMNS)
		return 0;

	if ((rowQueue = queueCreate ()) == NULL)
		return 0;

	for (columnCount = 0; columnCount < colCount; columnCount++)
	{
		fullColDesc[columnCount] = (FULL_COLUMN_DESC *)malloc (sizeof (FULL_COLUMN_DESC));
		if (fullColDesc[columnCount] == NULL)
			return 0;

		memset (fullColDesc[columnCount], 0, sizeof (FULL_COLUMN_DESC));
		fullColDesc[columnCount] -> maxWidth = colDesc[columnCount] -> maxWidth;
		fullColDesc[columnCount] -> minWidth = colDesc[columnCount] -> minWidth;
		fullColDesc[columnCount] -> displaySize = colDesc[columnCount] -> startWidth;
		fullColDesc[columnCount] -> attrib = colDesc[columnCount] -> attrib;
		fullColDesc[columnCount] -> priority = colDesc[columnCount] -> priority;
		fullColDesc[columnCount] -> colour = colDesc[columnCount] -> colour;
		fullColDesc[columnCount] -> gap = colDesc[columnCount] -> gap;
		fullColDesc[columnCount] -> maxSize = fullColDesc[columnCount] -> minWidth;
		fullColDesc[columnCount] -> heading = colDesc[columnCount] -> heading;
	}

	displayOptions = options;

	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V  D I S P L A Y  I N  C O L U M N                                                                                *
 *  ==================================                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function to add a column to the current display line.
 *  \param column Which column to add.
 *  \param format How to format the column.
 *  \param arg_ptr Arguments to the format.
 *  \result 1 is the add worked, 0 if a problem like column out of range.
 */
static int vDisplayInColumn (int column, char *format, va_list arg_ptr)
{
	char tempBuff[TEMP_BUFF_SIZE];
	int strSize, i, maxSize = TEMP_BUFF_SIZE - 1;

	if (column >= columnCount)
		return 0;

	if (currentRow == NULL)
	{
		if ((currentRow = (ROW_DESC *)malloc (sizeof (ROW_DESC))) == NULL)
			return 0;

		memset (currentRow, 0, sizeof (ROW_DESC));
		currentRow -> rowType = ROW_NORMAL_LINE;
		for (i = 0; i < columnCount; i++)
			currentRow -> colColour[i] = -1;
	}

	tempBuff[0] = 0;
	if (currentRow -> colString[column] != NULL)
	{
		/* String already in the column */
		strcpy (tempBuff, currentRow -> colString[column]);
		free (currentRow -> colString[column]);
		maxSize -= strlen (tempBuff);
	}

	vsnprintf (&tempBuff[strlen (tempBuff)], maxSize, format, arg_ptr);
	strSize = strlen (tempBuff);

	if ((currentRow -> colString[column] = (char *)malloc (strSize + 1)) == NULL)
		return 0;

	strcpy (currentRow -> colString[column], tempBuff);

	if (strSize > fullColDesc[column] -> maxSize)
	{
		fullColDesc[column] -> maxSize = strSize;
		if (strSize < fullColDesc[column] -> maxWidth)
			fullColDesc[column] -> displaySize = strSize;
		else
			fullColDesc[column] -> displaySize = fullColDesc[column] -> maxWidth;
	}
	else if (strSize > fullColDesc[column] -> displaySize)
	{
		fullColDesc[column] -> displaySize = strSize;
	}
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  I N  C O L O U R                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display in a different colour to the default.
 *  \param column Number of the column to display the string in.
 *  \param colour Colour to use.
 *  \param format Format of the string (printf style) maybe more op.
 *  \param ... Parameters to format.
 *  \result 0 if the display failed, 1 if OK.
 */
int displayInColour (int column, int colour, char *format, ...)
{
	int retn = 1;
	va_list arg_ptr;

	va_start (arg_ptr, format);
	if ((retn = vDisplayInColumn (column, format, arg_ptr)) == 1)
	{
		if (currentRow != NULL)
		{
			currentRow -> colColour[column] = colour;
		}
	}
	va_end (arg_ptr);
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  V I N  C O L O U R                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display in a different colour to the default.
 *  \param column Number of the column to display the string in.
 *  \param colour Colour to use.
 *  \param format Format of the string (printf style) maybe more op.
 *  \param arg_ptr Parameters to format.
 *  \result 0 if the display failed, 1 if OK.
 */
int displayVInColour (int column, int colour, char *format, va_list arg_ptr)
{
	int retn = vDisplayInColumn (column, format, arg_ptr);

	if (retn == 1 && currentRow != NULL)
	{
		currentRow -> colColour[column] = colour;
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  I N  C O L U M N                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display a string in a column.
 *  \param column Number of the column to display the string in.
 *  \param format Format of the string (printf style) maybe more op.
 *  \param ... Parameters to format.
 *  \result 0 if the display failed, 1 if OK.
 */
int displayInColumn (int column, char *format, ...)
{
	int retn = 1;
	va_list arg_ptr;

	va_start (arg_ptr, format);
	retn = vDisplayInColumn (column, format, arg_ptr);
	va_end (arg_ptr);
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  V I N  C O L U M N                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display a string in a column.
 *  \param column Number of the column to display the string in.
 *  \param format Format of the string (printf style) maybe more op.
 *  \param arg_ptr Parameters to format.
 *  \result 0 if the display failed, 1 if OK.
 */
int displayVInColumn (int column, char *format, va_list arg_ptr)
{
	return vDisplayInColumn (column, format, arg_ptr);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A L C  D I S P L A Y  S I Z E                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the total size of all the columns.
 *  \result The size of all the columns.
 */
static int calcDisplaySize (void)
{
	int retnSize = 0, i;

	for (i = 0; i < columnCount; i++)
	{
		if (fullColDesc[i] -> displaySize)
		{
			retnSize += fullColDesc[i] -> displaySize;
			retnSize += fullColDesc[i] -> gap;
		}
	}
	return retnSize;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A D D  H E A D I N G  S I Z E S                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Add the sizes of the headings to the column sizes.
 *  \result None.
 */
static void addHeadingSizes (void)
{
	int i;

	for (i = 0; i < columnCount; i++)
	{
		if (fullColDesc[i] -> displaySize > 0 && fullColDesc[i] -> heading != NULL)
		{
			if (strlen (fullColDesc[i] -> heading) > fullColDesc[i] -> displaySize)
			{
				/*------------------------------------------------------------*
                 * Assume the heading size is less than the maxSize           *
                 *------------------------------------------------------------*/
				fullColDesc[i] -> displaySize = strlen (fullColDesc[i] -> heading);
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E D U C E  D I S P L A Y  S I Z E                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Look at the columns an see if we can make them smaller.
 *  \result 1 if the size was changed, 0 if they are as small as possi.
 */
static int reduceDisplaySize(void)
{
	int reduceCol = -1, reducePri = 0, sizeDiff = 0, gapSize = 0, i;

	for (i = 0; i < columnCount; i++)
	{
		if (fullColDesc[i] -> gap > 1)
		{
			if (fullColDesc[i] -> gap >= gapSize)
			{
				gapSize = fullColDesc[i] -> gap;
				reduceCol = i;
			}
		}
	}
	if (reduceCol != -1)
	{
		fullColDesc[reduceCol] -> gap --;
		return 1;
	}

	for (i = 0; i < columnCount; i++)
	{
		if (fullColDesc[i] -> displaySize > fullColDesc[i] -> minWidth)
		{
			if (fullColDesc[i] -> displaySize - fullColDesc[i] -> minWidth > sizeDiff)
			{
				sizeDiff = fullColDesc[i] -> displaySize - fullColDesc[i] -> minWidth;
				reduceCol = i;
			}
		}
	}
	if (reduceCol != -1)
	{
		fullColDesc[reduceCol] -> displaySize --;
		return 1;
	}

	for (i = 0; i < columnCount; i++)
	{
		if (fullColDesc[i] -> priority >  reducePri && fullColDesc[i] -> displaySize)
		{
			reducePri = fullColDesc[i] -> priority;
			reduceCol = i;
		}
	}
	if (reduceCol != -1)
	{
		fullColDesc[reduceCol] -> displaySize = 0;
		return 1;
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I X  S T R I N G  S I Z E                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Make an over size string smaller to fit a column.
 *  \param dispString String to make smaller.
 *  \param displaySize Size that the fixed string should be.
 *  \result None.
 */
static void fixStringSize (char *dispString, int displaySize)
{
	int len = strlen (dispString);

	if (len > displaySize)
	{
		if (displaySize > 4)
		{
			int x = (displaySize - 3) / 2;
			int y = len - (displaySize - 3 - x);

			strcpy (&dispString[x], "...");
			x += 3;
			do
			{
				dispString[x++] = dispString[y++];
			}
			while (dispString[y]);
			dispString[x] = 0;
		}
		else
		{
			dispString[displaySize] = 0;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  C O L O U R                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a colour to an ansi esape code.
 *  \param colour Colour to convert.
 *  \param outString Output colour as an escape code.
 *  \result None.
 */
static void displayColour (int colour, char *outString)
{
	strcpy (outString, "\033[");
	if (colour & 0x08)
	{
		strcat (outString, "1;");
	}
	if (colour & 0x80)
	{
		strcat (outString, "4;");
	}
	if (colour & 0x07)
	{
		sprintf (&outString[strlen(outString)], "%d", 30 + (colour & 0x07));
	}
	colour = (colour >> 4) & 0x07;
	if (colour & 0x07)
	{
		sprintf (&outString[strlen(outString)], "%d", 40 + (colour & 0x07));
	}
	strcat (outString, "m");
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  C O L U M N                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Output one of the columns to the screen.
 *  \param column The number of the column we are showing.
 *  \param string The string to show within the column.
 *  \param showColour Should we display in colour.
 *  \result None.
 */
static void displayColumn (int column, char *string, int showColour)
{
	if (fullColDesc[column] -> displaySize > 0)
	{
		char blankString[2] = "";
		char tempString[80];
		char *dispString;
		int strSize;

		/*--------------------------------------------------------------------*
         * Prepare the string to be displayed.                                *
         *--------------------------------------------------------------------*/
		if (string == NULL)
			string = blankString;

		strSize = strlen (string);
		dispString = (char *)malloc (strSize + 1);
		strcpy (dispString, string);

		if (strSize > fullColDesc[column] -> displaySize)
		{
			fixStringSize (dispString, fullColDesc[column] -> displaySize);
		}
		strSize = strlen (dispString);

		/*--------------------------------------------------------------------*
         * If right aligned add the padding first.                            *
         *--------------------------------------------------------------------*/
		if (fullColDesc[column] -> attrib & COL_ALIGN_RIGHT)
		{
			if (strSize < fullColDesc[column] -> displaySize)
			{
				sprintf (tempString, "%%%ds", fullColDesc[column] -> displaySize - strSize);
				printf (tempString, "");

			}
		}

		/*--------------------------------------------------------------------*
         * If displaying colours set them now.                                *
         *--------------------------------------------------------------------*/
		if (showColour != -1)
		{
			displayColour (showColour, tempString);
			fputs (tempString, stdout);
		}

		/*--------------------------------------------------------------------*
         * Display the string with a simple puts.                             *
         *--------------------------------------------------------------------*/
		fputs (dispString, stdout);

		if (showColour != -1)
			fputs ("\033[0m", stdout);

		/*--------------------------------------------------------------------*
         * If aligned left add the padding after the string.                  *
         *--------------------------------------------------------------------*/
		if (!(fullColDesc[column] -> attrib & COL_ALIGN_RIGHT))
		{
			if (strSize < fullColDesc[column] -> displaySize)
			{
				sprintf (tempString, "%%%ds", fullColDesc[column] -> displaySize - strSize);
				printf (tempString, "");
			}
		}

		/*--------------------------------------------------------------------*
         * Finally display the gap between columns.                           *
         *--------------------------------------------------------------------*/
		if (fullColDesc[column] -> gap)
		{
			sprintf (tempString, "%%%ds", fullColDesc[column] -> gap);
			printf (tempString, "");
		}
		free (dispString);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  M A T C H  W I D T H                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to make all the columns the same width.
 *  \result None.
 */
void displayMatchWidth (void)
{
	int i, maxSize = 0;

	for (i = 0; i < columnCount; i++)
	{
		if (fullColDesc[i] -> displaySize > maxSize)
			maxSize = fullColDesc[i] -> displaySize;
	}
	if (maxSize)
	{
		for (i = 0; i < columnCount; i++)
		{
			if (maxSize <= fullColDesc[i] -> maxWidth)
				fullColDesc[i] -> displaySize = maxSize;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  N E W  L I N E                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Finish adding columns to a line and start a new one.
 *  \param flags Display flags.
 *  \result None.
 */
void displayNewLine (char flags)
{
	if (currentRow != NULL)
	{
		if (flags & DISPLAY_INFO)
			currentRow -> rowType = ROW_DISPLAY_INFO;
		else
			displayLines ++;

		if (flags & DISPLAY_FIRST)
			queuePush (rowQueue, currentRow);
		else
			queuePut (rowQueue, currentRow);

		currentRow = NULL;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  D R A W  L I N E                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Add a special line the will draw a line of dashes.
 *  \param flags Display flags.
 *  \result None.
 */
void displayDrawLine (char flags)
{
	if (rowQueue)
	{
		ROW_DESC *tempRow;

		if ((tempRow = (ROW_DESC *)malloc (sizeof (ROW_DESC))) == NULL)
			return;

		memset (tempRow, 0, sizeof (ROW_DESC));
		tempRow -> rowType = ROW_DISPLAY_LINE;
		displayNewLine (flags);

		if (flags & DISPLAY_FIRST)
			queuePush (rowQueue, tempRow);
		else
			queuePut (rowQueue, tempRow);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  B L A N K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Add a special line the will draw a blank line.
 *  \param flags Display flags.
 *  \result None.
 */
void displayBlank (char flags)
{
	if (rowQueue)
	{
		ROW_DESC *tempRow;

		if ((tempRow = (ROW_DESC *)malloc (sizeof (ROW_DESC))) == NULL)
			return;

		memset (tempRow, 0, sizeof (ROW_DESC));
		tempRow -> rowType = ROW_DISPLAY_BLANK;
		displayNewLine (flags);

		if (flags & DISPLAY_FIRST)
			queuePush (rowQueue, tempRow);
		else
			queuePut (rowQueue, tempRow);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  H E A D I N G                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Add a special line that will display the column headings.
 *  \param flags Display flags.
 *  \result None.
 */
void displayHeading (char flags)
{
	if (rowQueue)
	{
		ROW_DESC *tempRow;

		if ((tempRow = (ROW_DESC *)malloc (sizeof (ROW_DESC))) == NULL)
			return;

		memset (tempRow, 0, sizeof (ROW_DESC));
		tempRow -> rowType = ROW_DISPLAY_HEADING;
		displayNewLine (flags);

		if (flags & DISPLAY_FIRST)
			queuePush (rowQueue, tempRow);
		else
			queuePut (rowQueue, tempRow);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  S O M E  L I N E S                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display some of the lines and discard the rest.
 *  \param lines The number of lines to show, eg: 5 the first 5, -5 the last 5.
 *  \result None.
 */
void displaySomeLines (int lines)
{
	if (lines >= 0)
		displayEndLine = lines;
	else if (lines < 0)
	{
		displayStartLine = displayLines + lines;
		displayEndLine = displayLines;
	}
	displayAllLines ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  A L L  L I N E S                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display all the lines saved in the columns.
 *  \result None.
 */
void displayAllLines (void)
{
	int i, line = 0;
	ROW_DESC *displayRow;

	if (currentRow != NULL)
		displayNewLine (0);

	if (displayOptions & DISPLAY_HEADINGS)
	{
		if (!(displayOptions & DISPLAY_HEADINGS_NB))
		{
			displayDrawLine (DISPLAY_FIRST);
		}
		displayHeading (DISPLAY_FIRST);
		if (!(displayOptions & DISPLAY_HEADINGS_NT))
		{
			displayDrawLine (DISPLAY_FIRST);
		}
		addHeadingSizes ();
	}

	while (calcDisplaySize() > displayGetWidth())
	{
		if (!reduceDisplaySize())
			break;
	}

	while ((displayRow = (ROW_DESC *)queueGet (rowQueue)) != NULL)
	{
		switch (displayRow -> rowType)
		{
		case ROW_DISPLAY_LINE:
			displayLine();
			break;

		case ROW_DISPLAY_BLANK:
			printf ("\n");
			break;

		case ROW_DISPLAY_HEADING:
			{
				char headingBuff[41];
				for (i = 0; i < columnCount; i++)
				{
					headingBuff[0] = 0;

					if (fullColDesc[i] -> heading && fullColDesc[i] -> displaySize)
					{
						strncpy (headingBuff, fullColDesc[i] -> heading, 40);
						headingBuff[40] = 0;
					}
					displayColumn (i, headingBuff, -1);
				}
			}
			printf ("\n");
			break;

		case ROW_NORMAL_LINE:
			{
				int noShow = ((displayStartLine > 0 && line < displayStartLine) ||
						line >= displayEndLine);

				for (i = 0; i < columnCount; i++)
				{
					if (!noShow)
					{
						int colour = -1;

						if (displayOptions & DISPLAY_COLOURS)
						{
							if (displayRow -> colColour[i] != -1)
								colour = displayRow -> colColour[i];
							else if (fullColDesc[i] -> colour != -1)
								colour = fullColDesc[i] -> colour;
						}
						displayColumn (i, displayRow -> colString[i], colour);
					}
					if (displayRow -> colString[i] != NULL)
						free (displayRow -> colString[i]);
				}
				if (!noShow)
					printf ("\n");
			}
			line ++;
			break;

		case ROW_DISPLAY_INFO:
			for (i = 0; i < columnCount; i++)
			{
				displayColumn (i, displayRow -> colString[i], -1);

				if (displayRow -> colString[i] != NULL)
					free (displayRow -> colString[i]);
			}
			printf ("\n");
			line ++;
			break;
		}
		free (displayRow);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  T I D Y                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Tidy up and free all memory used by the display functions.
 *  \result None.
 */
void displayTidy (void)
{
	int i;

	for (i = 0; i < MAX_COLUMNS; i++)
	{
		if (fullColDesc[i])
		{
			free (fullColDesc[i]);
			fullColDesc[i] = NULL;
		}
	}
	columnCount = 0;

	if (currentRow)
	{
		for (i = 0; i < MAX_COLUMNS; i++)
		{
			if (currentRow -> colString[i])
				free (currentRow -> colString[i]);
		}
		free (currentRow);
	}
	queueDelete (rowQueue);
	rowQueue = NULL;

}
