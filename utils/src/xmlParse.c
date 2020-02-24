/**********************************************************************************************************************
 *                                                                                                                    *
 *  X M L  P A R S E . C                                                                                              *
 *  ====================                                                                                              *
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
void processStdin (void);

#define FILE_UNKNOWN	0
#define FILE_XML		1
#define FILE_HTML		2

#define COL_DEPTH		0
#define COL_NAME		1
#define COL_ATTR		2
#define COL_VALUE		3
#define COL_KEY			4
#define COL_ERROR		5
#define COL_COUNT		6

#define DISP_DEPTH		0x0001
#define DISP_NAME		0x0002
#define DISP_ATTR		0x0004
#define DISP_VALUE		0x0008
#define DISP_KEY		0x0010
#define DISP_ERROR		0x0020
#define DISP_ALL		0x003F

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 * 00 00 00 00 00 00 00 00-08 13 50 00 00 00 00 00 : ..........P.....         *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colParseDescs[6] =
{
	{	20,		5,	0,	2,	0x01,	0,	"Depth",		0	},	/* 0 */
	{	255,	5,	0,	2,	0x02,	0,	"Name",			2	},	/* 1 */
	{	40,		5,	0,	2,	0x04,	0,	"Attribute",	3	},	/* 2 */
	{	255,	5,	0,	2,	0x04,	0,	"Value",		4	},	/* 3 */
	{	255,	5,	0,	0,	0x06,	0,	"Key",			2	},	/* 4 */
	{	255,	5,	0,	0,	0x01,	0,	"Error",		1	},	/* 5 */
};

COLUMN_DESC *ptrParseColumn[6] =
{
	&colParseDescs[0],	&colParseDescs[1],	&colParseDescs[2],	&colParseDescs[3],	&colParseDescs[4],	&colParseDescs[5]
};

COLUMN_DESC fileDescs[4] =
{
	{	5,		5,	0,	2,	0x02,	0,	"Type",		1	},	/* 1 */
	{	255,	8,	0,	2,	0x07,	0,	"Filename", 2	},	/* 2 */
	{	20,		4,	0,	2,	0x07,	0,	"Size",		3	},	/* 3 */
	{	81,		12, 0,	0,	0x02,	0,	"Modified", 4	},	/* 4 */
};

COLUMN_DESC *ptrFileColumn[4] =
{
	&fileDescs[0],	&fileDescs[1],	&fileDescs[2],	&fileDescs[3]
};

int filesFound = 0;
int fileType = FILE_UNKNOWN;
int displayOptions = DISPLAY_HEADINGS | DISPLAY_HEADINGS_NB;
bool displayQuiet = false;
bool displayDebug = false;
bool displayPaths = false;
int displayCols = DISP_ALL;
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
	printf ("    -C . . . . . . . Display output in colour.\n");
	printf ("    -D[dnavke] . . . Toggle display columns.\n");
	printf ("    -d . . . . . . . Output parser debug messages.\n");
	printf ("    -q . . . . . . . Quite mode, output name=key pairs.\n");
	printf ("    -P . . . . . . . Output the full path.\n");
	printf ("    -p <path>  . . . Path to search (eg: /sensors/light).\n");
	printf ("    -s <xsd> . . . . Path to xsd schema validation file.\n");
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
	int i, j, found = 0;
	char fullVersion[81];

	strcpy (fullVersion, VERSION);
#ifdef USE_STATX
	strcat (fullVersion, ".X");
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

		case 'C':
			displayOptions ^= DISPLAY_COLOURS;
			break;

		case 'D':
			j = 0;
			while (optarg[j])
			{
				switch (optarg[j])
				{
				case 'd':
					displayCols ^= DISP_DEPTH;
					break;
				case 'n':
					displayCols ^= DISP_NAME;
					break;
				case 'a':
					displayCols ^= DISP_ATTR;
					break;
				case 'v':
					displayCols ^= DISP_VALUE;
					break;
				case 'k':
					displayCols ^= DISP_KEY;
					break;
				case 'e':
					displayCols ^= DISP_ERROR;
					break;
				}
				++j;
			}
			break;

		case 'd':
			displayDebug = !displayDebug;
			break;

		case 'q':
			displayOptions ^= DISPLAY_HEADINGS;
			displayQuiet = !displayQuiet;
			break;

		case 'P':
			displayPaths = !displayPaths;
			break;

		case 'p':
			parsePath (optarg);
			break;

		case 's':
			if (strlen (optarg) > PATH_SIZE)
			{
				optarg[PATH_SIZE] = 0;
			}
			strcpy (xsdPath, optarg);
			break;

		case '?':
			helpThem (argv[0]);
			exit (1);
		}
	}

	if (optind == argc)
	{
		processStdin ();
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

		if (!displayQuiet)
		{
			if (!displayColumnInit (3, ptrFileColumn, displayOptions & ~DISPLAY_HEADINGS))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 0;
			}
			displayDrawLine (0);
			displayInColumn (1, "%d %s shown\n", filesFound, filesFound == 1 ? "File" : "Files");
			displayNewLine(DISPLAY_INFO);
			displayAllLines ();
		}
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
	int i;

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

				key = xmlNodeListGetString (doc, curNode -> xmlChildrenNode, 1);
				if (displayQuiet)
				{
					rmWhiteSpace ((char *)key, tempBuff, 1020);
					if (tempBuff[0])
					{
						printf ("%s=\"%s\"\n",
								(displayPaths ? fullPath : (char *)curNode -> name),
								key == NULL ? "(null)" : tempBuff);
					}
				}
				else
				{
					bool shown = false;
					if (displayCols & DISP_DEPTH)
					{
						int count = xmlChildElementCount (curNode);
						for (i = 0; i < readLevel - 1 && i < 1020; ++i)
						{
							tempBuff[i] = ' ';
						}
						tempBuff[i] = count ? '+' : '-';
						tempBuff[i + 1] = 0;
						displayInColumn (COL_DEPTH, tempBuff);
						shown = true;
					}
					if (displayCols & DISP_NAME)
					{
						displayInColumn (COL_NAME, "%s", (displayPaths ? fullPath : (char *)curNode -> name));
						shown = true;
					}
					if (key != NULL && displayCols & DISP_KEY)
					{
						rmWhiteSpace ((char *)key, tempBuff, 1020);
						if (tempBuff[0])
						{
							displayInColumn (COL_KEY, "%s", key == NULL ? "(null)" : tempBuff);
							shown = true;
						}
					}
					for (attr = curNode -> properties; attr != NULL; attr = attr -> next)
					{
						bool shownP = false;
						if (displayCols & DISP_ATTR)
						{
							displayInColumn (COL_ATTR, "%s", (char *)attr -> name);
							shownP = true;
						}
						if (displayCols & DISP_VALUE)
						{
							if (attr -> children)
							{
								displayInColumn (COL_VALUE, "%s", (char *)attr -> children -> content);
								shownP = true;
							}
						}
						if (shownP)
						{
							displayNewLine (0);
						}
					}
					if (shown)
					{
						displayNewLine (0);
					}
				}
				xmlFree (key);
			}
		}
		processElementNames (doc, curNode->children, fullPath, readLevel);
		readLevel = saveLevel;
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
		if (displayCols & DISP_ERROR)
		{
			int i = 0, delCR = 0;
			char tmpErrBuff[4100] = "";

			va_list arg_ptr;
			va_start (arg_ptr, msg);
			vsnprintf (tmpErrBuff, 4096, msg, arg_ptr);
			va_end (arg_ptr);
			tmpErrBuff[4096] = 0;

			while (tmpErrBuff[i] != 0)
			{
				if (tmpErrBuff[i] == '\n')
				{
					tmpErrBuff[i] = ' ';
					delCR = 1;
				}
				++i;
			}
			displayInColumn (COL_ERROR, tmpErrBuff);
			if (delCR)
				displayNewLine (0);
		}
	}
	else if (!shownError)
	{
		shownError = true;
		if (displayCols & DISP_ERROR)
		{
			displayInColumn (COL_ERROR, "Parsing file failed");
			displayNewLine(0);
		}
	}
	return;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V A L I D A T E  D O C U M E N T                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Use any given xsd file to validate the document.
 *  \param doc Document to validate.
 *  \result Error code (0 if documant is good).
 */
int validateDocument (xmlDocPtr doc)
{
	xmlSchemaParserCtxtPtr parserCtxt = NULL;
	xmlSchemaValidCtxtPtr validCtxt = NULL;
	xmlSchemaPtr schema = NULL;

	xmlSetStructuredErrorFunc (NULL, NULL);
	xmlSetGenericErrorFunc (NULL, myErrorFunc);
	xmlThrDefSetStructuredErrorFunc (NULL, NULL);
	xmlThrDefSetGenericErrorFunc (NULL, myErrorFunc);

	parserCtxt = xmlSchemaNewParserCtxt (xsdPath);

	if (parserCtxt != NULL)
	{
		schema = xmlSchemaParse (parserCtxt);

		if (schema != NULL)
		{
			validCtxt = xmlSchemaNewValidCtxt (schema);

			if (validCtxt)
			{

				return xmlSchemaValidateDoc (validCtxt, doc);
			}
		}
	}
	return -1;
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
	int retn = 0, notValid = 0;
	xmlDocPtr doc = NULL;
	xmlNodePtr	 rootElement = NULL;

	xmlSetGenericErrorFunc (NULL, myErrorFunc);
	if ((doc =	xmlParseFile (xmlFile)) != NULL)
	{
		if (xsdPath[0])
		{
			notValid = validateDocument (doc);
		}
		if (!notValid)
		{
			if ((rootElement = xmlDocGetRootElement (doc)) != NULL)
			{
				processElementNames (doc, rootElement, "", 0);
				retn = 1;
			}
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
	int procRetn = 0, useType = fileType;
	char inFile[PATH_SIZE];

	/*------------------------------------------------------------------------*
     * First display a table with the file name and size.                     *
     *------------------------------------------------------------------------*/
	if (!displayColumnInit (4, ptrFileColumn, displayOptions))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}

	/*------------------------------------------------------------------------*
     * Open the file and display a table containing the hex dump.             *
     *------------------------------------------------------------------------*/
	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);
	if (useType == FILE_UNKNOWN)
	{
		useType = testFile (inFile) ? FILE_HTML : FILE_XML;
	}
	if (!displayQuiet)
	{
		char tempBuff[121];

		displayInColumn (0, "%s", (useType == FILE_HTML ? "HTML" : "XML"));
		displayInColumn (1, "%s", file -> fileName);
#ifdef USE_STATX
		displayInColumn (2, displayFileSize (file -> fileStat.stx_size, tempBuff));
		displayInColumn (3, displayDateString (file -> fileStat.stx_mtime.tv_sec, tempBuff));
#else
		displayInColumn (2, displayFileSize (file -> fileStat.st_size, tempBuff));
		displayInColumn (3, displayDateString (file -> fileStat.st_mtime, tempBuff));
#endif
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();
	}
	displayTidy ();

	if (!displayColumnInit (COL_COUNT, ptrParseColumn, displayOptions))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}
	shownError = false;
	if (useType == FILE_HTML)
	{
		procRetn = processHTMLFile (inFile);
	}
	else
	{
		procRetn = processFile (inFile);
	}
	if (procRetn)
	{
		++filesFound;
	}
	displayAllLines ();
	displayTidy ();
	return 1;
}

#define READSIZE 4096

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  S T D I N                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process input from stdin.
 *  \result None.
 */
void processStdin ()
{
	int readSize, buffSize = 0, totalRead = 0, notValid = 0;
	char *buffer;
	xmlDocPtr xDoc = NULL;
	xmlNodePtr rootElement = NULL;
	htmlDocPtr hDoc = NULL;
	xmlChar *xmlBuffer = NULL;

	buffer = (char *)malloc(buffSize = READSIZE);
	do
	{
		if ((readSize = fread (&buffer[buffSize - READSIZE], 1, READSIZE, stdin)) > 0)
		{
			totalRead += readSize;
			if (readSize == READSIZE)
			{
				buffSize += READSIZE;
				buffer = realloc (buffer, buffSize);
			}
		}
	}
	while (buffer && readSize == READSIZE);

	if (buffer && totalRead)
	{
		/*----------------------------------------------------------------------------*
         * Work out the type of input by looking for HTML strings.                    *
         *----------------------------------------------------------------------------*/
		if (fileType == FILE_UNKNOWN)
		{
			if (searchStr ("<!DOCTYPE html", buffer, totalRead) != NULL)
			{
				fileType = FILE_HTML;
			}
			else if (searchStr ("<html", buffer, totalRead) != NULL)
			{
				fileType = FILE_HTML;
			}
			else
			{
				fileType = FILE_XML;
			}
		}

		if (!displayQuiet)
		{
			/*------------------------------------------------------------------------*
             * Display a table with the type, like this was a normal file.            *
             *------------------------------------------------------------------------*/
			if (!displayColumnInit (2, ptrFileColumn, displayOptions))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return;
			}
			displayInColumn (0, "%s", (fileType == FILE_HTML ? "HTML" : "XML"));
			displayInColumn (1, "stdin");
			displayNewLine (DISPLAY_INFO);
			displayAllLines ();
			displayTidy ();
		}

		/*----------------------------------------------------------------------------*
         * Display the formatted output.                                              *
         *----------------------------------------------------------------------------*/
		if (displayColumnInit (COL_COUNT, ptrParseColumn, displayOptions))
		{
			shownError = false;
			xmlSetGenericErrorFunc (NULL, myErrorFunc);
			if ((xmlBuffer = xmlCharStrndup(buffer, totalRead)) != NULL)
			{
				if (fileType == FILE_HTML)
				{
					if ((hDoc = htmlParseDoc(xmlBuffer, NULL)) != NULL)
					{
						if ((rootElement = xmlDocGetRootElement (hDoc)) != NULL)
						{
							processElementNames (hDoc, rootElement, "", 0);
						}
						xmlFreeDoc(hDoc);
					}
				}
				else
				{
					if ((xDoc = xmlParseDoc(xmlBuffer)) != NULL)
					{
						if (xsdPath[0])
						{
							notValid = validateDocument (xDoc);
						}
						if (!notValid)
						{
							if ((rootElement = xmlDocGetRootElement (xDoc)) != NULL)
							{
								processElementNames (xDoc, rootElement, "", 0);
							}
						}
						xmlFreeDoc(xDoc);
					}
				}
				xmlFree (xmlBuffer);
			}
			if (!displayQuiet)
			{
				displayDrawLine (0);
			}
			xmlCleanupParser();
			displayAllLines ();
			displayTidy ();
		}
	}
}

