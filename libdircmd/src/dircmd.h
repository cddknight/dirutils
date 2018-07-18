/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R C M D . H                                                                                                   *
 *  ===============                                                                                                   *
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
 *  \brief Headers for libdircmd.
 */
#ifndef INCLUDE_DIRCMD_H
#define INCLUDE_DIRCMD_H

#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef HAVE_VALUES_H
#include <values.h>
#else
#define MAXINT 2147483647
#endif
#include <sys/stat.h>

#ifdef DOS
	#define DIRSEP				'\\'
	#define PATHSEP				';'
#else
	#define DIRSEP				'/'
	#define PATHSEP				':'
#endif

#define PATH_SIZE				PATH_MAX

/** 
 *  @def ALLFILES
 *  @brief Match all file types in the directory.
 */
#define ALLFILES				0x007F

/** 
 *  @def ONLYNEXCS
 *  @brief Match only non-executable files in the directory.
 */
#define ONLYNEXCS				0x0001

/** 
 *  @def ONLYDIRS
 *  @brief Match only sub-directories in the directory.
 */
#define ONLYDIRS				0x0002

/** 
 *  @def ONLYLINKS
 *  @brief Match only links in the directory.
 */
#define ONLYLINKS				0x0004

/** 
 *  @def ONLYEXECS
 *  @brief Match only executable files in the directory.
 */
#define ONLYEXECS				0x0008

/** 
 *  @def ONLYFILES
 *  @brief Match only files in the directory.
 */
#define ONLYFILES				0x0009

/** 
 *  @def ONLYDEV
 *  @brief Match only DEVICES in the directory.
 */
#define ONLYDEVS				0x0010

/** 
 *  @def ONLYDEV
 *  @brief Match only DEVICES in the directory.
 */
#define ONLYSOCKS				0x0020

/** 
 *  @def ONLYDEV
 *  @brief Match only DEVICES in the directory.
 */
#define ONLYPIPES				0x0040

/** 
 *  @def USECASE
 *  @brief Do a case dependant compare.
 */
#define USECASE					0x0100

/** 
 *  @def SHOWALL
 *  @brief Show all files even those starting with a dot.
 */
#define SHOWALL					0x0200

/** 
 *  @def RECUDIR
 *  @brief Include subdirectories in the directory.
 */
#define RECUDIR					0x0400

/** 
 *  @def HIDEVERCTL
 *  @brief Tide the version control subdirectories.
 */
#define HIDEVERCTL				0x0800

/** 
 *  @def MAX_COLUMNS
 *  @brief Maximum number of columns allowed.
 */
#define MAX_COLUMNS				80

/** 
 *  @def COL_ALIGN_RIGHT
 *  @brief Flag set if the column should be right aligned.
 *
 *  Used in COLUMN_DESC::attrib.
 */
#define COL_ALIGN_RIGHT			1	/* default left */

/** 
 *  @def COL_CAN_DELETE
 *  @brief Flag set if the column can be deleted on small terminals.
 *
 *  Used in COLUMN_DESC::attrib.
 */
#define COL_CAN_DELETE			2	/* can delete column to save size */

/** 
 *  @def DISPLAY_HEADINGS
 *  @brief Flag set if the headings should be added to the table.
 *
 *  Used in  displayColumnInit options.
 */
#define DISPLAY_HEADINGS		0x0001
#define DISPLAY_COLOURS			0x0002
#define DISPLAY_HEADINGS_NT		0x0004
#define DISPLAY_HEADINGS_NB		0x0008
#define DISPLAY_IN_COLUMNS		0x0010
#define DISPLAY_IN_PAGES		0x0020

#define DISPLAY_FIRST			1
#define DISPLAY_INFO			2

#define DISPLAY_ENCODE_HEX		0	// Default
#define DISPLAY_ENCODE_BASE64	1

/**
 *  @typedef comparePtr
 *  @brief Function pointer for comparing objects of unknown type.
 */
typedef int (comparePtr)(const void *first, const void *second);

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
 *  @struct dirFileVerInfo dircmd
 *  @brief Structure to store file version information.
 */
struct dirFileVerInfo
{
	char *fileStart;
	int verVals[21];
};

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
	/** SHA256 checksum if needed */
	unsigned char *sha256Sum;
	/** Version extracted from the name */
	struct dirFileVerInfo *fileVer;
	/** Directory information */
	struct stat fileStat;
	/** Has the CRC been calculated for this file */
	unsigned int doneCRC;
	/** CRC value for this file */
	unsigned int CRC;
	/** Was a match found for this file */
	unsigned int match;
	/** Pointer to function used to compare the files */
	comparePtr *Compare;
};

/**
 *  @typedef dirEntry DIR_ENTRY
 *  @brief Type definition of the directory entry.
 */
typedef struct dirEntry DIR_ENTRY;

/**
 *  @typedef compareFile
 *  @brief Function pointer for comparing two files.
 */
typedef int (compareFile)(DIR_ENTRY *first, DIR_ENTRY *second);

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

/*
 *  dircmd.c
 */
EXTERNC char *directoryVersion(void);
EXTERNC int directoryLoad (char *inPath, int findFlags, compareFile Compare, void **fileList);
EXTERNC int directorySort (void **fileList);
EXTERNC int directoryProcess (int(*ProcFile)(DIR_ENTRY *f1), void **fileList);
EXTERNC mode_t directoryTrueLinkType (DIR_ENTRY *f1);

/*
 *  crc.c
 */
EXTERNC int CRCFile (char *filename);
EXTERNC int MD5File (char *filename, unsigned char *md5Buffer);
EXTERNC int SHA256File (char *filename, unsigned char *md5Buffer);

/*
 *  display.c
 */
EXTERNC void displayInit (void);
EXTERNC void displayLine (void);
EXTERNC void displayLineChar (char dispChar);
EXTERNC char *displayCommaNumber (long long number, char *outString);
EXTERNC char *displayDateString (time_t showDate, char *outString);
EXTERNC void displaySetDateFormat (char *format, int which);
EXTERNC char *displayFileSize (long long size, char *outString);
EXTERNC char *displayRightsString (int userRights, char *outString);
EXTERNC char *displayRightsStringACL (DIR_ENTRY *file, char *outString);
EXTERNC char *displayOwnerString (int ownerID, char *outString);
EXTERNC char *displayGroupString (int groupID, char *outString);
EXTERNC char *displayContextString (char *fullpath, char *outString);
EXTERNC char *displayMD5String (DIR_ENTRY *file, char *outString, int encode);
EXTERNC char *displaySHA256String (DIR_ENTRY *file, char *outString, int encode);
EXTERNC char *displayVerString (DIR_ENTRY *file, char *outString);
EXTERNC void displayGetWindowSize (void);
EXTERNC void displayForceSize (int cols, int rows);
EXTERNC int displayGetWidth (void);
EXTERNC int displayGetDepth (void);
EXTERNC int displayColumnInit (int colCount, COLUMN_DESC *colDesc[], int options);
EXTERNC int displayInColumn (int column, char *format, ...);
EXTERNC int displayInColour (int column, int colour, char *format, ...);
EXTERNC int displayVInColumn (int column, char *format, va_list arg_ptr);
EXTERNC int displayVInColour (int column, int colour, char *format, va_list arg_ptr);
EXTERNC void displayMatchWidth (void);
EXTERNC void displaySomeLines (int lines);
EXTERNC void displayAllLines (void);
EXTERNC void displayNewLine (char flags);
EXTERNC void displayDrawLine (char flags);
EXTERNC void displayHeading (char flags);
EXTERNC void displayBlank (char flags);
EXTERNC void displayTidy (void);
EXTERNC int displayKeyPress (void);
EXTERNC void displayUpdateHeading (int column, char *heading);

/*
 *  list.c
 */
EXTERNC void *queueCreate (void);
EXTERNC void  queueDelete (void *queueHandle);
EXTERNC void *queueGet (void *queueHandle);
EXTERNC void  queuePut (void *queueHandle, void *putData);
EXTERNC void  queuePutSort (void *queueHandle, void *putData, comparePtr Compare);
EXTERNC void  queuePush (void *queueHandle, void *putData);
EXTERNC void *queueRead (void *queueHandle, int item);
EXTERNC void queueSetFreeData (void *queueHandle, unsigned long setData);
EXTERNC unsigned long queueGetFreeData (void *queueHandle);
EXTERNC unsigned long queueGetItemCount (void *queueHandle);
EXTERNC void queueSort (void *queueHandle, comparePtr Compare);

/*
 *  match.c
 */
EXTERNC int matchLogic (char *str, char *ptn, int flags);
EXTERNC int matchPattern (char *str, char *ptn, int flags);

/*
 * config.c
 */
EXTERNC int configLoad (const char *configFile);
EXTERNC int configSave (const char *configFile);
EXTERNC void configFree ();
EXTERNC int configSetValue (const char *configName, char *configValue);
EXTERNC int configSetIntValue (const char *configName, int configValue);
EXTERNC int configSetBoolValue (const char *configName, bool configValue);
EXTERNC int configGetValue (const char *configName, char *value, int maxLen);
EXTERNC int configGetIntValue (const char *configName, int *value);
EXTERNC int configGetBoolValue (const char *configName, bool *value);

#undef EXTERNC
#endif
