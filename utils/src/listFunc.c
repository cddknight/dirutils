/**********************************************************************************************************************
 *                                                                                                                    *
 *  L I S T  F U N C . C                                                                                              *
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
 *  \brief Chris Knight's own program to list C functions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dircmd.h>

/*----------------------------------------------------------------------------*
 * Prototypes															      *
 *----------------------------------------------------------------------------*/
int fileCompare(DIR_ENTRY * fileOne, DIR_ENTRY * fileTwo);
int showDir(DIR_ENTRY * file);

#define BUFFER_SIZE 1024

/*----------------------------------------------------------------------------*
 * Globals                                                                    *
 *----------------------------------------------------------------------------*/
COLUMN_DESC colFuncDescs[4] =
{
	{	10,	10,	0,	3,	0x07,	COL_ALIGN_RIGHT,	"Count",	3	},	/* 0 */
	{	10,	10,	0,	3,	0x07,	COL_ALIGN_RIGHT,	"Line",		2	},	/* 1 */
	{	160,12,	0,	3,	0x07,	0,					"Filename",	1	},	/* 2 */
	{	160,12,	0,	0,	0x07,	0,					"Function",	0	},	/* 3 */
};

COLUMN_DESC *ptrFuncColumn[4] =
{
	&colFuncDescs[0],
	&colFuncDescs[1],
	&colFuncDescs[2],
	&colFuncDescs[3]
};

int filesFound = 0;
int totalFuncs = 0;
int javaMode = 0;
int debug = 0;

static char inBuffer[BUFFER_SIZE];
static char whiteSpace[] = " \t\n\r[]<>=+-*/";
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
 *  \brief Display the version of the application.
 *  \result None.
 */
void version (void)
{
	printf("TheKnight: List Functions, Version %s\n", directoryVersion());
	displayLine();
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

	displayGetWindowSize();

	if (argc == 1)
	{
		version();
		printf("Enter the command: %s [options] <filename>\n", argv[0]);
		exit(1);
	}
	/*------------------------------------------------------------------------*
     * If we got a path then split it into a path and a file pattern to match *
	 * files with.                                                            *
     *------------------------------------------------------------------------*/
	while (i < argc)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'd':
				debug = 1;
				break;

			case 'j':
				javaMode = 1;
				break;
				
			case '?':
				printf("Options are:\n\n");
				printf("    -d ...... Switch on debug output.\n");
				return (1);
			}
		}
		else
		{
			found += directoryLoad(argv[i], ONLYFILES, fileCompare, &fileList);
		}
		i++;
	}

    /*------------------------------------------------------------------------*
     * Now we can sort the directory.                                         *
     *------------------------------------------------------------------------*/
	directorySort (&fileList);

	if (found)
	{
		char numBuff[15];
		
		if (!displayColumnInit (4, ptrFuncColumn, DISPLAY_HEADINGS))
		{
			fprintf (stderr, "ERROR in: displayColumnInit\n");
			return 1;
		}
		directoryProcess (showDir, &fileList);
		
		displayInColumn (1, displayCommaNumber (totalFuncs, numBuff));
		displayInColumn (2, "Functions");
		displayNewLine(DISPLAY_INFO);
		displayInColumn (1, displayCommaNumber (filesFound, numBuff));
		displayInColumn (2, "Files");
		displayNewLine(DISPLAY_INFO);
		displayAllLines ();		
		
		displayTidy ();
	}
	else
	{
		version();
		printf ("No files found\n");
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  B U F F E R  R E A D                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read a chunck of bytes from a file.
 *  \param inChar Return the next charater read here.
 *  \param inFile File to read from.
 *  \result True if a character was read.
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
 *  I S  K E Y  W O R D                                                                                               *
 *  ===================                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Is this word a special work for C.
 *  \param currentWord Word to check.
 *  \result True if it is special.
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
 *  \brief If debug is on print debug information.
 *  \param format String to display.
 *  \param ... Extra paramters for the format.
 *  \result Nothing.
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
 *  D I S P L A Y  F U N C                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Output the name and parameters of the fuction found.
 *  \param fileName Name of the file the function was found in.
 *  \param line Line number that function was found on.
 *  \param func The number of the function in the file.
 *  \param count Number of parameters, first is function name.
 *  \param values Pointers to the parmamters.
 *  \result Nothing.
 */
void displayFunc(char *fileName, int line, int func, int count, char *values[])
{
	int i;
	char numBuffer[21];

	displayInColumn (0, displayCommaNumber (func, numBuffer));
	displayInColumn (1, displayCommaNumber (line, numBuffer));
	displayInColumn (2, "%s", fileName);
	
	for (i = 0; i < count; i++)
	{
		if (i == 0)
		{
			displayInColumn (3, "%s (", values[i]);
		}
		else if (i == 1)
		{
			displayInColumn (3, "%s", values[i]);
		}
		else
		{
			displayInColumn (3, ", %s", values[i]);
		}
	}
	displayInColumn (3, ");");
	displayNewLine (0);
	totalFuncs ++;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  D I R                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called back by process dir to show the directory.
 *  \param file Information about the file to search.
 *  \result 1 if the file contained functions.
 */
int showDir(DIR_ENTRY * file)
{
	char inChar, lastChar;
	char *possibleName[30];
	char currentWord[80];
	char inFile[PATH_SIZE];
	int braceLevel, bracketLevel, inComment, inQuote, inDefine;
	int curWordPos = 0, curParam = 0, i, line = 1, shownFunc = 0;
	int funcLine = 0;
	FILE *readFile;

	strcpy(inFile, file->fullPath);
	strcat(inFile, file->fileName);

	for (i = 0; i < 30; i++)
	{
		if ((possibleName[i] = (char *)malloc(80)) == NULL)
			return 0;
	}

	lastChar = 0;
	possibleName[0][0] = 0;
	currentWord[curWordPos = 0] = 0;
	bracketLevel = braceLevel = inComment = inQuote = inDefine = curParam = 0;

	if ((readFile = fopen(inFile, "rb")) != NULL)
	{
		while (bufferRead(&inChar, readFile))
		{
			if (inChar == '\n')
				line++;

			if (inComment == 1)
			{
				if (lastChar == '*' && inChar == '/')
				{
					debugLine("Line %d: Ending C comment\n", line);
					inComment = 0;
				}
			}
			else if (inComment == 2)
			{
				if (inChar == '\n')
				{
					debugLine("Line %d: Ending C++ comment\n", line);
					inComment = 0;
				}
			}
			else if (lastChar == '/' && inChar == '*' && !inQuote)
			{
				debugLine("Line %d: Starting C comment\n", line);
				inComment = 1;
			}
			else if (lastChar == '/' && inChar == '/' && !inQuote)
			{
				debugLine("Line %d: Starting C++ comment\n", line);
				inComment = 2;
			}
			else if (inChar == '"' && lastChar != '\\' && inQuote != 1)
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
			else if (inChar == '\'' && lastChar != '\\' && inQuote != 2)
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
				inDefine = 1;
			}
			else if (inChar == '{')
			{
				debugLine("Line %d: Starting brace level (%d)\n", line, braceLevel + 1);
				if ((!javaMode && braceLevel == 0) || (javaMode && braceLevel == 1))
				{
					if (curParam)
					{
						displayFunc(file->fileName, funcLine, ++shownFunc, curParam, possibleName);
						curParam = 0;
					}
				}
				braceLevel++;
			}
			else if (inChar == '}' && braceLevel)
			{
				debugLine("Line %d: Ending brace level (%d)\n", line, braceLevel);
				braceLevel--;
			}
			else if (inChar == '(')
			{
				debugLine("Line %d: Starting bracket level (%d)\n", line, bracketLevel + 1);
				if (bracketLevel == 0)
				{
					if (currentWord[0])
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
				if (bracketLevel == 1 && braceLevel == 0)
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
			else if (strchr(whiteSpace, inChar))
			{
				curWordPos = 0;
			}
			else if (inChar == ';' && !bracketLevel && !braceLevel)
			{
				curParam = 0;
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

			if (lastChar == '\\' && inChar == '\\')
				lastChar = 0;
			else
				lastChar = inChar;
		}
		fclose(readFile);

		if (bracketLevel || braceLevel || inComment || inQuote || inDefine)
		{
			printf("(%s) WARNING unexpected EOF: Bracket: %d, Brace: %d, Comment: %d, "
				   "Quote: %d, Define: %d\n", file->fileName, bracketLevel, braceLevel, 
				   inComment, inQuote, inDefine);
		}
		displayDrawLine (0);
		filesFound++;
	}
	for (i = 0; i < 30; i++)
	{
		free(possibleName[i]);
	}
	return shownFunc ? 1 : 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F I L E  C O M P A R E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called back by load dir to sort the files.
 *  \param fileOne First file.
 *  \param fileTwo Second file to compare first with.
 *  \result 1, 0 or -1 depending on order.
 */
int fileCompare(DIR_ENTRY * fileOne, DIR_ENTRY * fileTwo)
{
	if (fileOne->fileStat.st_mode & S_IFDIR)
	{
		if (!(fileTwo->fileStat.st_mode & S_IFDIR))
			return -1;
	}
	if (fileTwo->fileStat.st_mode & S_IFDIR)
	{
		if (!(fileOne->fileStat.st_mode & S_IFDIR))
			return 1;
	}
	return strcasecmp(fileOne->fileName, fileTwo->fileName);
}
