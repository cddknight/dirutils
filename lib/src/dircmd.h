/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R C M D . H                                                                                                   *
 *  ===============                                                                                                   *
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
 *  \brief Headers for libdircmd.
 */
#ifndef INCLUDE_DIRCMD_H
#define INCLUDE_DIRCMD_H

#include <config.h> 
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_VALUES_H
#include <values.h>
#else
#define MAXINT 2147483647
#endif
#include <sys/stat.h>

#ifdef DOS
	#define DIRSEP			'\\'
	#define PATHSEP			';'
#else
	#define DIRSEP			'/'
	#define PATHSEP			':'
#endif

#define PATH_SIZE			PATH_MAX

/** 
 *  @def ALLFILES
 *  @brief Match all file types in the directory.
 */
#define ALLFILES			0x007F

/** 
 *  @def ONLYNEXCS
 *  @brief Match only non-executable files in the directory.
 */
#define ONLYNEXCS			0x0001

/** 
 *  @def ONLYDIRS
 *  @brief Match only sub-directories in the directory.
 */
#define ONLYDIRS			0x0002

/** 
 *  @def ONLYLINKS
 *  @brief Match only links in the directory.
 */
#define ONLYLINKS			0x0004

/** 
 *  @def ONLYEXECS
 *  @brief Match only executable files in the directory.
 */
#define ONLYEXECS			0x0008

/** 
 *  @def ONLYFILES
 *  @brief Match only files in the directory.
 */
#define ONLYFILES			0x0009

/** 
 *  @def ONLYDEV
 *  @brief Match only DEVICES in the directory.
 */
#define ONLYDEVS			0x0010

/** 
 *  @def ONLYDEV
 *  @brief Match only DEVICES in the directory.
 */
#define ONLYSOCKS			0x0020

/** 
 *  @def ONLYDEV
 *  @brief Match only DEVICES in the directory.
 */
#define ONLYPIPES			0x0040

/** 
 *  @def USECASE
 *  @brief Do a case dependant compare.
 */
#define USECASE				0x0100

/** 
 *  @def SHOWALL
 *  @brief Show all files even those starting with a dot.
 */
#define SHOWALL				0x0200

/** 
 *  @def RECUDIR
 *  @brief Include subdirectories in the directory.
 */
#define RECUDIR				0x0400

/** 
 *  @def MAX_COLUMNS
 *  @brief Maximum number of columns allowed.
 */
#define MAX_COLUMNS			50

/** 
 *  @def COL_ALIGN_RIGHT
 *  @brief Flag set if the column should be right aligned.
 *
 *  Used in COLUMN_DESC::attrib.
 */
#define COL_ALIGN_RIGHT		1	/* default left */

/** 
 *  @def COL_CAN_DELETE
 *  @brief Flag set if the column can be deleted on small terminals.
 *
 *  Used in COLUMN_DESC::attrib.
 */
#define COL_CAN_DELETE		2	/* can delete column to save size */

/** 
 *  @def DISPLAY_HEADINGS
 *  @brief Flag set if the headings should be added to the table.
 *
 *  Used in  displayColumnInit options.
 */
#define DISPLAY_HEADINGS	1
#define DISPLAY_COLOURS		2

#define DISPLAY_FIRST		1
#define DISPLAY_INFO		2

/**
 *  @struct columnDesc dircmd.h
 *  @brief Used when calling displayColumnInit.
 */
struct columnDesc
{
	/** The maximum size of the column */
	int maxWidth;
	/** The minimum size of the column */
	int minWidth;
	/** The starting column width */
	int startWidth;
	/** The gap after of the column */
	int gap;
	/** The colour to use for the column */
	int colour;
	/** The attributes of the column */
	int attrib;
	/** A text heading for the column */
	char *heading;
	/** Order to be deleted */
	int priority;
};

/**
 *  @typedef columnDesc COLUMN_DESC
 *  @brief Type definition of the column description.
 */
typedef struct columnDesc COLUMN_DESC;

/**
 *  @struct dirEntry dircmd.h
 *  @brief Used to pass directory information to call back functions.
 */
struct dirEntry
{
	/** Name of the file */
	char *fileName;
	/** Path to the file */
	char *fullPath;
	/** Path as it goes into sub directories */
	char *partPath;
	/** MD5 checksum if needed */
	unsigned char *md5Sum;
	/** Directory information */
	struct stat fileStat;
	/** Has the CRC been calculated for this file */
	unsigned int doneCRC;
	/** CRC value for this file */
	unsigned int CRC;
	/** Was a match found for this file */
	unsigned int match;
	/** Pointer to function used to compare the files */
	int(*Compare)(struct dirEntry *f1, struct dirEntry *f2);
};

/**
 *  @typedef dirEntry DIR_ENTRY
 *  @brief Type definition of the directory entry.
 */
typedef struct dirEntry DIR_ENTRY;

/*
 *  dircmd.c
 */
char *directoryVersion(void);
int directoryLoad (char *inPath, int findFlags,
		int(*Compare)(DIR_ENTRY *f1, DIR_ENTRY *f2),
		void **fileList);
int directorySort (void **fileList);
int directoryProcess (int(*ProcFile)(DIR_ENTRY *f1), void **fileList);

/*
 *  crc.c
 */
int CRCFile (char *filename);
int MD5File (char *filename, unsigned char *md5Buffer);

/*
 *  display.c
 */
void displayLine (void);
char *displayCommaNumber (long long number, char *outString);
char *displayDateString (time_t showDate, char *outString);
void displaySetDateFormat (char *format, int which);
char *displayFileSize (long long size, char *outString);
char *displayRightsString (int userRights, char *outString);
char *displayRightsStringACL (DIR_ENTRY *file, char *outString);
char *displayOwnerString (int ownerID, char *outString);
char *displayGroupString (int groupID, char *outString);
char *displayContextString (char *fullpath, char *outString);
char *displayMD5String (DIR_ENTRY *file, char *outString);
void displayGetWindowSize (void);
void displayForceSize (int cols, int rows);
int displayGetWidth (void);
int displayGetDepth (void);
int displayColumnInit (int colCount, COLUMN_DESC *colDesc[], int options);
int displayInColumn (int column, char *format, ...);
int displayInColour (int column, int colour, char *format, ...);
int displayVInColumn (int column, char *format, va_list arg_ptr);
int displayVInColour (int column, int colour, char *format, va_list arg_ptr);
void displayMatchWidth (void);
void displaySomeLines (int lines);
void displayAllLines (void);
void displayNewLine (char flags);
void displayDrawLine (char flags);
void displayHeading (char flags);
void displayBlank (char flags);
void displayTidy (void);

/*
 *  list.c
 */
void *queueCreate (void);
void  queueDelete (void *queueHandle);
void *queueGet (void *queueHandle);
void  queuePut (void *queueHandle, void *putData);
void  queuePutSort (void *queueHandle, void *putData, 
	  		int(*Compare)(const void *item1, const void *item2));
void  queuePush (void *queueHandle, void *putData);
void *queueRead (void *queueHandle, int item);
void queueSetFreeData (void *queueHandle, unsigned long setData);
unsigned long queueGetFreeData (void *queueHandle);
unsigned long queueGetItemCount (void *queueHandle);
void queueSort (void *queueHandle, 
			int(*Compare)(const void *item1, const void *item2));


/*
 *  match.c
 */
int matchLogic (char *str, char *ptn, int flags);
int matchPattern (char *str, char *ptn, int flags);

/*
 * config.c
 */
int configLoad (const char *configFile);
void configFree ();
int configSetValue (char *configName, char *configValue);
int configSetIntValue (char *configName, int configValue);
int configGetValue (char *configName, char *value);
int configGetIntValue (const char *configName, int *value);


#endif
