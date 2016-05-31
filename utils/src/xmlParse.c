/**********************************************************************************************************************
 *                                                                                                                    *
 *  X M L  P A R S E . C                                                                                              *
 *  ====================                                                                                              *
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
 *  \brief Program to parse an XML file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dircmd.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/xmlschemas.h>

/*----------------------------------------------------------------------------*
 * Prototypes															      *
 *----------------------------------------------------------------------------*/
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
int showDir (DIR_ENTRY *file);
void parsePath (char *path);
void processStdin (void);

#define COL_DEPTH	0
#define COL_NAME	1
#define COL_ATTR	2
#define COL_VALUE	3
#define COL_KEY		4
#define COL_ERROR	5
#define COL_COUNT	6

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 * 00 00 00 00 00 00 00 00-08 13 50 00 00 00 00 00 : ..........P.....         *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colParseDescs[6] =
{
	{	20,		5,	0,	2,	0x01,	0,	"Depth",		0	},	/* 0 */
	{	255,	5,	0,	2,	0x02,	0,	"Name",			2	},	/* 1 */
	{	40, 	5,	0,	2, 	0x04,	0,	"Attribute",	3	},	/* 3 */
	{	255, 	5,	0,	2, 	0x04,	0,	"Value",		4	},	/* 3 */
	{	255,	5,	0,	0,	0x06,	0,	"Key",			2	},	/* 2 */
	{	255,	5,	0,	0,	0x01,	0,	"Error",		1	},	/* 4 */
};

COLUMN_DESC *ptrParseColumn[6] =
{
	&colParseDescs[0],  &colParseDescs[1],  &colParseDescs[2],  &colParseDescs[3],  &colParseDescs[4],  &colParseDescs[5]
};

COLUMN_DESC fileDescs[3] =
{
	{	255,	8,	0,	2,	0x07,	0,	"Filename",	1	},	/* 0 */
	{	20,		4,	0,	2,	0x07,	0,	"Size",		2	},	/* 1 */
	{	255,	12,	0,	0,	0x02,	0,	"Modified",	3	},	/* 2 */

};

COLUMN_DESC *ptrFileColumn[3] =
{
	&fileDescs[0],  &fileDescs[1],  &fileDescs[2]
};

int filesFound = 0;
int displayOptions = DISPLAY_HEADINGS | DISPLAY_HEADINGS_NB;
int displayQuiet = 0;
int displayDebug = 0;
int displayPaths = 0;
int levels = 0;
char *levelName[20];
char xsdPath[PATH_SIZE];
int shownError = 0;

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
	printf ("TheKnight: XML Parse a File, Version %s\n", directoryVersion());
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
	printf ("Enter the command: %s [-Cqp <path>] <filename>\n", basename (progName));
	printf ("    -C . . . . . Display output in colour.\n");
	printf ("    -d . . . . . Output parser debug messages.\n");
	printf ("    -q . . . . . Quite mode, output name=key pairs.\n");
	printf ("    -P . . . . . Output the full path.\n");
	printf ("    -p <path>  . Path to search (eg: /sensors/light).\n");
	printf ("    -s <xsd> . . Path to xsd schema validation file.\n");
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

	if (strcmp (directoryVersion(), VERSION) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
		exit (1);
	}

	displayGetWindowSize ();
	displayGetWidth();

	while ((i = getopt(argc, argv, "CdqPp:s:?")) != -1)
	{
		switch (i) 
		{
		case 'C':
			displayOptions ^= DISPLAY_COLOURS;
			break;
			
		case 'd':
			displayDebug ^= 1;
			break;
			
		case 'q':
			displayOptions ^= DISPLAY_HEADINGS;
			displayQuiet ^= 1;
			break;
			
		case 'P':
			displayPaths ^= 1;
			break;
			
		case 'p':
			parsePath (optarg);
			break;

		case 's':
			strncpy (xsdPath, optarg, PATH_SIZE);
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
		found += directoryLoad (argv[optind], ONLYFILES, fileCompare, &fileList);
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
			if (!displayColumnInit (2, ptrFileColumn, displayOptions & ~DISPLAY_HEADINGS))
			{
				fprintf (stderr, "ERROR in: displayColumnInit\n");
				return 0;
			}
			displayDrawLine (0);
			displayInColumn (0, "%d %s shown\n", filesFound, filesFound == 1 ? "File" : "Files");
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

		if (displayPaths)
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
					printf ("%s=%s\n", 
							(displayPaths ? fullPath : (char *)curNode -> name), 
							key == NULL ? "(null)" : rmWhiteSpace ((char *)key, tempBuff, 1020));
				}
				else
				{
					int count = 0;
					count = xmlChildElementCount (curNode);
					for (i = 0; i < readLevel - 1; ++i) tempBuff[i] = ' ';
					tempBuff[i] = count ? '+' : '-';
					tempBuff[i + 1] = 0;
					displayInColumn (COL_DEPTH, tempBuff);
					displayInColumn (COL_NAME, "%s", (displayPaths ? fullPath : (char *)curNode -> name));
					if (key != NULL)
					{
						displayInColumn (COL_KEY, "%s", key == NULL ? "(null)" : 
								rmWhiteSpace ((char *)key, tempBuff, 1020));
					}											
					for (attr = curNode -> properties; attr != NULL; attr = attr -> next)
					{
						displayInColumn (COL_ATTR, "%s", (char *)attr -> name);
						displayInColumn (COL_VALUE, "%s", (char *)attr -> children -> content);
						displayNewLine (0);
					}
					displayNewLine (0);
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
		va_list arg_ptr;

		va_start (arg_ptr, msg);
		displayVInColumn (COL_ERROR, (char *)msg, arg_ptr);
		va_end (arg_ptr);
//		displayNewLine(0);
	}
	else if (!shownError)
	{
		++shownError;
		displayInColumn (COL_ERROR, "Parsing file failed");
		displayNewLine(0);
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
 *  \result None.
 */
int processFile (char *xmlFile)
{
	int retn = 0, notValid = 0;
	xmlDoc *doc = NULL;
	xmlNode *rootElement = NULL;

	xmlSetGenericErrorFunc (NULL, myErrorFunc);
	if ((doc = 	xmlParseFile (xmlFile)) != NULL)
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
	char inFile[PATH_SIZE];

    /*------------------------------------------------------------------------*
     * First display a table with the file name and size.                     *
     *------------------------------------------------------------------------*/
	if (!displayColumnInit (3, ptrFileColumn, displayOptions))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}
	if (!displayQuiet) 
	{
		char tempBuff[121];

		displayInColumn (0, "%s", file -> fileName);
		displayInColumn (1,	displayFileSize (file -> fileStat.st_size, tempBuff));
		displayInColumn (2, displayDateString (file -> fileStat.st_mtime, tempBuff));
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();		
	}
	displayTidy ();

    /*------------------------------------------------------------------------*
     * Open the file and display a table containing the hex dump.             *
     *------------------------------------------------------------------------*/
	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);

	if (!displayColumnInit (COL_COUNT, ptrParseColumn, displayOptions))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}
	shownError = 0;
	if (processFile (inFile))
	{
		++filesFound;
	}
	displayAllLines ();
	displayTidy ();
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I L E  C O M P A R E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two files for sorting.
 *  \param fileOne First file.
 *  \param fileTwo Second file.
 *  \result 0, 1 or -1 depending on order.
 */
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo)
{
	if (fileOne -> fileStat.st_mode & S_IFDIR)
	{
		if (!(fileTwo -> fileStat.st_mode & S_IFDIR))
			return -1;
	}
	if (fileTwo -> fileStat.st_mode & S_IFDIR)
	{
		if (!(fileOne -> fileStat.st_mode & S_IFDIR))
			return 1;
	}
	return strcasecmp (fileOne -> fileName, fileTwo -> fileName);
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
	xmlDoc *doc = NULL;
	xmlNode *rootElement = NULL;
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
		if (displayColumnInit (COL_COUNT, ptrParseColumn, displayOptions))
		{
			shownError = 0;		
			xmlSetGenericErrorFunc (NULL, myErrorFunc);
			if ((xmlBuffer = xmlCharStrndup(buffer, totalRead)) != NULL)
			{
				if ((doc = xmlParseDoc(xmlBuffer)) != NULL)
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
						}
					}
					xmlFreeDoc(doc);
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

