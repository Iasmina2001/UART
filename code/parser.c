#include <stdlib.h>
#include <stdio.h>
#include "parser.h"

#define CR 0x0D
#define LF 0x0A

AT_COMMAND_DATA myData;


void initATData()
{
	int i = 0;
	int j = 0;
	myData.ok = 0;
	myData.column_count = 0;
	myData.line_count = 0;
	for (i = 0; i < AT_COMMAND_MAX_LINES; i++)
		for (j = 0; j < AT_COMMAND_MAX_LINE_SIZE; j++)
			myData.data[i][j] = 0;
}

void addCharacterToATTextMatrix(uint8_t character)
{
	myData.data[myData.line_count][myData.column_count] = character;
	myData.column_count++;
}

void printATData()
{
	int line = 0;
	int column = 0;
	printf("\n------------AT DATA------------\n");
	while (line < myData.line_count)
	{
		column = 0;
		while (myData.data[line][column] != '\0')
		{
			printf("%c", myData.data[line][column]);
			column++;
		}
		line++;
	}
	printf("-------------------------------\n");

	if (myData.ok)
		printf("--OK--\n");
	else
		printf("--ERROR--\n");
}

STATE_MACHINE_RETURN_VALUE at_command_parse(uint8_t current_character, uint8_t flag)
{
	// printf("stare: %d - %c\n", state, current_character);
	switch (state)
	{
	case 0:
	{
		if (current_character == CR)
		{
			state = 1;
			initATData();
		}
		break;
	}
	case 1:
	{
		if (current_character == LF)
			state = 2;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 2:
	{
		if (current_character == '+' && flag == 0)
		{
			state = 12;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
			{
				addCharacterToATTextMatrix(current_character);
			}
		}
		else if (current_character == 'E' && flag == 0)
			state = 7;
		else if (current_character == 'O' && flag == 0)
			state = 3;
		else if (current_character >= 48 && current_character <= 122 && flag == 1)
		{
			state = 20;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
			{
				addCharacterToATTextMatrix(current_character);
			}
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 3:
	{
		if (current_character == 'K')
		{
			state = 4;
			myData.ok = 1;
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 4:
	{
		if (current_character == CR)
			state = 5;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 5:
	{
		if (current_character == LF)
		{
			state = 0;
			return STATE_MACHINE_READY_OK;
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 7:
	{
		if (current_character == 'R')
			state = 8;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 8:
	{
		if (current_character == 'R')
			state = 9;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 9:
	{
		if (current_character == 'O')
			state = 10;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 10:
	{
		if (current_character == 'R')
		{
			state = 4;
			myData.ok = 0;
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 11:
	{
		if (current_character == 'E')
			state = 7;
		else if (current_character == 'O')
			state = 3;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 12:
	{
		if (current_character >= 65 && current_character <= 90)
		{
			state = 13;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
				addCharacterToATTextMatrix(current_character);
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 13:
	{
		if (current_character >= 65 && current_character <= 90)
		{
			state = 13;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
				addCharacterToATTextMatrix(current_character);
		}
		else if (current_character == ':')
		{
			state = 14;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
				addCharacterToATTextMatrix(current_character);
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 14:
	{
		if (current_character == ' ')
		{
			state = 15;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
				addCharacterToATTextMatrix(current_character);
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 15:
	{
		if (current_character >= 32 && current_character <= 126)
		{
			state = 16;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
				addCharacterToATTextMatrix(current_character);
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 16:
	{
		if (current_character >= 32 && current_character <= 126)
		{
			state = 16;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
				addCharacterToATTextMatrix(current_character);
		}
		else if (current_character == CR)
		{
			state = 17;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
				addCharacterToATTextMatrix(current_character);
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 17:
	{
		if (current_character == LF)
		{
			state = 18;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
			{
				addCharacterToATTextMatrix(current_character);
				addCharacterToATTextMatrix('\0');
				myData.line_count++;
				myData.column_count = 0;
			}
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 18:
	{
		if (current_character == CR)
			state = 19;
		else if (current_character == '+')
		{
			state = 12;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
			{
				myData.column_count = 0;
				addCharacterToATTextMatrix(current_character);
			}
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 19:
	{
		if (current_character == LF)
			state = 11;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 20:
	{
		if (current_character >= 48 && current_character <= 122)
		{
			state = 20;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
			{
				addCharacterToATTextMatrix(current_character);
			}
		}
		else if (current_character == CR)
		{
			state = 21;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
			{
				addCharacterToATTextMatrix(current_character);
			}
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 21:
	{
		if (current_character == LF)
		{
			state = 22;
			if (myData.column_count < AT_COMMAND_MAX_LINE_SIZE && myData.line_count < AT_COMMAND_MAX_LINES)
			{
				addCharacterToATTextMatrix(current_character);
				addCharacterToATTextMatrix('\0');
				myData.line_count++;
				myData.column_count = 0;
			}
		}
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 22:
	{
		if (current_character == CR)
			state = 23;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 23:
	{
		if (current_character == LF)
			state = 24;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	case 24:
	{
		if (current_character == 'E')
			state = 7;
		else if (current_character == 'O')
			state = 3;
		else
			return STATE_MACHINE_READY_WITH_ERROR;
		break;
	}
	}
	return STATE_MACHINE_NOT_READY;
}
