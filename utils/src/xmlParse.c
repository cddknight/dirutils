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

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 * 00 00 00 00 00 00 00 00-08 13 50 00 00 00 00 00 : ..........P.....         *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colParseDescs[5] =
{
	{	10,		5,	5,	2,	0x07,	0,	"Depth",	0	},	/* 0 */
	{	40,		5,	5,	2,	0x07,	0,	"Name",		2	},	/* 1 */
	{	40, 	5,	0,	2, 	0x07,	0,	"Attr.",	3	},	/* 3 */
	{	40,		5,	0,	0,	0x07,	0,	"Key",		2	},	/* 2 */
	{	120,	5,	0,	0,	0x07,	0,	"Error",	1	},	/* 4 */
};

COLUMN_DESC *ptrParseColumn[5] =
{
	&colParseDescs[0],  &colParseDescs[1],  &colParseDescs[2],  &colParseDescs[3],  &colParseDescs[4]
};

COLUMN_DESC fileDescs[2] =
{
	{	120,	8,	16,	2,	0x07,	0,	"Filename",	1	},	/* 0 */
	{	20,		4,	4,	0,	0x07,	0,	"Size",		0	},	/* 1 */
};

COLUMN_DESC *ptrFileColumn[2] =
{
	&fileDescs[0],  &fileDescs[1]
};

int filesFound = 0;
int displayColour = 0;
int displayQuiet = 0;
int displayDebug = 0;
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

	displayGetWindowSize ();

	displayGetWidth();

	while ((i = getopt(argc, argv, "Cdqp:s:?")) != -1)
	{
		switch (i) 
		{
		case 'C':
			displayColour = DISPLAY_COLOURS;
			break;
			
		case 'd':
			displayDebug ^= 1;
			break;
			
		case 'q':
			displayQuiet ^= 1;
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
			if (!displayColumnInit (2, ptrFileColumn, displayColour))
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
	int lastChar = 0, i = 0, j = 0;
	
	while (inBuff[i] && j < maxLen)
	{
		if (inBuff[i] > ' ')
		{
			lastChar = i + 1;
		}
		if (lastChar)
		{
			outBuff[j] = inBuff[i];
			outBuff[++j] = 0;
		}
		++i;
	}
	outBuff[j] = 0;
	return outBuff;
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
 *  \param readLevel 0 current, 1 today, 2 tomorrow.
 *  \result None.
 */
static void
processElementNames (xmlDoc *doc, xmlNode * aNode, int readLevel)
{
	xmlChar *key;
    xmlNode *curNode = NULL;
    char tempBuff[81];
	int i;

    for (curNode = aNode; curNode; curNode = curNode->next) 
    {
       	int saveLevel = readLevel;

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
					printf ("%s=%s\n", (char *)curNode -> name, key == NULL ? "(null)" : 
							rmWhiteSpace ((char *)key, tempBuff, 80));
				}
				else
				{
					int count = xmlChildElementCount (curNode);

					for (i = 0; i < readLevel - 1; ++i) tempBuff[i] = ' ';
					tempBuff[i] = count ? '+' : '-';
					tempBuff[i + 1] = 0;
					displayInColumn (0, tempBuff);
					displayInColumn (1, "%s", (char *)curNode -> name);
					if (key != NULL)
					{
						displayInColumn (3, "%s", key == NULL ? "(null)" : 
								rmWhiteSpace ((char *)key, tempBuff, 80));
					}											
					for (attr = curNode -> properties; attr != NULL; attr = attr -> next)
					{
						displayInColumn (2, "%s = %s", (char *)attr -> name, (char *)attr -> children -> content);
						displayNewLine(0);
					}
					displayNewLine(0);
				}
		    	xmlFree (key);
		    }
        }
        processElementNames (doc, curNode->children, readLevel);
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
		displayVInColumn (4, (char *)msg, arg_ptr);
		va_end (arg_ptr);
//		displayNewLine(0);
	}
	else if (!shownError)
	{
		++shownError;
		displayInColumn (4, "Parsing file failed");
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
				processElementNames (doc, rootElement, 0);
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
	if (!displayColumnInit (2, ptrFileColumn, displayColour))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}
	if (!displayQuiet) 
	{
		char sizeBuff[41];

		displayDrawLine (0);
		displayHeading (0);
		displayNewLine (0);
		displayInColumn (0, "%s", file -> fileName);
		displayInColumn (1,	displayFileSize (file -> fileStat.st_size, sizeBuff));
		displayNewLine (DISPLAY_INFO);
		displayAllLines ();		
	}
	displayTidy ();

    /*------------------------------------------------------------------------*
     * Open the file and display a table containing the hex dump.             *
     *------------------------------------------------------------------------*/
	strcpy (inFile, file -> fullPath);
	strcat (inFile, file -> fileName);

	if (!displayColumnInit (5, ptrParseColumn, displayColour))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}
	if (!displayQuiet)
	{
		displayDrawLine (0);
		displayHeading (0);
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
		if (displayColumnInit (5, ptrParseColumn, displayColour))
		{
			if (!displayQuiet)
			{
				displayDrawLine (0);
				displayHeading (0);
			}
		
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
							processElementNames (doc, rootElement, 0);
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

