/**********************************************************************************************************************
 *                                                                                                                    *
 *  A D D  C O M M E N T . C                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File addComment.c part of DirCmdUtils is free software: you can redistribute it and/or modify it under the terms  *
 *  of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License,  *
 *  or (at your option) any later version.                                                                            *
 *                                                                                                                    *
 *  DirCmdUtils is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the         *
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for  *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see:           *
 *  <http://www.gnu.org/licenses/>                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Automatically add comments to C source file.
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
#include <readline/readline.h>
#include <readline/history.h>

/**********************************************************************************************************************
 *  Prototypes                                                                                                        *
 **********************************************************************************************************************/
int addComments (DIR_ENTRY * file);
int displayBox(int type, int count, FILE *outFile);
void commentWrite(char inChar, FILE *outFile);

#define BUFFER_SIZE 4096
#define MAX_BOXWIDTH 250
#define DEF_BOXWIDTH 116
#define SQ	0x27
#define DQ	0x22

/**********************************************************************************************************************
 *  Globals                                                                                                           *
 **********************************************************************************************************************/
int commentLine = 0;
int filesFound = 0;
int lineChar = '*';
int boxWidth = DEF_BOXWIDTH;
int tabSize = 8;
int debug = 0;
int cvsID = 0;
int cppStyle = 0;
int quietMode = 0;
int copyrightType = 0;
int autoMain = 0;
int boxCheck = 0;

static int addedComments = 0;
static char curFilename[81];
static char possibleName[30][81];
static char inBuffer[BUFFER_SIZE];
static char outBuffer[BUFFER_SIZE];
static char comBuffer[BUFFER_SIZE];
static char companyName[128];
static char productName[128];
static int outBuffPos = 0;
static int comBuffPos = 0;
static int firstWrite = 1;

static char whiteSpace[] = " \t\n\r/";
static char ignoreChars[] = "[]<>=+-*";
static char funtionChars[] =
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"1234567890-_:~.";

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V E R S I O N                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display version and help info.
 *  \param progName Full name of the program.
 *  \param helpThem Should help be shown.
 *  \result None.
 */
void version (char *progName, int helpThem)
{
	printf("TheKnight: Add Comment, Version: %s, Built: %s\n", VERSION, buildDate);
	displayLine();
	if (helpThem)
	{
		printf ("Enter the command: %s [-option] <filename>\n", basename(progName));
		printf ("Options are:\n\n");
		printf ("    -l<n>  . . . . . Add a comment only at line n.\n");
		printf ("    -w<n>  . . . . . Set the width of the box to n.\n");
		printf ("    -c<c>  . . . . . Set the character used to draw lines to c.\n");
		printf ("    -t<n>  . . . . . Set the tab size.\n");
		printf ("    -C<n>{owner} . . Copyright type n. 0 GPL, 1 Company, 2 Generic\n");
		printf ("    -P{name} . . . . Product name used in header.\n");
		printf ("    -h . . . . . . . Only add a header to the file.\n");
		printf ("    -i . . . . . . . Include a CVS ID tag in the file header.\n");
		printf ("    -d . . . . . . . Switch on debug output, don't overwrite.\n");
		printf ("    -p . . . . . . . Use c++ comments and a default - line.\n");
		printf ("    -q . . . . . . . Quiet mode, don't prompt for text.\n");
		printf ("    -m . . . . . . . Detect the main function and fill it in.\n");
		printf ("    -B . . . . . . . Look for comments in boxes and fix them.\n");
	}
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
int main(int argc, char *argv[])
{
	void *fileList = NULL;
	int i = 1, found = 0;
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

	displayInit();
	rl_bind_key ('\t',rl_abort);		/* disable auto-complete */

	while ((i = getopt(argc, argv, "hdipqmvBl:w:c:C:o:P:t:?")) != -1)
	{
		switch (i)
		{
		case 'l':
			commentLine = atoi (optarg);
			break;

		case 'h':
			commentLine = -1;
			break;

		case 'w':
			boxWidth = atoi (optarg);
			if (boxWidth < 40 || boxWidth > MAX_BOXWIDTH)
			{
				boxWidth = DEF_BOXWIDTH;
			}
			break;

		case 't':
			tabSize = atoi (optarg);
			if (tabSize < 1 || tabSize > 16)
			{
				tabSize = 8;
			}
			break;

		case 'c':
			if (optarg[0] >= ' ' && optarg[0] < 127)
			{
				lineChar = optarg[0];
			}
			break;

		case 'C':
			copyrightType = atoi (optarg);
			if (strlen (optarg) > 1)
			{
				strncpy (companyName, &optarg[1], 127);
			}
			break;

		case 'P':
			strncpy (productName, optarg, 127);
			break;

		case 'd':
			debug = !debug;
			break;

		case 'i':
			cvsID = !cvsID;
			break;

		case 'p':
			cppStyle = !cppStyle;
			lineChar = '-';
			break;

		case 'q':
			quietMode = !quietMode;
			break;

		case 'm':
			autoMain = !autoMain;
			break;

		case 'v':
			version (argv[0], 0);
			return 0;

		case 'B':
			boxCheck = !boxCheck;
			break;

		case '?':
			version (argv[0], 1);
			return (1);
		}
	}

	for (; optind < argc; ++optind)
	{
		found += directoryLoad(argv[optind], ONLYFILES, NULL, &fileList);
	}
	directorySort (&fileList);

	if (found)
		directoryProcess(addComments, &fileList);

	if (filesFound)
		displayLine();

	printf("%5d File%s changed\n", filesFound, filesFound == 1 ? "" : "s");
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  B U F F E R  R E A D                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read a character from the input file.
 *  \param inChar Return the read character here.
 *  \param inFile File to read from.
 *  \result 1 if a character was read 0 if not.
 */
int bufferRead(char *inChar, FILE * inFile)
{
	static int buffPos = 0, buffSize = 0;

	if (buffPos == buffSize)
	{
		if ((buffSize = fread(inBuffer, 1, BUFFER_SIZE, inFile)) == 0)
		{
			buffPos = 0;
			return 0;
		}
		buffPos = 0;
	}
	*inChar = inBuffer[buffPos++];
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  B U F F E R  F L U S H                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Flush all the bytes in the output buffer.
 *  \param outFile File to write any bytes in the buffer too.
 *  \param removeSpace Remove the leading white space from the buffer.
 *  \result None.
 */
void bufferFlush(FILE * outFile, int removeSpace)
{
	int saveSize = 0;

	if (outBuffPos)
	{
		outBuffer[outBuffPos] = 0;
		printf ("[Buffer: [%s]]\n", outBuffer);

		if (firstWrite && commentLine == -1)
		{
			displayBox(0, 1, outFile);
			++addedComments;
			comBuffPos = 0;
			firstWrite = 0;
		}
		if (removeSpace)
		{
			while (saveSize < outBuffPos && strchr (whiteSpace, outBuffer[saveSize]))
				++saveSize;
			if (saveSize)
			{
				if (fwrite(outBuffer, 1, saveSize, outFile) != saveSize)
					printf ("Buffer write problem!\n");
				outBuffPos -= saveSize;
				memcpy (outBuffer, &outBuffer[saveSize], outBuffPos);
			}
			if (removeSpace == 2)
			{
				while (saveSize < outBuffPos)
				{
					commentWrite(outBuffer[saveSize], outFile);
					++saveSize;
				}
			}
		}
		else
		{
			if (fwrite(outBuffer, 1, outBuffPos, outFile) != outBuffPos)
				printf ("Buffer write problem!\n");
			outBuffPos = 0;
		}
	}
}


/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O O K  F O R  A B O X                                                                                           *
 *  =======================                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Look to see if there is a box to be drawn.
 *  \param outFile Look for a comment box and expand it.
 *  \param braceLevel How many tabs to start with.
 *  \result 1 if a box was found.
 */
int lookForABox (FILE *outFile, int braceLevel)
{
	int outSize = boxWidth - (braceLevel * tabSize), firstText, lastText;
	int found = 0, level = 0, boxPos, textPos = 0, done = 0, retn = 0, i, j;
	char boxBuffer[BUFFER_SIZE], textBuffer[1024], lastChar = -1;

	comBuffer[comBuffPos] = 0;
	boxBuffer[boxPos = 0] = 0;
	for (i = 0; i < comBuffPos && !done; ++i)
	{
		if (level == 0)
		{
			if (strncmp (&comBuffer[i], "/***", 4) == 0)
			{
				for (j = 0; j < braceLevel; ++j)
				{
					boxBuffer[boxPos++] = '\t';
				}
				boxBuffer[boxPos++] = '/';
				boxBuffer[boxPos++] = '*';
				for (j = 0; j < outSize; ++j)
				{
					boxBuffer[boxPos++] = '*';
				}
				boxBuffer[boxPos++] = '*';
				boxBuffer[boxPos++] = '\n';
				boxBuffer[boxPos] = 0;
				level = 2;
			}
		}
		else if (level == 2)
		{
			if (comBuffer[i] == '\n')
			{
				for (j = 0; j < braceLevel; ++j)
				{
					boxBuffer[boxPos++] = '\t';
				}
				boxBuffer[boxPos++] = ' ';
				boxBuffer[boxPos++] = '*';
				boxBuffer[boxPos] = 0;
				level = 3;
			}
			else if (!(comBuffer[i] == '*' || comBuffer[i] == ' ' || comBuffer[i] == '\t'))
			{
				done = 1;
			}
		}
		else if (level == 3)
		{
			if (comBuffer[i] == '*')
			{
				textBuffer[firstText = lastText = textPos = 0] = 0;
				level = 4;
			}
			else if (!(comBuffer[i] == ' ' || comBuffer[i] == '\t'))
			{
				done = 1;
			}
		}
		else if (level == 4)
		{
			if (lastChar == '*' && comBuffer[i] == '/')
			{
				if (textPos > 0)
				{
					textBuffer[lastText] = 0;
					boxBuffer[boxPos++] = ' ';
					boxBuffer[boxPos++] = ' ';
					for (j = 0; j < lastText; ++j)
					{
						boxBuffer[boxPos++] = textBuffer[j];
					}
					firstText = lastText = textPos = 0;
					for (; j < outSize - 2; ++j)
					{
						boxBuffer[boxPos++] = ' ';
					}
					boxBuffer[boxPos++] = '*';
					boxBuffer[boxPos++] = '\n';

					for (j = 0; j < braceLevel; ++j)
					{
						boxBuffer[boxPos++] = '\t';
					}
					boxBuffer[boxPos++] = ' ';
					boxBuffer[boxPos++] = '*';
				}
				for (j = 0; j < outSize; ++j)
				{
					boxBuffer[boxPos++] = '*';
				}
				boxBuffer[boxPos++] = '*';
				boxBuffer[boxPos++] = '/';
				boxBuffer[boxPos] = 0;
				retn = 1;
				level = 0;
			}
			else if (comBuffer[i] == '\n')
			{
				textBuffer[lastText] = 0;
				boxBuffer[boxPos++] = ' ';
				boxBuffer[boxPos++] = ' ';
				for (j = 0; j < lastText; ++j)
				{
					boxBuffer[boxPos++] = textBuffer[j];
				}
				firstText = lastText = textPos = 0;
				for (; j < outSize - 2; ++j)
				{
					boxBuffer[boxPos++] = ' ';
				}
				boxBuffer[boxPos++] = '*';
				boxBuffer[boxPos++] = '\n';
				for (j = 0; j < braceLevel; ++j)
				{
					boxBuffer[boxPos++] = '\t';
				}
				boxBuffer[boxPos++] = ' ';
				boxBuffer[boxPos++] = '*';
			}
			else if (comBuffer[i] != '*' && comBuffer[i] >= ' ')
			{
				if (comBuffer[i] > ' ')
				{
					if (firstText == 0)
					{
						firstText = 1;
					}
					lastText = textPos + 1;
				}
				if (firstText)
				{
					textBuffer[textPos++] = comBuffer[i];
				}
			}
		}
		lastChar = comBuffer[i];
	}	
	if (retn == 1)
	{
		boxBuffer[boxPos] = 0;
		if (fwrite (boxBuffer, 1, boxPos, outFile) != boxPos)
		{
			printf ("Box write problem!\n");
			retn = 0;
		}
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O M M E N T  F L U S H                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Flush the contents of the comment buffer.
 *  \param outFile File to write any bytes in the buffer too.
 *  \param braceLevel How many tabs to start with.
 *  \result None.
 */
void commentFlush(FILE * outFile, int braceLevel)
{
	if (comBuffPos)
	{
		comBuffer[comBuffPos] = 0;
		printf ("[Comment: [%s]]\n", comBuffer);

		if (firstWrite && commentLine == -1)
		{
			displayBox(0, 1, outFile);
			++addedComments;
			firstWrite = 0;
		}
		else
		{
			if (boxCheck)
			{
				if (lookForABox	(outFile, braceLevel))
				{
					comBuffPos = 0;
					return;
				}
			}
			if (fwrite(comBuffer, 1, comBuffPos, outFile) != comBuffPos)
			{
				printf ("Buffer write problem!\n");
			}
		}
		comBuffPos = 0;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  B U F F E R  W R I T E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Write a character to the output buffer.
 *  \param inChar Character to write to the buffer.
 *  \param outFile If the buffer gets full write it to this file.
 *  \result None.
 */
void bufferWrite(char inChar, FILE * outFile)
{
	outBuffer[outBuffPos++] = inChar;
	if (outBuffPos == BUFFER_SIZE)
	{
		commentFlush(outFile, 0);
		bufferFlush(outFile, 0);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O M M E N T  W R I T E                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Write a character to the comment buffer.
 *  \param inChar Character to write to the buffer.
 *  \param outFile If the buffer gets full write it to this file.
 *  \result None.
 */
void commentWrite(char inChar, FILE * outFile)
{
	comBuffer[comBuffPos++] = inChar;
	if (comBuffPos == BUFFER_SIZE)
	{
		commentFlush(outFile, 0);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D O U B L E  N A M E                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a string into a doubled up string.
 *  \param oldName String to convert from.
 *  \param newName The converted string is saved here.
 *  \param underLn An under line string of the same length goes here.
 *  \result None.
 */
void doubleName(char *oldName, char *newName, char *underLn)
{
	int j = 0, upper;

	while (*oldName)
	{
		upper = !(*oldName >= 'a' && *oldName <= 'z');

		newName[j] = toupper(*oldName);
		underLn[j++] = '=';
		oldName++;

		if (*oldName)
		{
			if ((*oldName >= 'A' && *oldName <= 'Z') && upper == 0)
			{
				newName[j] = ' ';
				underLn[j++] = '=';
			}
			newName[j] = ' ';
			underLn[j++] = '=';
		}
	}
	newName[j] = 0;
	underLn[j] = 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M Y  R E A D  L I N E                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read a line from the users console.
 *  \param retnBuff Save the read line here.
 *  \param maxSize Maximum size of the read line.
 *  \param format Format of the prompt, may have extra parameters.
 *  \param ... Parmeters for the format.
 *  \result None.
 */
void myReadLine (char *retnBuff, int maxSize, char *format, ...)
{
	char fullPrompt[256], *readLine;
	va_list ap;

	if (quietMode)
	{
		retnBuff[0] = 0;
	}
	else
	{
		va_start(ap, format);
		vsprintf(fullPrompt, format, ap);
		va_end(ap);

		readLine = readline (fullPrompt);
		if (strlen (readLine) > maxSize)
			readLine[maxSize] = 0;
		add_history (readLine);
		strcpy (retnBuff, readLine);
		free (readLine);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E A R C H  B U F F                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Search the comment buffer for description of the parameter.
 *  \param findStr String to look for.
 *  \param outBuffer Returned description.
 *  \param endChar Character to end the buffer search on.
 *  \result Length of the description.
 */
int searchBuff (char *findStr, char *outBuffer, char endChar)
{
	int len = strlen (findStr), pos = 0, offset = 0;

	outBuffer[0] = 0;
	while (pos < comBuffPos)
	{
		while ((offset < len) && (findStr[offset] == comBuffer[pos + offset]) && (pos + offset < comBuffPos))
			++offset;
		if (offset == len)
			break;
		offset = 0;
		++pos;
	}
	if (offset == len)
	{
		pos += offset;
		offset = 0;
		while ((comBuffer[pos] != endChar) && (comBuffer[pos] != '\n') && (pos < comBuffPos) && comBuffer[pos])
		{
			outBuffer[offset] = comBuffer[pos++];
			outBuffer[++offset] = 0;
		}
	}
	return strlen(outBuffer);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  C O M M E N T                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the description for a parameter.
 *  \param retnBuff Return the description here.
 *  \param type Type of parameter.
 *  \param name Name of the parameter.
 *  \result None.
 */
void getComment (char *retnBuff, int type, char *name)
{
	*retnBuff = 0;
	switch (type)
	{
	case 0:
		if (!searchBuff ("\\brief ",  retnBuff, '.'))
		{
			if (!searchBuff ("@brief ",	 retnBuff, '.'))
				myReadLine (retnBuff, boxWidth - 9, "Enter (%s) description: ", name);
		}
		break;
	case 1:
		if (!searchBuff ("\\brief ",  retnBuff, '.'))
		{
			if (!searchBuff ("@brief ",	 retnBuff, '.'))
				myReadLine (retnBuff, boxWidth - 9, "Enter (%s) description: ", name);
		}
		break;
	case 2:
		{
			char buff[81];
			sprintf (buff, "\\param %s ", name);
			if (!searchBuff (buff,	retnBuff, '.'))
			{
				sprintf (buff, "@param %s ", name);
				if (!searchBuff (buff,	retnBuff, '.'))
					myReadLine (retnBuff, boxWidth - strlen(name) - 10, "Enter (%s) description: ", name);
			}
		}
		break;
	case 3:
		if (!searchBuff ("\\result ", retnBuff, '.'))
		{
			if (!searchBuff ("@result ", retnBuff, '.'))
				myReadLine (retnBuff, boxWidth - 10, "Enter result: ");
		}
		break;
	case 4:
		if (!searchBuff ("\\version ", retnBuff, '\n'))
			searchBuff ("@version ", retnBuff, '\n');
		break;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A D D  B O X  T E X T                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Add text to a box, with a simple wrap.
 *  \param outBuff Save the line so far here.
 *  \param inBuff Line to process.
 *  \param pad Add padding to the end of the line.
 *  \result Number of bytes read from the input.
 */
int addBoxText (char *outBuff, char *inBuff, int pad)
{
	int i = 0, s = 0, k = 0;

	if (inBuff[0])
	{
		outBuff[i++] = ' ';
		outBuff[i++] = ' ';
		while (i < boxWidth - 1 && inBuff[k])
		{
			outBuff[i] = inBuff[k++];
			if (outBuff[i] == ' ')
			{
				s = k;
			}
			++i;
		}
		if (i < boxWidth - 1)
		{
			s = k;
		}
		else
		{
			i = s + 1;
		}
		while (i < boxWidth && pad)
		{
			outBuff[i++] = ' ';
		}
	}
	outBuff[i] = 0;
	return s;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  B O X  L I N E                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Draw a part of a box.
 *  \param boxText Text to put in the box.
 *  \param first None zero if this is the first line of the box.
 *  \param last None zero if this is the last line of the box.
 *  \param addBlank None zero if a blank line should be added after the text.
 *  \param outFile File to output the box too.
 *  \result None.
 */
void boxLine (char *boxText, int first, int last, int addBlank, FILE *outFile)
{
	int pos = 0, len = strlen (boxText);
	char boxLine [MAX_BOXWIDTH + 1];
	char boxFill [MAX_BOXWIDTH + 1];
	char txtBuff [MAX_BOXWIDTH + 1];

	char firstStart[6],		firstEnd[6];
	char middleStart[6],	middleEnd[6];
	char lastStart[6],		lastEnd[6];

	if (cppStyle)
	{
		strcpy (firstStart, "//");
		strcpy (firstEnd, "");
		strcpy (middleStart, "//");
		strcpy (middleEnd, "");
		strcpy (lastStart, "//");
		strcpy (lastEnd, "");
	}
	else
	{
		strcpy (firstStart, "/*");
		strcpy (firstEnd, "*");
		strcpy (middleStart, " *");
		strcpy (middleEnd, "*");
		strcpy (lastStart, " *");
		strcpy (lastEnd, "*/");
	}
	if (first || last || addBlank)
	{
		int i, f = 0;
		for (i = 0; i < boxWidth; i++)
		{
			boxLine[i] = lineChar;
			if (middleEnd[0] > 0)
				boxFill[f++] = ' ';
		}
		boxLine[i] = 0;
		boxFill[f] = 0;
	}
	if (first)
	{
		fprintf(outFile, "%s%s%s\n", firstStart, boxLine, firstEnd);
		fprintf(outFile, "%s%s%s\n", middleStart, boxFill, middleEnd);
	}
	do
	{
		pos += addBoxText (txtBuff, &boxText[pos], middleEnd[0] > 0);
		fprintf(outFile, "%s%s%s\n", middleStart, txtBuff, middleEnd);
	}
	while (pos < len);
	if (addBlank)
	{
		fprintf(outFile, "%s%s%s\n", middleStart, boxFill, middleEnd);
	}
	if (last)
	{
		fprintf(outFile, "%s%s%s\n", middleStart, boxFill, middleEnd);
		fprintf(outFile, "%s%s%s\n", lastStart, boxLine, lastEnd);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  B O X                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display a box, could be called add the comment.
 *  \param type Type of comment to add, header or function.
 *  \param count Number of parameters including the name.
 *  \param outFile File to write the comment too.
 *  \result Nothing.
 */
int displayBox(int type, int count, FILE *outFile)
{
	struct tm *tm;
	time_t theTime = time(NULL);
	int i, functionMain = 0;
	char doubled [MAX_BOXWIDTH + 1];
	char underLn [MAX_BOXWIDTH + 1];
	char readLine[MAX_BOXWIDTH + 1];
	char firstCom[4], middleCom[4], lastCom[4];

	tm = localtime(&theTime);

	if (cppStyle)
	{
		strcpy (firstCom,	"///");
		strcpy (middleCom,	"///");
		strcpy (lastCom,	"///");
	}
	else
	{
		strcpy (firstCom,	"/**");
		strcpy (middleCom,	" * ");
		strcpy (lastCom,	" */");
	}

	if (type == 0)
	{
		char tempBuff[2048];

		doubleName(curFilename, doubled, underLn);

		boxLine (doubled, 1, 0, 0, outFile);
		boxLine (underLn, 0, 0, 1, outFile);
		if (copyrightType == 0)
		{
			sprintf (tempBuff, "Copyright (c) %d %s", tm -> tm_year + 1900, companyName);
			boxLine (tempBuff, 0, 0, 1, outFile);
			if (productName[0])
			{
				sprintf (tempBuff, "File %s part of %s", curFilename, productName);
			}
			else
			{
				sprintf (tempBuff, "File %s", curFilename);
			}
			strncat (tempBuff, " is free software: you can redistribute it and/or modify "
					 "it under the terms of the GNU General Public License as published by "
					 "the Free Software Foundation, either version 3 of the License, or (at "
					 "your option) any later version.", 2040);
			boxLine (tempBuff, 0, 0, 1, outFile);

			if (productName[0])
			{
				sprintf (tempBuff, "%s", productName);
			}
			else
			{
				sprintf (tempBuff, "File %s", curFilename);
			}
			strncat (tempBuff, " is distributed in the hope that it will be useful, but "
					 "WITHOUT ANY WARRANTY; without even the implied warranty of "
					 "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
					 "General Public License for more details.", 2040);
			boxLine (tempBuff, 0, 0, 1, outFile);

            boxLine ("You should have received a copy of the GNU General Public License "
					 "along with this program. If not, see: <http://www.gnu.org/licenses/>",
					 0, 1, 0, outFile);
		}
		else if (copyrightType == 1)
		{
			sprintf (tempBuff, "Copyright (c) %d %s", tm -> tm_year + 1900, companyName);
			boxLine (tempBuff, 0, 0, 1, outFile);
			boxLine ("All rights reserved. Reproduction, modification, use or disclosure "
					 "to third parties without express authority is forbidden.", 0, 1, 0, outFile);
		}
		else
		{
			sprintf (tempBuff, "Copyright (c) %d.", tm -> tm_year + 1900);
			boxLine (tempBuff, 0, 1, 0, outFile);
		}

		getComment (readLine, 0, curFilename);
		fprintf(outFile, "%s\n", firstCom);
		fprintf(outFile, "%s \\file\n", middleCom);
		fprintf(outFile, "%s \\brief %s.\n", middleCom, readLine);
		if (cvsID)
		{
			getComment (readLine, 4, curFilename);
			if (readLine[0])
				fprintf(outFile, "%s \\version %s\n", middleCom, readLine);
			else
				fprintf(outFile, "%s \\version %cId: %s 0 %04d-%02d-%02d 00:00:00Z owner $\n", middleCom,
						'$', curFilename, tm -> tm_year + 1900, tm -> tm_mon + 1,  tm -> tm_mday);
		}
		fprintf(outFile, "%s\n", lastCom);
	}
	else if (type == 1)
	{
		doubleName(possibleName[0], doubled, underLn);

		boxLine (doubled, 1, 0, 0, outFile);
		boxLine (underLn, 0, 1, 0, outFile);

		for (i = 0; i < count; i++)
		{
			if (i == 0)
				printf ("[%s] Function: %s (", curFilename, possibleName[i]);
			else if (i == 1)
				printf ("%s", possibleName[i]);
			else
				printf (", %s", possibleName[i]);
		}
		printf (");\n");

		if (autoMain && strcmp (possibleName[0], "main") == 0)
		{
			functionMain = 1;
			fprintf(outFile, "%s\n", firstCom);
			fprintf(outFile, "%s \\brief %s.\n", middleCom, "The program starts here");
		}
		else
		{
			getComment (readLine, 1, possibleName[0]);
			fprintf(outFile, "%s\n", firstCom);
			fprintf(outFile, "%s \\brief %s.\n", middleCom, readLine);
		}
		for (i = 1; i < count; i++)
		{
			if (functionMain && strcmp (possibleName[i], "argc") == 0)
			{
				fprintf(outFile, "%s \\param %s %s.\n", middleCom, possibleName[i],
						"The number of arguments passed to the program");
				continue;
			}
			if (functionMain && strcmp (possibleName[i], "argv") == 0)
			{
				fprintf(outFile, "%s \\param %s %s.\n", middleCom, possibleName[i],
						"Pointers to the arguments passed to the program");
				continue;
			}
			getComment (readLine, 2, possibleName[i]);
			fprintf(outFile, "%s \\param %s %s.\n", middleCom, possibleName[i], readLine);
		}

		if (functionMain)
		{
			fprintf(outFile, "%s \\result %s.\n", middleCom, "0 (zero) if all processed OK");
		}
		else
		{
			getComment (readLine, 3, "");
			fprintf(outFile, "%s \\result %s.\n", middleCom, readLine);
		}
		fprintf(outFile, "%s\n", lastCom);
		displayLine ();
	}
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  I S  K E Y  W O R D                                                                                               *
 *  ===================                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Check if this is a special keyword.
 *  \param currentWord Word to check.
 *  \result 1 if it is a keyword, 0 if not.
 */
int isKeyWord (char *currentWord)
{
	if (!strcmp (currentWord, "void"))
		return 1;

	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D E B U G  L I N E                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Paramters for the format.
 *  \param format Line to output, may have extra parameters.
 *  \param ... Parameter for the format.
 *  \result None.
 */
void debugLine(char *format, ...)
{
	va_list ap;

	if (debug)
	{
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A D D  C O M M E N T S                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call back from the directoryProcess to display the file.
 *  \param file File found in the directory.
 *  \result 1 if file processed.
 */
int addComments(DIR_ENTRY * file)
{
	char inChar, lastChar = 0;
	char currentWord[80];
	char inFile[PATH_SIZE], outFile[PATH_SIZE], bkpFile[PATH_SIZE];
	int braceLevel = 0, bracketLevel = 0, inComment = 0, inQuote = 0, inDefine = 0, colonCount = 0;
	int addComment = 0, curWordPos = 0, curParam = 0, clear = 0, line = 1, funcLine = 0;
	FILE *readFile, *writeFile;

	strcpy(inFile, file->fullPath);
	strcat(inFile, file->fileName);

	strcpy(outFile, file->fullPath);
	strcat(outFile, file->fileName);
	strcat(outFile, ".$$$");

	firstWrite = 1;
	addedComments = 0;
	possibleName[0][0] = 0;
	currentWord[curWordPos = 0] = 0;

	if ((readFile = fopen(inFile, "rb")) != NULL)
	{
		if ((writeFile = fopen(outFile, "wb")) != NULL)
		{
			strcpy(curFilename, file -> fileName);

			while (bufferRead(&inChar, readFile))
			{
				if (inChar == '\n')
					line++;

				if (inComment == 1)
				{
					if (lastChar == '*' && inChar == '/')
					{
						debugLine("Line %d: Ending C comment\n", line);
						commentFlush(writeFile, braceLevel);
						if (braceLevel) clear = 1;
						inComment = 0;
					}
				}
				else if (inComment == 2)
				{
					if (inChar == '\n')
					{
						debugLine("Line %d: Ending C++ comment\n", line);
						if (braceLevel) clear = 1;
						inComment = 0;

						if (inDefine == 1)
						{
							debugLine("Line %d: Ending define\n", line);
							inDefine = 0;
						}
					}
				}
				else if (lastChar == '/' && inChar == '*' && !inQuote)
				{
					debugLine("Line %d: Starting C comment\n", line);
					if (outBuffPos)
					{
						--outBuffPos;
						commentFlush(writeFile, braceLevel);
						bufferFlush(writeFile, boxCheck ? 2 : 0);
						commentWrite('/', writeFile);
					}
					inComment = 1;
				}
				else if (lastChar == '/' && inChar == '/' && !inQuote)
				{
					debugLine("Line %d: Starting C++ comment\n", line);
					if (outBuffPos)
					{
						--outBuffPos;
						commentFlush(writeFile, braceLevel);
						bufferFlush(writeFile, 0);
						commentWrite('/', writeFile);
					}
					inComment = 2;
				}
				else if (inChar == DQ && lastChar != '\\' && inQuote != 1)
				{
					if (inQuote == 0)
					{
						debugLine("Line %d: Starting double quote\n", line);
						inQuote = 2;
					}
					else if (inQuote == 2)
					{
						debugLine("Line %d: Ending double quote\n", line);
						inQuote = 0;
					}
				}
				else if (inChar == SQ && lastChar != '\\' && inQuote != 2)
				{
					if (inQuote == 0)
					{
						debugLine("Line %d: Starting single quote\n", line);
						inQuote = 1;
					}
					else if (inQuote == 1)
					{
						debugLine("Line %d: Ending single quote\n", line);
						inQuote = 0;
					}
				}
				else if (inQuote)
				{
					;
				}
				else if (inDefine == 1)
				{
					if (inChar == '\n')
					{
						debugLine("Line %d: Ending define\n", line);
						inDefine = 0;
					}
				}
				else if (inChar == '#')
				{
					debugLine("Line %d: Starting define\n", line);
					commentFlush(writeFile, braceLevel);
					bufferFlush(writeFile, 0);
					inDefine = 1;
				}
				else if (inChar == ':')
				{
					if (bracketLevel == 0 && braceLevel == 0)
						++colonCount;
					currentWord[curWordPos++] = inChar;
					currentWord[curWordPos] = 0;
				}
				else if (inChar == '{')
				{
					debugLine("Line %d: Starting brace level (%d)\n", line, braceLevel + 1);
					if (braceLevel == 0)
					{
						if (curParam)
						{
							if (commentLine == 0 || commentLine == funcLine)
							{
								bufferFlush(writeFile, 1);
								addedComments += displayBox(1, curParam, writeFile);
								comBuffPos = 0;
								bufferFlush(writeFile, 0);
							}
							curParam = 0;
						}
						colonCount = 0;
					}
					braceLevel++;
				}
				else if (inChar == '}' && braceLevel)
				{
					debugLine("Line %d: Ending brace level (%d)\n", line, braceLevel);
					if (--braceLevel == 0)
					{
						commentFlush(writeFile, braceLevel);
						colonCount = 0;
						clear = 1;
					}
				}
				else if (inChar == '(')
				{
					debugLine("Line %d: Starting bracket level (%d)\n", line, bracketLevel + 1);
					if (bracketLevel == 0)
					{
						if (currentWord[0] && colonCount < 3)
						{
							funcLine = line;
							strcpy(possibleName[0], currentWord);
							currentWord[0] = 0;
							curParam = 1;
						}
					}
					bracketLevel++;
				}
				else if (inChar == ',' && bracketLevel == 1 && braceLevel == 0)
				{
					if (currentWord[0])
					{
						debugLine("Line %d: Found possible name (%s)\n", line, currentWord);
						strcpy(possibleName[curParam], currentWord);
						currentWord[0] = 0;
						if (++curParam == 30)
							curParam = 29;
					}
				}
				else if (inChar == ')' && bracketLevel)
				{
					debugLine("Line %d: Ending bracket level (%d)\n", line, bracketLevel);
					if (bracketLevel == 1 && braceLevel == 0 && colonCount < 3)
					{
						if (currentWord[0])
						{
							debugLine("Line %d: Found possible name (%s)\n", line, currentWord);
							if (!isKeyWord (currentWord))
							{
								strcpy(possibleName[curParam], currentWord);
								if (++curParam == 30)
									curParam = 29;
							}
							currentWord[0] = 0;
						}
					}
					bracketLevel--;
				}
				else if (strchr(ignoreChars, inChar) || strchr(whiteSpace, inChar))
				{
					curWordPos = 0;
				}
				else if (inChar == ';' && !bracketLevel && !braceLevel)
				{
					commentFlush(writeFile, braceLevel);
					colonCount = 0;
					curParam = 0;
					clear = 1;
				}
				else if (strchr(funtionChars, inChar))
				{
					currentWord[curWordPos++] = inChar;
					currentWord[curWordPos] = 0;
				}
				else
				{
					currentWord[curWordPos = 0] = 0;
				}
				if (inComment)
				{
					commentWrite(inChar, writeFile);
					addComment = 1;
				}
				else
				{
					if (strchr(whiteSpace, inChar) && addComment)
					{
						commentWrite(inChar, writeFile);
					}
					else
					{
						addComment = 0;
						bufferWrite(inChar, writeFile);
						if (clear)
						{
							commentFlush(writeFile, braceLevel);
							bufferFlush(writeFile, 0);
							clear = 0;
						}
					}
				}
				if (lastChar == '\\' && inChar == '\\')
					lastChar = 0;
				else
					lastChar = inChar;
			}
			commentFlush(writeFile, braceLevel);
			bufferFlush(writeFile, 0);
			fclose(writeFile);
		}
		fclose(readFile);

		if (addedComments)
		{
			strcpy(bkpFile, file->fullPath);
			strcat(bkpFile, file->fileName);
			strcat(bkpFile, "~");

			if (rename (inFile, bkpFile) != 0)
			{
				fprintf (stderr, "Rename from %s to %s failed!\n", inFile, bkpFile);
				return 0;
			}
			if (rename (outFile, inFile) != 0)
			{
				fprintf (stderr, "Rename from %s to %s failed!\n", inFile, outFile);
				return 0;
			}
		}
		else
		{
			if (unlink (outFile) != 0)
			{
				fprintf (stderr, "Delete %s failed!\n", outFile);
				return 0;
			}
		}
		if (bracketLevel || braceLevel || inComment || inQuote || inDefine)
		{
			printf("WARNING unexpected EOF: Bracket: %d, Brace: %d, Comment: %d, "
				   "Quote: %d, Define: %d\n", bracketLevel, braceLevel, inComment,
				   inQuote, inDefine);
		}
		printf("      %-16s %d added comment(s)\n", file->fileName, addedComments);
		filesFound++;
	}
	return 1;
}

