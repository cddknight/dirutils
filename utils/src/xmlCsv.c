/**********************************************************************************************************************
 *                                                                                                                    *
 *  X M L  C S V . C                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File xmlCsv.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms of   *
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
 *  \brief Program to parse an XML file.
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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/xmlschemas.h>
#include <libxml/HTMLparser.h>
/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);
void parsePath (char *path);
int getColumnNum (char *colName);

#define FILE_UNKNOWN	0
#define FILE_XML		1
#define FILE_HTML		2

int maxColumn;
char *columnNames[1024];
char *values[1024];
int filesFound = 0;
int fileType = FILE_UNKNOWN;
bool displayDebug = false;
bool displayPaths = true;
int levels = 0;
char *levelName[20];
char xsdPath[PATH_SIZE];
bool shownError = false;

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
	printf ("TheKnight: XML Parse a File, Version: %s, Built: %s\n", VERSION, buildDate);
	displayLine ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  H E L P  T H E M                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function to display help information.
 *  \param progName Name of the running program.
 *  \result None.
 */
void helpThem(char *progName)
{
	version();

	printf ("%s -[Options] <FileName>\n", basename (progName));
	printf ("\nOptions:\n");
	printf ("    -x . . . . . . . Force the use of the XML parser.\n");
	printf ("    -h . . . . . . . Force the use of the HTML parser.\n");
	printf ("    -d . . . . . . . Output parser debug messages.\n");
	printf ("    -P . . . . . . . Output the full path.\n");
	printf ("    -p <path>  . . . Path to search (eg: /sensors/light).\n");
	displayLine ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E A R C H  S T R                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Seach for a string in a string.
 *  \param find String to look for.
 *  \param buffer Buffer to look in.
 *  \param len Length of the buffer.
 *  \result Pointer to string in the buffer if found, NULL if not.
 */
char *searchStr (char *find, char *buffer, int len)
{
	int i = 0, j = 0;

	if (find[0])
	{
		while (i < len)
		{
			if (find[j] == buffer[i])
			{
				if (find[++j] == 0)
				{
					return (&buffer[i - (j - 1)]);
				}
			}
			else
			{
				if (find[j = 0] == buffer[i])
					++j;
			}
			++i;
		}
	}
	return NULL;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T E S T  F I L E                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Test to see if a file contains HTML.
 *  \param fileName File to test.
 *  \result 1 if html is found, 0 if not.
 */
int testFile (char *fileName)
{
	FILE *inFile;
	int retn = 0;
	char buffer[2049];

	if ((inFile = fopen (fileName, "r")) != NULL)
	{
		int readSize;
		if ((readSize = fread (buffer, 1, 2048, inFile)) > 0)
		{
			if (searchStr ("<!DOCTYPE html", buffer, readSize) != NULL)
			{
				retn = 1;
			}
			else if (searchStr ("<html", buffer, readSize) != NULL)
			{
				retn = 1;
			}
		}
		fclose (inFile);
	}
	return retn;
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
	int i, found = 0;
	char fullVersion[81];

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
	displayGetWidth();

	while ((i = getopt(argc, argv, "hxCdqPp:s:D:?")) != -1)
	{
		switch (i)
		{
		case 'x':
			fileType = FILE_XML;
			break;

		case 'h':
			fileType = FILE_HTML;
			break;

		case 'd':
			displayDebug = !displayDebug;
			break;

		case 'P':
			displayPaths = !displayPaths;
			break;

		case 'p':
			parsePath (optarg);
			break;

		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	if (optind == argc)
	{
		printf ("No files to open.\n");
		helpThem (argv[0]);
		exit (0);
	}

	for (; optind < argc; ++optind)
	{
		found += directoryLoad (argv[optind], ONLYFILES|ONLYLINKS, NULL, &fileList);
	}

	if (found)
	{
		/*--------------------------------------------------------------------*
         * Now we can sort the directory.                                     *
         *--------------------------------------------------------------------*/
		directorySort (&fileList);
		directoryProcess (showDir, &fileList);
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
 *  P A R S E  P A T H                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Split the passed in path so we know what to look for.
 *  \param path Path from the command line.
 *  \result None.
 */
void parsePath (char *path)
{
	char tempBuffer[81];
	int i = 0, j = 0;

	do
	{
		if (path[i] == '/' || path[i] == 0)
		{
			if (j && levels < 20)
			{
				levelName[levels] = (char *)malloc (j + 1);
				if (levelName[levels])
				{
					strcpy (levelName[levels], tempBuffer);
					tempBuffer[j = 0] = 0;
					++levels;
				}
			}
		}
		else if (j < 80)
		{
			tempBuffer[j] = path[i];
			tempBuffer[++j] = 0;
		}
	}
	while (path[i++]);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R M  W H I T E  S P A C E                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Remove any white space from a string.
 *  \param inBuff String to process.
 *  \param outBuff Save the output here.
 *  \param maxLen Size of the output buffer.
 *  \result Pointer to the output buffer.
 */
char *rmWhiteSpace (char *inBuff, char *outBuff, int maxLen)
{
	int lastCharWS = 1, lastSave = 0, i = 0, j = 0;

	if (inBuff != NULL)
	{
		while (inBuff[i] && j < maxLen)
		{
			if (!lastCharWS || inBuff[i] > ' ')
			{
				lastCharWS = inBuff[i] > ' ' ? 0 : 1;
				outBuff[j] = (lastCharWS ? ' ' : inBuff[i]);
				outBuff[++j] = 0;
				if (!lastCharWS) lastSave = j;
			}
			++i;
		}
	}
	outBuff[lastSave] = 0;
	return outBuff;
}

#if LIBXML_VERSION < 20700
/**********************************************************************************************************************
 *                                                                                                                    *
 *  X M L  C H I L D  E L E M E N T  C O U N T                                                                        *
 *  ==========================================                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Added function available in newer versions of libxml.
 *  \param curNode Current node.
 *  \result Count of sub nodes.
 */
int xmlChildElementCount (xmlNode *curNode)
{
	int count = 0;
	xmlNode *cNode = curNode -> children;

	for (; cNode; cNode = cNode->next)
	{
		if (cNode->type == XML_ELEMENT_NODE)
			++count;
	}
	return count;
}
#endif

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  C O L U M N  N U M                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the column number.
 *  \param colName Name of the column.
 *  \result Number of the column.
 */
int getColumnNum (char *colName)
{
	int i;

	for (i = 0; i < 1023; ++i)
	{
		if (columnNames[i] == NULL)
		{
			columnNames[i] = (char *)malloc (strlen (colName) + 1);
			if (columnNames[i] != NULL)
			{
				strcpy (columnNames[i], colName);
				maxColumn = i;
				return i;
			}
			break;
		}
		else if (strcmp (columnNames[i], colName) == 0)
		{
			return i;
		}
	}
	return -1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  E L E M E N T  N A M E S                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process each of the elements in the file.
 *  \param doc Document to read.
 *  \param aNode Current node.
 *  \param curPath Current path, added to recusivly.
 *  \param readLevel 0 current, 1 today, 2 tomorrow.
 *  \result None.
 */
static void
processElementNames (xmlDoc *doc, xmlNode * aNode, char *curPath, int readLevel)
{
	xmlChar *key;
	xmlNode *curNode = NULL;
	char tempBuff[1024], fullPath[1024];
	int i, colNum, maxColNum = -1, startLevel = 0;

	for (curNode = aNode; curNode; curNode = curNode->next)
	{
		int saveLevel = readLevel;

		if (displayPaths && curNode -> name != NULL)
		{
			if ((strlen (curPath) + strlen ((char *)curNode -> name)) > 1022)
			{
				fprintf (stderr, "XML path too long to display\n");
				break;
			}
			strcpy (fullPath, curPath);
			strcat (fullPath, "/");
			strcat (fullPath, (char *)curNode -> name);
		}

		if (curNode->type == XML_ELEMENT_NODE)
		{
			if ((!xmlStrcmp (curNode -> name, (const xmlChar *)levelName[readLevel])) || readLevel >= levels)
			{
				++readLevel;
			}
			if (readLevel >= levels)
			{
				xmlAttrPtr attr;

				if (readLevel == levels)
					startLevel = saveLevel;

				colNum = getColumnNum ((char *)curNode -> name);
				if (colNum >= 0)
				{
					if (colNum > maxColNum)
						maxColNum = colNum;

					values[colNum] = (char *)malloc (strlen (fullPath) + 1);
					if (values[colNum] != NULL)
						strcpy (values[colNum], fullPath);
				}

				key = xmlNodeListGetString (doc, curNode -> xmlChildrenNode, 1);
				if (key != NULL)
				{
					rmWhiteSpace ((char *)key, tempBuff, 1020);
					if (tempBuff[0])
					{
						colNum = getColumnNum ((char *)curNode -> name);
						if (colNum >= 0)
						{
							if (colNum > maxColNum)
								maxColNum = colNum;

							values[colNum] = (char *)malloc (strlen (tempBuff) + 1);
							if (values[colNum] != NULL)
								strcpy (values[colNum], tempBuff);
						}
					}
					xmlFree (key);
				}

				for (attr = curNode -> properties; attr != NULL; attr = attr -> next)
				{
					if (attr -> children)
					{
						colNum = getColumnNum ((char *)attr -> name);
						if (colNum >= 0)
						{
							char *value = (char *)attr -> children -> content;
							if (colNum > maxColNum)
								maxColNum = colNum;

							values[colNum] = (char *)malloc (strlen (value) + 1);
							if (values[colNum] != NULL)
								strcpy (values[colNum], value);
						}
					}
				}
			}
		}
		processElementNames (doc, curNode->children, fullPath, readLevel);
		readLevel = saveLevel;

printf ("%d %d %d\n", saveLevel, startLevel, levels);
		if (saveLevel == startLevel)
		{
			for (i = 0; i <= maxColumn; ++i)
			{
				if (values[i] != NULL)
				{
					printf ("%s,", values[i]);
					free (values[i]);
					values[i] = NULL;
				}
				else
					printf (",");
			}
			printf ("\n");
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M Y  E R R O R  F U N C                                                                                           *
 *  =======================                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Callback used by the libxml2 to display messages.
 *  \param ctx File handle (not used).
 *  \param msg Error message.
 *  \param ... Extra parameters.
 *  \result None.
 */
void myErrorFunc (void *ctx, const char *msg, ...)
{
	if (displayDebug)
	{
		va_list arg_ptr;

		va_start (arg_ptr, msg);
//		displayVInColumn (COL_ERROR, (char *)msg, arg_ptr);
		va_end (arg_ptr);
	}
	else if (!shownError)
	{
		shownError = true;
//		displayInColumn (COL_ERROR, "Parsing file failed");
//		displayNewLine(0);
	}
	return;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  F I L E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the down loaded buffer.
 *  \param xmlFile File to parse.
 *  \result 1 if the file was processed.
 */
int processFile (char *xmlFile)
{
	int retn = 0;
	xmlDocPtr doc = NULL;
	xmlNodePtr	 rootElement = NULL;

	xmlSetGenericErrorFunc (NULL, myErrorFunc);
	if ((doc =	xmlParseFile (xmlFile)) != NULL)
	{
		if ((rootElement = xmlDocGetRootElement (doc)) != NULL)
		{
			processElementNames (doc, rootElement, "", 0);
			retn = 1;
		}
		xmlFreeDoc(doc);
	}
	xmlCleanupParser();
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  H T M L F I L E                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process an HTML file.
 *  \param htmlFile File to open.
 *  \result 1 if the file was processed.
 */
int processHTMLFile (char *htmlFile)
{
	int retn = 0;
	htmlDocPtr doc = NULL;
	htmlNodePtr rootElement = NULL;

	xmlSetGenericErrorFunc (NULL, myErrorFunc);
	if ((doc =	htmlParseFile (htmlFile, NULL)) != NULL)
	{
		if ((rootElement = xmlDocGetRootElement (doc)) != NULL)
		{
			processElementNames (doc, rootElement, "", 0);
			retn = 1;
		}
		xmlFreeDoc(doc);
	}
	xmlCleanupParser();
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  D I R                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Hex dump the file.
 *  \param file File to dump.
 *  \result 1 if all OK.
 */
int showDir (DIR_ENTRY *file)
{
	int procRetn = 0, useType = fileType, i;
	char inFile[PATH_SIZE];

	/*------------------------------------------------------------------------*
     * Open the file and display a table containing the hex dump.             *
     *------------------------------------------------------------------------*/
	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	if (useType == FILE_UNKNOWN)
	{
		useType = testFile (inFile) ? FILE_HTML : FILE_XML;
	}
	shownError = false;
	if (useType == FILE_HTML)
	{
		procRetn = processHTMLFile (inFile);
	}
	else
	{
		procRetn = processFile (inFile);
		for (i = 0; i < 1024; ++i)
		{
			if (columnNames[i] != NULL)
			{
				printf ("%s,", columnNames[i]);
				free (columnNames[i]);
			}
			else
				break;
		}
		printf ("\n");
	}
	if (procRetn)
	{
		++filesFound;
	}
	return 1;
}

#define READSIZE 4096


