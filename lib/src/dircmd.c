/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R C M D . C                                                                                                   *
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
 *  \brief Functions to process directories.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dircmd.h"
#include "config.h"

/******************************************************************************************************
 * Prototypes															                              *
 ******************************************************************************************************/
static int listCompare (const void **item1, const void **item2);
static int getEntryType (struct stat *fileStat);
static char directoryVersionText[] = VERSION;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R E C T O R Y  V E R S I O N                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Return the version number of the dir library.
 *  \result Pointer to the version number.
 */
char *directoryVersion(void)
{
	return directoryVersionText;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T R C A T _ C H                                                                                                 *
 *  =================                                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Internal fuction to add a char to the end of a string.
 *  \param buff String to add the char to.
 *  \param ch Char to add the the string.
 *  \result Pointer to the string.
 */
static char *strcat_ch (char *buff, char ch)
{
	int l = strlen(buff);

	if (l)
	{
		if (buff[l - 1] != ch)
		{
			buff[l + 1] = 0;
			buff[l] = ch;
		}
	}
	else
	{
		buff[0] = ch;
		buff[1] = 0;
	}

	return buff;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R E C T O R Y  L O A D  I N T                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the contents of a directory into memory.
 *  \param inPath The path to the directory to be process.
 *  \param partPath If it is recursive keep the subdirs to be added to t.
 *  \param findFlags Various options to select what files to read.
 *  \param f2 .
 *  \param fileList Where to save the directory.
 *  \result The number of files found.
 */
static int directoryLoadInt (char *inPath, char *partPath, int findFlags,
		int(*Compare)(DIR_ENTRY *f1, DIR_ENTRY *f2),
		void **fileList)
{
	DIR *dirPtr;
	int filesFound = 0;
	struct dirent *dirList;
	char fullPath[PATH_SIZE], filePattern[PATH_SIZE], *endPath;

	strcpy (fullPath, inPath);
	if ((endPath = strrchr (fullPath, DIRSEP)) != NULL)
	{
		if (*(++endPath))
		{
			strcpy (filePattern, endPath);
			*endPath = 0;
		}
		else
			strcpy (filePattern, "*");
	}
	else
	{
		strcpy (filePattern, fullPath);
		if (getcwd (fullPath, PATH_SIZE - 2) == NULL)
			strcpy (fullPath, ".");
		strcat_ch (fullPath, DIRSEP);
	}

	if (!(*fileList))
	{
		if ((*fileList = queueCreate ()) == NULL)
			return 0;
	}

    /*------------------------------------------------------------------------*
     * Open the directory we plan to view                                     *
     *------------------------------------------------------------------------*/
	if ((dirPtr = opendir (fullPath)) != NULL)
	{
		endPath = &fullPath[strlen(fullPath)];

		while ((dirList = readdir (dirPtr)) != NULL)
		{
		    /*----------------------------------------------------------------*
			 * Recursive directories, avoid '.' and '..'                      *
             *----------------------------------------------------------------*/
			if (findFlags & RECUDIR && 
					(dirList -> d_name[0] != '.' || findFlags & SHOWALL) && 
					strcmp (dirList -> d_name, ".") && strcmp (dirList -> d_name, ".."))
			{
				struct stat tempStat;
				char tempPath[PATH_SIZE];
				char subPath[PATH_SIZE];

				*endPath = 0;
				strcpy (tempPath, fullPath);
				strcat_ch (tempPath, DIRSEP);
				strcat (tempPath, dirList -> d_name);

				if (lstat (tempPath, &tempStat) == 0)
				{
					if (getEntryType (&tempStat) & ONLYDIRS)
					{
					    /*----------------------------------------------------*
						 * Hide any directories linked with version control   *
			             *----------------------------------------------------*/
						if (findFlags & HIDEVERCTL)
						{
							if (strcmp (dirList -> d_name, "CVS") == 0 || 
									strcmp (dirList -> d_name, ".git") == 0 || 
									strcmp (dirList -> d_name, ".svn") == 0)
							{
								continue;
							}
						}
						strcpy (tempPath, fullPath);
						strcat_ch (tempPath, DIRSEP);
						strcat (tempPath, dirList -> d_name);
						strcat_ch (tempPath, DIRSEP);
						strcat (tempPath, filePattern);

						strcpy (subPath, partPath);
						strcat (subPath, dirList -> d_name);
						strcat_ch (subPath, DIRSEP);

						filesFound += directoryLoadInt (tempPath, subPath, findFlags,
								Compare, fileList);
					}
				}
			}

            /*----------------------------------------------------------------*
             * Does this file match our pattern, if yes then show it to the   *
			 * user.                                                          *
             *----------------------------------------------------------------*/
			if (matchLogic (dirList -> d_name, filePattern, findFlags))
			{
				DIR_ENTRY *saveEntry = malloc (sizeof (DIR_ENTRY));

				saveEntry -> fileName = malloc (strlen (dirList -> d_name) + 1);
				saveEntry -> fullPath = malloc (strlen (fullPath) + 1);
				saveEntry -> partPath = malloc (strlen (partPath) + 1);
				saveEntry -> md5Sum = NULL;

				*endPath = 0;
				strcpy (saveEntry -> fileName, dirList -> d_name);
				strcpy (saveEntry -> fullPath, fullPath);
				strcpy (saveEntry -> partPath, partPath);
				strcpy (endPath, dirList -> d_name);
				saveEntry -> doneCRC = saveEntry -> CRC = saveEntry -> match = 0;
				saveEntry -> Compare = Compare;

                /*------------------------------------------------------------*
				 * The 'STAT' function we get the full low down on file and   *
				 * stores it in fileStat                                      *
                 *------------------------------------------------------------*/
				lstat (fullPath, &saveEntry -> fileStat);

				if (getEntryType (&saveEntry -> fileStat) & findFlags)
				{
					if (strlen (saveEntry -> fileName) > queueGetFreeData (*fileList))
						queueSetFreeData (*fileList, strlen (saveEntry -> fileName));

					queuePut (*fileList, saveEntry);
					filesFound ++;
				}
				else
				{
					free (saveEntry -> partPath);
					free (saveEntry -> fullPath);
					free (saveEntry -> fileName);
					free (saveEntry);
				}
			}
		}
        /*--------------------------------------------------------------------*
         * All done so close the directory and tell them how many files and   *
		 * directories were found                                             *
         *--------------------------------------------------------------------*/
		closedir (dirPtr);
	}
	return filesFound;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R E C T O R Y  L O A D                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the contents of a directory into memory.
 *  \param inPath The path to the directory to be process.
 *  \param findFlags Various options to select what files to read.
 *  \param f2 .
 *  \param fileList Where to save the directory.
 *  \result The number of files found.
 */
int directoryLoad (char *inPath, int findFlags,
		int(*Compare)(DIR_ENTRY *f1, DIR_ENTRY *f2),
		void **fileList)
{
	return directoryLoadInt (inPath, "", findFlags, Compare, fileList);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R E C T O R Y  S O R T                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Sort the directory.
 *  \param fileList List of files.
 *  \result 1 if OK.
 */
int directorySort (void **fileList)
{
	queueSort (*fileList, (void *)listCompare);
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I R E C T O R Y  P R O C E S S                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Funtion to process all the files in the directory.
 *  \param dirEntry xx.
 *  \param fileList Saved directory, loaded with directoryLoad.
 *  \result Number of files processed.
 */
int directoryProcess (int(*ProcFile)(DIR_ENTRY *dirEntry),	void **fileList)
{
	DIR_ENTRY *readEntry;
	int filesProcessed = 0;

	while ((readEntry = (DIR_ENTRY *)queueGet (*fileList)) != NULL)
	{
		if (ProcFile (readEntry))
			filesProcessed ++;

		free (readEntry -> fileName);
		free (readEntry -> fullPath);
		free (readEntry -> partPath);

		free (readEntry);
	}
	queueDelete (*fileList);
	*fileList = NULL;
	return filesProcessed;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L I S T  C O M P A R E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function called to compare two directory items.
 *  \param item1 First item to compare.
 *  \param item2 Second item to compare with.
 *  \result 1, 0 and -1 depending on callback function.
 */
static int listCompare (const void **item1, const void **item2)
{
	DIR_ENTRY *dirOne = (DIR_ENTRY *)*item1;
	DIR_ENTRY *dirTwo = (DIR_ENTRY *)*item2;

	return (dirOne -> Compare (dirOne, dirTwo));
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  E N T R Y  T Y P E                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Check the directory entry type, should we add it.
 *  \param fileStat File stat of the found file.
 *  \result The type of the file fond.
 */
static int getEntryType (struct stat *fileStat)
{
	if (S_ISLNK(fileStat -> st_mode))
		return ONLYLINKS;
	if (S_ISDIR(fileStat -> st_mode))
		return ONLYDIRS;
	if (!fileStat -> st_mode || S_ISREG (fileStat -> st_mode))
	{
		if (fileStat -> st_mode & (S_IXOTH | S_IXGRP | S_IXUSR))
			return ONLYEXECS;

		return ONLYNEXCS;
	}
	if (S_ISBLK(fileStat -> st_mode) || S_ISCHR(fileStat -> st_mode))
	{
		return ONLYDEVS;
 	}
 	if (S_ISSOCK(fileStat -> st_mode))
 		return ONLYSOCKS;
 	if (S_ISFIFO(fileStat -> st_mode))
 		return ONLYPIPES;

	printf ("OTHER: %X\n", fileStat -> st_mode);
	return 0;
}
