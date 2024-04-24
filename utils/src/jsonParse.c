/**********************************************************************************************************************
 *                                                                                                                    *
 *  J S O N  P A R S E . C                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File jsonParse.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms   *
 *  of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License,  *
 *  or (at your option) any later version.                                                                            *
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
 *  \brief Program to parse an JSON files.
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
#include <math.h>
#ifdef HAVE_VALUES_H
#include <values.h>
#else
#define MAXINT 2147483647
#endif

#include <dircmd.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>
#include "buildDate.h"

/*----------------------------------------------------------------------------*
 * Prototypes                                                                 *
 *----------------------------------------------------------------------------*/
int showDir (DIR_ENTRY *file);
void processStdin (void);

struct levelInfo
{
	int level;
	char objName[1024];
	char pathName[1024];
};

#define FILE_UNKNOWN	0
#define FILE_JSON		1

#define COL_DEPTH		0
#define COL_NAME		1
#define COL_VALUE		2
#define COL_ERROR		3
#define COL_COUNT		4

#define DISP_DEPTH		0x0001
#define DISP_NAME		0x0002
#define DISP_VALUE		0x0004
#define DISP_ERROR		0x0008
#define DISP_ALL		0x000F

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 * 00 00 00 00 00 00 00 00-08 13 50 00 00 00 00 00 : ..........P.....         *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colParseDescs[6] =
{
	{	20,		5,	0,	2,	0x01,	0,	"Depth",	0	},	/* 0 */
	{	255,	5,	0,	2,	0x02,	0,	"Name",		1	},	/* 1 */
	{	255,	5,	0,	2,	0x04,	0,	"Value",	2	},	/* 2 */
	{	255,	5,	0,	2,	0x04,	0,	"Error",	3	},	/* 2 */
};

COLUMN_DESC *ptrParseColumn[6] =
{
	&colParseDescs[0],	&colParseDescs[1],	&colParseDescs[2],	&colParseDescs[3]
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
bool displayPaths = false;
int displayCols = DISP_ALL;
int levels = 0;
char *levelName[20];
char jsonMatchPath[PATH_SIZE];

void jsonObjectForEachFunc(JsonObject *object, const gchar *member_name, JsonNode *member_node, gpointer user_data);

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
	printf ("TheKnight: JSON Parse a File, Version: %s, Built: %s\n", VERSION, buildDate);
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
	printf ("    -C . . . . . . . Display output in colour.\n");
	printf ("    -D[dnv]  . . . . Toggle display columns.\n");
	printf ("    -q . . . . . . . Quite mode, output name=key pairs.\n");
	printf ("    -P . . . . . . . Output the full path.\n");
	printf ("    -p <path>  . . . Path to search (eg: /sensors/light).\n");
	displayLine ();
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

	while ((i = getopt(argc, argv, "CqPp:D:?")) != -1)
	{
		switch (i)
		{
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
				case 'v':
					displayCols ^= DISP_VALUE;
					break;
				}
				++j;
			}
			break;

		case 'q':
			displayOptions ^= DISPLAY_HEADINGS;
			displayQuiet = !displayQuiet;
			break;

		case 'P':
			displayPaths = !displayPaths;
			break;

		case 'p':
			strncpy (jsonMatchPath, optarg, 1020);
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
char *rmWhiteSpace (const char *inBuff, char *outBuff, int maxLen)
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

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  D E P T H                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the depth marker.
 *  \param depth Current depth.
 *  \param code Marker to show.
 *  \result None.
 */
void displayDepth (int depth, char code)
{
	if (!displayQuiet && displayCols & DISP_DEPTH)
	{
		int i;
		char tempBuff[1025];

		for (i = 0; i < depth - 1 && i < 1020; ++i)
		{
			tempBuff[i] = ' ';
		}
		tempBuff[i] = code;
		tempBuff[i + 1] = 0;
		displayInColumn (COL_DEPTH, tempBuff);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  V A L U E                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the contents of a value.
 *  \param levelInfo Information about the current level.
 *  \param value Value to display.
 *  \result None.
 */
void displayValue (struct levelInfo *levelInfo, GValue *value)
{
	char *pathPtr = displayPaths ? levelInfo -> pathName : levelInfo -> objName;

	displayDepth (levelInfo -> level, '-');
	if (!displayQuiet && displayCols & DISP_NAME)
	{
		displayInColumn (COL_NAME, "%s", pathPtr);
	}
	if (G_VALUE_HOLDS (value, G_TYPE_STRING))
	{
		char tempBuff[1025];
		rmWhiteSpace (g_value_get_string (value), tempBuff, 1024);
		if (displayQuiet)
		{
			g_print ("%s=\"%s\"\n", pathPtr, tempBuff);
		}
		else
		{
			if (displayCols & DISP_VALUE)
			{
				displayInColumn (COL_VALUE, "%s", tempBuff);
			}
		}
	}
	else if (G_VALUE_HOLDS (value, G_TYPE_BOOLEAN))
	{
		if (displayQuiet)
		{
			g_print ("%s=%s\n", pathPtr, g_value_get_boolean (value) ? "true" : "false");
		}
		else
		{
			if (displayCols & DISP_VALUE)
			{
				displayInColumn (COL_VALUE, "%s", g_value_get_boolean (value) ? "true" : "false");
			}
		}
	}
	else
	{
		GValue number = G_VALUE_INIT;
		g_value_init (&number, G_TYPE_DOUBLE);
		if (g_value_transform (value, &number))
		{
			double num = g_value_get_double (&number);
			if (displayQuiet)
			{
				g_print ((ceilf (num) == num ? "%s=%0.0f\n" : "%s=%0.2f\n"), pathPtr, num);
			}
			else
			{
				if (displayCols & DISP_VALUE)
				{
					displayInColumn (COL_VALUE, (ceilf (num) == num ? "%0.0f" : "%0.2f"), num);
				}
			}
		}
		else
		{
			if (!displayQuiet)
			{
				displayInColumn (COL_VALUE, "Unknown type");
			}
		}
		g_value_unset (&number);
	}
	if (!displayQuiet)
	{
		displayNewLine(0);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  J S O N  A R R A Y  F O R  E A C H  F U N C                                                                       *
 *  ===========================================                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the elements in an array.
 *  \param array Array we are processing.
 *  \param index_ Index of this element.
 *  \param element_node The elements node.
 *  \param user_data User data (at the moment this is level).
 *  \result None.
 */
void jsonArrayForEachFunc (JsonArray *array, guint index_, JsonNode *element_node, gpointer user_data)
{
	int match = 0;
	char tempBuff[81];
	struct levelInfo *inLevelInfo = (struct levelInfo *)user_data;
	struct levelInfo outLevelInfo;

	sprintf (tempBuff, "[%d]", index_);
	outLevelInfo.level = inLevelInfo -> level;
	strcpy (outLevelInfo.pathName, inLevelInfo -> pathName);
	strcat (outLevelInfo.pathName, tempBuff);
	strcpy (outLevelInfo.objName, inLevelInfo -> objName);
	strcat (outLevelInfo.objName, tempBuff);

	if (jsonMatchPath[0])
	{
		match = strncmp (outLevelInfo.pathName, jsonMatchPath, strlen (jsonMatchPath));
	}

	if (element_node)
	{
		if (json_node_get_node_type (element_node) == JSON_NODE_OBJECT)
		{
			JsonObject *objectInner = json_node_get_object(element_node);
			if (objectInner != NULL)
			{
				if (match == 0 && !displayQuiet && displayCols & DISP_NAME)
				{
					displayDepth (outLevelInfo.level, '+');
					displayInColumn (COL_NAME, "%s", (displayPaths ? outLevelInfo.pathName : outLevelInfo.objName));
					displayNewLine(0);
				}
				json_object_foreach_member (objectInner, jsonObjectForEachFunc, (gpointer)&outLevelInfo);
			}
		}
		else if (json_node_get_node_type (element_node) == JSON_NODE_VALUE)
		{
			GValue value = G_VALUE_INIT;
			json_node_get_value (element_node, &value);
			if (match == 0)
			{
				displayValue (&outLevelInfo, &value);
			}
			g_value_unset (&value);
		}
		else if (json_node_get_node_type (element_node) == JSON_NODE_ARRAY)
		{
			JsonArray *array = json_node_get_array(element_node);
			if (array != NULL)
			{
				json_array_foreach_element (array, jsonArrayForEachFunc, (gpointer)&outLevelInfo);
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  J S O N  O B J E C T  F O R  E A C H  F U N C                                                                     *
 *  =============================================                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the objects in a node.
 *  \param object Current object.
 *  \param member_name Name of the object.
 *  \param member_node The node of the object.
 *  \param user_data User data (at the moment this is level).
 *  \result None.
 */
void jsonObjectForEachFunc(JsonObject *object, const gchar *member_name, JsonNode *member_node, gpointer user_data)
{
	int match = 0;
	struct levelInfo *inLevelInfo = (struct levelInfo *)user_data;
	struct levelInfo outLevelInfo;

	outLevelInfo.level = inLevelInfo -> level + 1;
	strcpy (outLevelInfo.pathName, inLevelInfo -> pathName);
	if (inLevelInfo -> level) strcat (outLevelInfo.pathName, "/");
	strcat (outLevelInfo.pathName, member_name);
	strcpy (outLevelInfo.objName, member_name);

	if (jsonMatchPath[0])
	{
		match = strncmp (outLevelInfo.pathName, jsonMatchPath, strlen (jsonMatchPath));
	}

	if (member_node)
	{
		if (json_node_get_node_type (member_node) == JSON_NODE_OBJECT)
		{
			JsonObject *objectInner = json_node_get_object(member_node);
			if (objectInner != NULL)
			{
				if (match == 0 && !displayQuiet && displayCols & DISP_NAME)
				{
					displayDepth (outLevelInfo.level, '+');
					displayInColumn (COL_NAME, "%s", (displayPaths ? outLevelInfo.pathName : outLevelInfo.objName));
					displayNewLine(0);
				}
				json_object_foreach_member (objectInner, jsonObjectForEachFunc, (gpointer)&outLevelInfo);
			}
		}
		else if (json_node_get_node_type (member_node) == JSON_NODE_VALUE)
		{
			GValue value = G_VALUE_INIT;
			json_node_get_value (member_node, &value);
			if (match == 0)
			{
				displayValue (&outLevelInfo, &value);
			}
			g_value_unset (&value);
		}
		else if (json_node_get_node_type (member_node) == JSON_NODE_ARRAY)
		{
			JsonArray *array = json_node_get_array(member_node);
			if (array != NULL)
			{
				json_array_foreach_element (array, jsonArrayForEachFunc, (gpointer)&outLevelInfo);
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  F I L E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the down loaded buffer.
 *  \param jsonFile File to parse.
 *  \result 1 if the file was processed.
 */
int processFile (char *jsonFile)
{
	int retn = 0;
	JsonParser *parser = NULL;
	JsonNode *root = NULL;
	GError *error = NULL;
	struct levelInfo outLevelInfo;

	outLevelInfo.level = 0;
	outLevelInfo.objName[0] = 0;
	strcpy (outLevelInfo.pathName, "/");

	parser = json_parser_new ();
	if (parser != NULL)
	{
		json_parser_load_from_file (parser, jsonFile, &error);
		if (error)
		{
			displayInColumn (COL_ERROR, "%s", error -> message);
			displayNewLine(0);
			g_error_free (error);
			g_object_unref (parser);
			return retn;
		}

		root = json_parser_get_root (parser);
		if (root != NULL)
		{
			if (json_node_get_node_type (root) == JSON_NODE_OBJECT)
			{
				JsonObject *object = json_node_get_object(root);
				if (object != NULL)
				{
					json_object_foreach_member (object, jsonObjectForEachFunc, (gpointer)&outLevelInfo);
					retn = 1;
				}
			}
			else if (json_node_get_node_type (root) == JSON_NODE_ARRAY)
			{
				JsonArray *array = json_node_get_array(root);
				if (array != NULL)
				{
					json_array_foreach_element (array, jsonArrayForEachFunc, (gpointer)&outLevelInfo);
					retn = 1;
				}
			}
		}
		g_object_unref (parser);
	}
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
	int procRetn = 0;
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
	if (!displayQuiet)
	{
		char tempBuff[121];

		displayInColumn (0, "JSON");
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
	procRetn = processFile (inFile);

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
	JsonParser *parser;
	JsonNode *root;
	GError *error;
	struct levelInfo outLevelInfo;
	int readSize, buffSize = 0, totalRead = 0;
	char *buffer;

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
			displayInColumn (0, "JSON");
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
			outLevelInfo.level = 0;
			outLevelInfo.objName[0] = 0;
			strcpy (outLevelInfo.pathName, "/");

			parser = json_parser_new ();

			error = NULL;
			json_parser_load_from_data (parser, buffer, totalRead, &error);
			if (error)
			{
				displayInColumn (COL_ERROR, "%s", error -> message);
				displayNewLine(0);
				g_error_free (error);
				g_object_unref (parser);
			}
			else
			{
				root = json_parser_get_root (parser);
				if (root != NULL)
				{
					if (json_node_get_node_type (root) == JSON_NODE_OBJECT)
					{
						JsonObject *object = json_node_get_object(root);
						if (object != NULL)
						{
							json_object_foreach_member (object, jsonObjectForEachFunc, (gpointer)&outLevelInfo);
						}
					}
					else if (json_node_get_node_type (root) == JSON_NODE_ARRAY)
					{
						JsonArray *array = json_node_get_array(root);
						if (array != NULL)
						{
							json_array_foreach_element (array, jsonArrayForEachFunc, (gpointer)&outLevelInfo);
						}
					}
				}
				g_object_unref (parser);

				if (!displayQuiet)
				{
					displayDrawLine (0);
				}
			}
			displayAllLines ();
			displayTidy ();
		}
	}
}

