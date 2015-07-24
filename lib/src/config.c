/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G . C                                                                                                   *
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
 *  \brief Read and write configuration to a file.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dircmd.h"

typedef struct _configEntry
{
	char *configName;
	char *configValue;
}
CONFIG_ENTRY;

void *configQueue = NULL;


/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  L O A D                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load the cobviguration file.
 *  \param configFile File to load.
 *  \result 1 if all OK.
 */
int configLoad (const char *configFile)
{
	FILE *inFile;
	char readBuff[512], configName[81], configValue[256];
	int i, j, quote;
	
	if (configQueue == NULL)
	{
		if ((configQueue = queueCreate ()) == NULL)
			return 0;
	}
	
	if (configFile[0] == 0)
		return 0;
	
	if ((inFile = fopen (configFile, "r")) == NULL)
		return 0;

 	while (fgets (readBuff, 512, inFile))
	{
		i = 0;
		quote = 0;
		
		/* Skip leading white space
		 *--------------------------------------------------------------------*/
		while (readBuff[i] <= ' ')
			i++;
			
		/* Skip comments
		 *--------------------------------------------------------------------*/
		if (readBuff[i] == '#')
			continue;
		
		/* Read parameter name
		 *--------------------------------------------------------------------*/
		j = 0;	
		while (readBuff[i] > ' ' && readBuff[i] != '=' && j < 80)
		{
			configName[j++] = readBuff[i];
			configName[j] = 0;
			i ++;
		}
		
		/* Skip to equal sign
		 *--------------------------------------------------------------------*/
		while (readBuff[i] != 0 && readBuff[i] != '=')
			i++;
			
		/* No equal sign then this is not a config line
		 *--------------------------------------------------------------------*/
		if (readBuff[i] == 0)
			continue;
			
		/* Skip while space after the equal sign
		 *--------------------------------------------------------------------*/
		i++;
		while (readBuff[i] <= ' ')
			i++;

		/* Read parameter value
		 *--------------------------------------------------------------------*/
		configValue[j = 0] = 0;
		while (readBuff[i] > 0 && j < 255)
		{
			if (readBuff[i] == '"')
			{
				quote = quote == 0 ? 1 : 0;
			}
			else
			{
				if (quote == 0 && (readBuff[i] <= ' ' || readBuff[i] == '#'))
					break;
									
				configValue[j++] = readBuff[i];
				configValue[j] = 0;
			}
			i ++;
		}
		
		/* Save the name and value
		 *--------------------------------------------------------------------*/
		configSetValue (configName, configValue);
	}
	fclose (inFile);
	return 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  F R E E                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Free the memory used by the configuration .
 *  \result None.
 */
void configFree ()
{
	CONFIG_ENTRY *foundEntry = NULL;

	if (configQueue != NULL)
	{
		while ((foundEntry = queueGet (configQueue)) != NULL)
		{
			free (foundEntry -> configName);
			free (foundEntry -> configValue);
			free (foundEntry);
		}
		queueDelete (configQueue);
		configQueue = NULL;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  F I N D  E N T R Y                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Find the value of a name.
 *  \param configName Name of the value to find.
 *  \result A config entery that was found or NULL.
 */
static CONFIG_ENTRY *configFindEntry (const char *configName)
{
	int rec = 0;
	CONFIG_ENTRY *foundEntry = NULL;
	
	if (configQueue != NULL)
	{
		while ((foundEntry = queueRead (configQueue, rec)) != NULL)
		{
			if (strcmp (configName, foundEntry -> configName) == 0)
				break;
			rec ++;
		}
	}
	return foundEntry;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  S E T  V A L U E                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set a config value.
 *  \param configName The name for the value.
 *  \param configValue The value of the value.
 *  \result 1 if all OK.
 */
int configSetValue (char *configName, char *configValue)
{
	CONFIG_ENTRY *newEntry = configFindEntry (configName);

	if (configQueue == NULL)
		return 0;
	
	if (newEntry == NULL)
	{
		if ((newEntry = malloc (sizeof (CONFIG_ENTRY))) == NULL)
			return 0;
			
		if ((newEntry -> configName = malloc (strlen (configName) + 1)) == NULL)
		{
			free (newEntry);
			return 0;
		}
		strcpy (newEntry -> configName, configName);
		
		if ((newEntry -> configValue = malloc (strlen (configValue) + 1)) == NULL)
		{
			free (newEntry -> configName);
			free (newEntry);
			return 0;
		}
		strcpy (newEntry -> configValue, configValue);
		
		queuePut (configQueue, newEntry);
	}
	else
	{
		char *tempPtr;
		if ((tempPtr = malloc (strlen (configValue) + 1)) == NULL)
			return 0;

		strcpy (tempPtr, configValue);
		free (newEntry -> configValue);
		newEntry -> configValue = tempPtr;
	}
	return 1;
}	
		
/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  S E T  I N T  V A L U E                                                                              *
 *  ====================================                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set an interger config value.
 *  \param configName The name for the value.
 *  \param configValue The value of the value.
 *  \result 1 if all OK.
 */
int configSetIntValue (char *configName, int configValue)
{
	char numBuffer[21];
	
	sprintf (numBuffer, "%d", configValue);
	
	return configSetValue (configName, numBuffer);
}
	
/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  G E T  V A L U E                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the value of an config item.
 *  \param configName Name of the value to find.
 *  \param value Save the value here.
 *  \result 1 if value found.
 */
int configGetValue (char *configName, char *value)
{
	CONFIG_ENTRY *foundEntry = configFindEntry (configName);

	if (foundEntry != NULL)
	{
		strcpy (value, foundEntry -> configValue);
		return 1;
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  G E T  I N T  V A L U E                                                                              *
 *  ====================================                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the integer value of a config item.
 *  \param configName Name of the value to find.
 *  \param value Save the value here.
 *  \result 1 if value found.
 */
int configGetIntValue (const char *configName, int *value)
{
	CONFIG_ENTRY *foundEntry = configFindEntry (configName);

	*value = 0;
	if (foundEntry != NULL)
	{
		sscanf (foundEntry -> configValue, "%i", value);
		return 1;
	}
	return 0;
}

