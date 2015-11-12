/**********************************************************************************************************************
 *                                                                                                                    *
 *  H E X  D U M P . C                                                                                                *
 *  ==================                                                                                                *
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
 *  \brief Program to dump the contence of a file in Hex.
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

/*----------------------------------------------------------------------------*
 * Prototypes															      *
 *----------------------------------------------------------------------------*/
int fileCompare (DIR_ENTRY *fileOne, DIR_ENTRY *fileTwo);
int showDir (DIR_ENTRY *file);

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 * 00 00 00 00 00 00 00 00-08 13 50 00 00 00 00 00 : ..........P.....         *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colDumpDescs[38] =
{
	{	40,	5,	0,	2,	0x07,	0,	"Name",		1	},	/* 0 */
	{	40,	5,	0,	0,	0x07,	0,	"key",		2	},	/* 1 */
};

COLUMN_DESC *ptrDumpColumn[2] =
{
	&colDumpDescs[0],  &colDumpDescs[1]
};

COLUMN_DESC fileDescs[38] =
{
	{	60,	8,	16,	2,	0x07,	0,	"Filename",	1	},	/* 0 */
	{	20,	4,	4,	0,	0x07,	0,	"Size",		0	},	/* 1 */
};

COLUMN_DESC *ptrFileColumn[2] =
{
	&fileDescs[0],  &fileDescs[1]
};

int filesFound = 0;
int displayColour = 0;
int displayQuiet = 0;
int displayWidth = -1;

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
	printf ("Enter the command: %s [-Cqw <size>] <filename>\n", basename (progName));
	printf ("    -C . . . . . Display output in colour.\n");
	printf ("    -q . . . . . Quite mode, only dump the hex codes.\n");
	printf ("    -w <size>  . Set the display size (8, 16, 24 or 32).\n");
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
	int i, found = 0, width = 80;

	displayGetWindowSize ();

	width = displayGetWidth();

	while ((i = getopt(argc, argv, "Cqw:?")) != -1)
	{
		switch (i) 
		{
		case '?':
			helpThem (argv[0]);
			exit (1);
		}
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
 *  P R O C E S S  E L E M E N T  N A M E S                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Process each of the elements in the file.
 *  @param doc Document to read.
 *  @param aNode Current node.
 *  @param readLevel 0 current, 1 today, 2 tomorrow.
 *  @result None.
 */
static void
processElementNames (xmlDoc *doc, xmlNode * aNode, int readLevel)
{
	xmlChar *key;
    xmlNode *curNode = NULL;

    for (curNode = aNode; curNode; curNode = curNode->next) 
    {
        if (curNode->type == XML_ELEMENT_NODE) 
        {
			if ((!xmlStrcmp (curNode -> name, (const xmlChar *)"sensors"))) 
			{
				++readLevel;
			}
			else
			{
				key = xmlNodeListGetString (doc, curNode -> xmlChildrenNode, 1);
//				printf ("Level: %d, Name: %s, Key: %s\n", readLevel, (const char *)curNode -> name, (char *)key);
				displayInColumn (0, "%s", (char *)curNode -> name);
				displayInColumn (1, "%s", (char *)key);
				displayNewLine(0);

//				processWeatherKey (readLevel, (const char *)curNode -> name, (char *)key);
		    	xmlFree (key);
		    }
        }
        processElementNames (doc, curNode->children, readLevel);
    }
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  B U F F E R                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Process the down loaded buffer.
 *  @param buffer Buffer to process.
 *  @param size Size of the buffer.
 *  @result None.
 */
static void processFile (char *xmlFile)
{
	xmlDoc *doc = NULL;
	xmlNode *rootElement = NULL;

	if ((doc = xmlParseFile (xmlFile)) != NULL)
	{
		if ((rootElement = xmlDocGetRootElement (doc)) != NULL)
		{
			processElementNames (doc, rootElement, 0);
		}
        xmlFreeDoc(doc);
	}
	xmlCleanupParser();
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
	unsigned char inFile[PATH_SIZE];

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
	strcpy ((char *)inFile, file -> fullPath);
	strcat ((char *)inFile, file -> fileName);

	if (!displayColumnInit (2, ptrDumpColumn, displayColour))
	{
		fprintf (stderr, "ERROR in: displayColumnInit\n");
		return 0;
	}

	if (!displayQuiet) displayDrawLine (0);
	if (!displayQuiet) displayHeading (0);

	processFile (inFile);
	++filesFound;

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
