#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "LPC177x_8x.h"
#include "system_LPC177x_8x.h"
#include <retarget.h>
#include <DRV\drv_sdram.h>
#include <DRV\drv_lcd.h>
#include <DRV\drv_uart.h>
#include <DRV\drv_touchscreen.h>
#include <DRV\drv_led.h>
#include <utils\timer_software.h>
#include <utils\timer_software_init.h>
#include "parser.h"

#define CR 0x0D
#define LF 0x0A

int current_command = 0;
enum commands {csq, creg, cops, gsn, gmi, gmr};
BOOLEAN ready;
BOOLEAN ready_ok;
BOOLEAN LCD_pressed = FALSE;
int pressed_X;
int pressed_Y;
int message_x = LCD_HEIGHT / 10;
int message_y = LCD_HEIGHT / 10;
int message2_x = LCD_HEIGHT / 10;
int message2_y = LCD_HEIGHT / 10 + 70;
int button_width = LCD_WIDTH / 3;
int button_height = LCD_HEIGHT / 2 - 50;
int button_x1 = 30;
int button_x2 = LCD_WIDTH / 2 + 30;
int button_y = LCD_HEIGHT / 2 + 20;
char extracted_string[30];
const char at_command_simple[] = "AT\r\n";
const char at_command_csq[] = "AT+CSQ\r\n";
const char at_command_creg[] = "AT+CREG?\r\n";
const char at_command_cops[] = "AT+COPS?\r\n";
const char at_command_gsn[] = "AT+GSN\r\n";
const char at_command_gmi[] = "AT+GMI\r\n";
const char at_command_gmr[] = "AT+GMR\r\n";
int color_red[] = {255, 0, 0};
int color_cream[] = {255, 253, 208};
int color_orange[] = {255, 165, 0};
LCD_PIXEL foreground = {0, 0, 0, 0};
LCD_PIXEL background = {255, 253, 208, 0};
char cregMessage[100];
char copsMessage[100];

timer_software_handler_t my_timer_handler;
timer_software_handler_t my_handler;

void timer_callback_1(timer_software_handler_t h)
{
}

void drawRectangle(int height, int width, int x, int y, int color[])
{
	uint32_t i,j;
	for (i = y; i < height + y; i++)
	{
		for (j = x; j < width + x; j++)
		{
			DRV_LCD_PutPixel(i, j, color[0], color[1], color[2]);
		}
	}
}

void drawLCD()
{
	drawRectangle(LCD_HEIGHT, LCD_WIDTH, 0, 0, color_orange);
	drawRectangle(LCD_HEIGHT / 2, LCD_WIDTH, 0, 0, color_cream);
	
	// prev
	drawRectangle(button_height, button_width, button_x1, button_y, color_cream);
	// next
	drawRectangle(button_height, button_width, button_x2, button_y, color_cream);
	
	DRV_LCD_Puts("Previous", button_width - 100, button_y + 30, foreground, background, TRUE);
	DRV_LCD_Puts("Next", LCD_WIDTH / 2 + button_width - 80, button_y + 30, foreground, background, TRUE);
}

void TouchScreenCallBack(TouchResult* touchData)
{
	LCD_pressed = true;
	pressed_X = touchData->X;
  pressed_Y = touchData->Y;
}

char* switchCOPS(const char *command)
{
	int mode = command[7] - 48;
	char aux[100];
	int i;
	int start_index;
	uint8_t count = 0;
	
	if ((mode == 0) && (strlen(command) == 9))
	{
		strcpy(copsMessage, "No operator");
	}
	else
	{
		for (i = 0; i < strlen(command); i++)
		{
			if (command[i] == ',')
			{
				count++;
				if (count == 2)
				{
					start_index = i + 2;
					break;
				}
		  }
    }
		i = start_index;
		while (command[i] != '”' && i < strlen(command))
		{
			aux[i - start_index] = command[i];
			i++;
		}
		aux[i - start_index] = '\0';
		strcpy(copsMessage, aux);
	}
	return copsMessage;
}

char* switchCREG(const char *command)
{
	int status = command[7] - 48;
	switch(status)
	{
		case 0: strcpy(cregMessage, "Not registered, MT is not currently searching a new operator to register to");
		        break;
		case 1: strcpy(cregMessage, "Registered, home network");
		        break;
		case 2: strcpy(cregMessage, "Not registered, but MT is currently searching a new operator to register to");
		        break;
		case 3: strcpy(cregMessage, "Registration denied");
		        break;
		case 4: strcpy(cregMessage, "Unknown");
		        break;
		case 5: strcpy(cregMessage, "Registered, roaming");
		        break;
		default: break;
	}
	return cregMessage;
}

void BoardInit()
{
	timer_software_handler_t handler;
	
	TIMER_SOFTWARE_init_system();
	
	DRV_SDRAM_Init();
	
	initRetargetDebugSystem();
	DRV_LCD_Init();
	DRV_LCD_ClrScr();
	DRV_LCD_PowerOn();	
	
	DRV_TOUCHSCREEN_Init();
	DRV_TOUCHSCREEN_SetTouchCallback(TouchScreenCallBack);
	DRV_LED_Init();
	printf ("Hello\n");
}

void SendCommand(const char *command)
{
	DRV_UART_FlushRX(UART_3);
	DRV_UART_FlushTX(UART_3);
	DRV_UART_Write(UART_3, (uint8_t *)command, strlen(command));
}

void GetCommandResponse(uint8_t flag)
{
	uint8_t ch;
	STATE_MACHINE_RETURN_VALUE state_return;
	ready = FALSE;
	
	// my_handler trebuie configurat
	my_handler = TIMER_SOFTWARE_request_timer(); // request a timer
	if (my_handler < 0) // check if the request was successful
	{
		// the system could not offer a software timer
	}
	TIMER_SOFTWARE_configure_timer(my_handler, MODE_1, 20000, true);
	// set a callback for the requested timer
	TIMER_SOFTWARE_start_timer(my_handler);
	
	while ((!TIMER_SOFTWARE_interrupt_pending(my_handler)) && (ready == FALSE))
	{
		while (DRV_UART_BytesAvailable(UART_3) > 0)
		{
			DRV_UART_ReadByte(UART_3, &ch);
			TIMER_SOFTWARE_Wait(5);
			state_return = at_command_parse(ch, flag);
			if (state_return != STATE_MACHINE_NOT_READY)
			{
				ready = TRUE;
				if (state_return == STATE_MACHINE_READY_OK)
				{
					ready_ok = TRUE;
				}
			}
		}
	}
}

uint32_t convert(char *s) 
{ 
  // Initialize a variable
	int i = 0;	
  int num = 0;
	int length = 0;
	while (s[length] != '\0')
	{
		length++;
	}
  
  // Iterate till length of the string 
  for (i = 0; i < length; i++) 
		num = num * 10 + (s[i] - 48); 
  
	return num;
} 

uint32_t ExtractData()
{
	int line = 0;
	int column = 0;
	int index_i = 0;
	int index_f = 0;
	char int_num[4];
	char float_num[4];
	column = 0;
	
	while (myData.data[line][column] != ' ')
	{
		column++;
	}
	column++;    // first decimal character
	
	while (myData.data[line][column] != ',')
	{
		int_num[index_i] = myData.data[line][column];
		column++;
		index_i++;
	}
	
	int_num[index_i] = '\0';    // add end of string character
	index_i++;
	column++;   // first floating point character
	
	return convert(int_num);
}

char* ExtractString()
{
	int line = 0;
	int column = 0;
	int i = 0;
	while (myData.data[line][column] != LF)
	{
		extracted_string[i] = myData.data[line][column];
		column++;
		i++;
	}
	column++;    // first LF character
	extracted_string[i] = '\0';    // add end of string character
	i++;
	
	return extracted_string;
}

void ExecuteCommand(const char *command, uint8_t flag)
{
	SendCommand(command);
	GetCommandResponse(flag);
}

int32_t ConvertAsuToDbmw(uint32_t rssi_value_asu)
{
	return 2 * rssi_value_asu - 113;
}

int main(void)
{
	uint8_t ch;
	uint32_t rssi_value_asu;
	uint32_t rssi_value_dbmw;
	
	char message_on_LCD[200];
	char current_string[50];
	char cops_vodafone_string[50] = "+COPS: 0,0,”Vodafone RO”,2";

	timer_software_handler_t handler; // declare a software timer handler
	
	BoardInit();
	drawLCD();
	
	my_timer_handler = TIMER_SOFTWARE_request_timer(); // request a timer
	if (my_timer_handler < 0) // check if the request was successful
	{
		// the system could not offer a software timer
	}
	TIMER_SOFTWARE_configure_timer(my_timer_handler, MODE_1, 5000, true);
	// set a callback for the requested timer
	
	DRV_UART_Configure(UART_3, UART_CHARACTER_LENGTH_8, 115200, UART_PARITY_NO_PARITY, 1, TRUE, 3);
	DRV_UART_Configure(UART_2, UART_CHARACTER_LENGTH_8, 115200, UART_PARITY_NO_PARITY, 1, TRUE, 3);
	
	DRV_UART_Write(UART_3, (uint8_t *)at_command_simple, strlen(at_command_simple));
	TIMER_SOFTWARE_Wait(1000);
	
	DRV_UART_Write(UART_3, (uint8_t *)at_command_simple, strlen(at_command_simple));
	TIMER_SOFTWARE_Wait(1000);
	
	DRV_UART_Write(UART_3, (uint8_t *)at_command_simple, strlen(at_command_simple));
	TIMER_SOFTWARE_Wait(1000);

	//TIMER_SOFTWARE_start_timer(my_timer_handler);
	
	DRV_LCD_Puts("AT+CSQ", message_x, message_y, foreground, background, TRUE);
	ExecuteCommand(at_command_csq, 0);
	if (ready_ok)
	{
		rssi_value_asu = ExtractData();
		rssi_value_dbmw = ConvertAsuToDbmw(rssi_value_asu);
		sprintf(message_on_LCD, "\nGSM Modem signal %d ASU -> %d\n", rssi_value_asu, rssi_value_dbmw);
		DRV_LCD_Puts(message_on_LCD, message2_x, message2_y, foreground, background, FALSE);
	}
	DRV_TOUCHSCREEN_Process();
	
	while (1)
	{
		if (LCD_pressed)
		{
			// prev button
			if (pressed_X >= button_x1 &&
		    pressed_X <= button_x1 + button_width &&
	      pressed_Y >= button_y &&
	      pressed_Y <= button_y + button_height)
	    {
		    current_command = (current_command + 7 - 1) % 7;
	    }
			// next button
	    else if (pressed_X >= button_x2 &&
		    pressed_X <= button_x2 + button_width &&
	      pressed_Y >= button_y &&
	      pressed_Y <= button_y + button_height)
	    {
	    	current_command = (current_command + 1) % 7;
	    }
			drawRectangle(LCD_HEIGHT / 2, LCD_WIDTH, 0, 0, color_cream);    // clears written message
			if (current_command == 0)
			{
				DRV_LCD_Puts("AT+CSQ", message_x, message_y, foreground, background, TRUE);
				ExecuteCommand(at_command_csq, 0);
			  if (ready_ok)
			  {
				  rssi_value_asu = ExtractData();
				  rssi_value_dbmw = ConvertAsuToDbmw(rssi_value_asu);
					sprintf(message_on_LCD, "\nGSM Modem signal %d ASU -> %d\n", rssi_value_asu, rssi_value_dbmw);
					DRV_LCD_Puts(message_on_LCD, message2_x, message2_y, foreground, background, FALSE);
			  }
		  }
			else if (current_command == 1)
			{
				DRV_LCD_Puts("AT+CREG?", message_x, message_y, foreground, background, TRUE);
				ExecuteCommand(at_command_creg, 0);
			  if (ready_ok)
			  {
				  strcpy(current_string, ExtractString());
					sprintf(message_on_LCD, "\nState of registration: %s\n", switchCREG(current_string));
					DRV_LCD_Puts(message_on_LCD, message2_x, message2_y, foreground, background, FALSE);
			  }
		  }
			else if (current_command == 2)
			{
				DRV_LCD_Puts("AT+COPS?", message_x, message_y, foreground, background, TRUE);
				ExecuteCommand(at_command_cops, 0);
			  if (ready_ok)
			  {
				  strcpy(current_string, ExtractString());
					sprintf(message_on_LCD, "\nOperator name: %s\n", switchCOPS(current_string));
					DRV_LCD_Puts(message_on_LCD, message2_x, message2_y, foreground, background, FALSE);
			  }
		  }
			else if (current_command == 4)
			{
				DRV_LCD_Puts("AT+GSN", message_x, message_y, foreground, background, TRUE);
			  ExecuteCommand(at_command_gsn, 1);
			  if (ready_ok)
			  {
				  strcpy(current_string, ExtractString());
					sprintf(message_on_LCD, "\nModem IMEI: %s\n", current_string);
					DRV_LCD_Puts(message_on_LCD, message2_x, message2_y, foreground, background, FALSE);
			  }
		  }
			else if (current_command == 5)
			{
				DRV_LCD_Puts("AT+GMI", message_x, message_y, foreground, background, TRUE);
				ExecuteCommand(at_command_gmi, 1);
			  if (ready_ok)
			  {
				  strcpy(current_string, ExtractString());
					sprintf(message_on_LCD, "\nModem manufacturer: %s\n", current_string);
					DRV_LCD_Puts(message_on_LCD, message2_x, message2_y, foreground, background, FALSE);
			  }
			}
			else if (current_command == 6)
			{
				DRV_LCD_Puts("AT+GMR", message_x, message_y, foreground, background, TRUE);
				ExecuteCommand(at_command_gmr, 1);
			  if (ready_ok)
			  {
				  strcpy(current_string, ExtractString());				
					sprintf(message_on_LCD, "\nModem software version: %s\n", current_string);
					DRV_LCD_Puts(message_on_LCD, message2_x, message2_y, foreground, background, FALSE);
			  }
			}
			LCD_pressed = FALSE;
			TIMER_SOFTWARE_Wait(100);
		}
		DRV_TOUCHSCREEN_Process();
	} 
	/*
	while(1)
	{
		DRV_UART_SendByte(UART_3, 'A');
	//	TIMER_SOFTWARE_Wait(1000);
	}
	*/
	/*
	while(1)
	{
		if (DRV_UART_ReadByte(UART_3, &ch) == OK)
		{
			DRV_UART_SendByte(UART_3, ch);
		}		
	}
*/
	while(1)
	{
		if (DRV_UART_ReadByte(UART_0, &ch) == OK)
		{
			DRV_UART_SendByte(UART_3, ch);
		}
		if (DRV_UART_ReadByte(UART_3, &ch) == OK)
		{
			DRV_UART_SendByte(UART_0, ch);
		}
		if (DRV_UART_ReadByte(UART_2, &ch) == OK)
		{
			DRV_UART_SendByte(UART_0, ch);
		}
	}
	
	while(1)
	{
		DRV_UART_Process();
		DRV_TOUCHSCREEN_Process();
	}
	
	return 0; 
}
