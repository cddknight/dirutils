/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A T C H . C                                                                                                     *
 *  =============                                                                                                     *
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
 *  \brief Functions to compare strings with wildcards.
 */
#define _GNU_SOURCE
 
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnmatch.h>
#include "dircmd.h"

#define NO_CMD		0
#define OR_CMD		1
#define AND_CMD		2
#define NOT_CMD		4

#define FALSE_STATE	0
#define TRUE_STATE	1
#define STOP_STATE	2

/*----------------------------------------------------------------------------*
 * Local Prototypes   													      *
 *----------------------------------------------------------------------------*/
static int evalCmd (int newLogic, int oldLogic, int cmd);

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A T C H  L O G I C                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief A pattern match with AND and OR added.
 *  \param str String to try and match with.
 *  \param ptn The pattern to compare against.
 *  \param flags Passed to pattern match.
 *  \result True if there is a match, false if not.
 */
int matchLogic (char *str, char *ptn, int flags)
{
	int lastCmd = NO_CMD, curLogic = TRUE_STATE, i = 0;
	char *curPtn;

	if ((curPtn = (char *)malloc (strlen (ptn) + 1)) == 0)
		return 0;

	while (*ptn && !(curLogic & STOP_STATE))
	{
		if (*ptn == '&')
		{
			curLogic = evalCmd (matchPattern (str, curPtn, flags), curLogic, lastCmd);
			lastCmd = AND_CMD;
			i = 0;
			if (curLogic == FALSE_STATE)
				break;
		}
		else if (*ptn == '|')
		{
			curLogic = evalCmd (matchPattern (str, curPtn, flags), curLogic, lastCmd);
			lastCmd = OR_CMD;
			i = 0;
		}
		else if (*ptn == '^' && i == 0)
		{
			lastCmd |= NOT_CMD;
		}
		else
		{
			curPtn[i++] = *ptn;
			curPtn[i] = 0;
		}
		ptn ++;
	}
	if (i && !(curLogic & STOP_STATE))
	{
		curLogic = evalCmd (matchPattern (str, curPtn, flags), curLogic, lastCmd);
	}
	free (curPtn);

	return (curLogic & TRUE_STATE);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  E V A L  C M D                                                                                                    *
 *  ==============                                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Evaluates the AND, OR and NOT from the logic.
 *  \param newLogic New command to process.
 *  \param oldLogic Current state of the logic.
 *  \param cmd New command to process.
 *  \result True or False.
 */
static int evalCmd (int newLogic, int oldLogic, int cmd)
{
	int retn = FALSE_STATE;

	if (cmd & NOT_CMD)
	{
		newLogic = !newLogic;
		cmd &= ~NOT_CMD;
	}
	if (cmd == OR_CMD)
		retn = (newLogic || oldLogic) ? TRUE_STATE : FALSE_STATE;
	else if (cmd == AND_CMD)
		retn = (newLogic && oldLogic) ? TRUE_STATE : FALSE_STATE | STOP_STATE;
	else if (cmd == NO_CMD)
		retn = (newLogic) ? TRUE_STATE : FALSE_STATE;

	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A T C H  P A T T E R N                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare a filename against a pattern.
 *  \param str File name to compare.
 *  \param ptn Pattern to compare the filename with.
 *  \param flags Extra control on how the match is done.
 *  \result True or False.
 */
int matchPattern (char *str, char *ptn, int flags)
{
	int fnFlags = FNM_PATHNAME;
	
	if (!(flags & SHOWALL))
	{
		fnFlags |= FNM_PERIOD;
	}	
	if (!(flags & USECASE))
	{
		char pattern[PATH_SIZE], string[PATH_SIZE];
		int i = 0;
		
		while (ptn[i] && i < 255)
		{
			pattern[i] = tolower (ptn[i]);
			i++;
		}
		pattern[i] = 0;
		
		i = 0;
		while (str[i] && i < 255)
		{
			string[i] = tolower (str[i]);
			i++;
		}
		string[i] = 0;

		return fnmatch(pattern, string, fnFlags) ? FALSE_STATE : TRUE_STATE;
	}

	return fnmatch(ptn, str, fnFlags) ? FALSE_STATE : TRUE_STATE;
}

