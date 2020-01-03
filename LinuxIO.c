#include <stdlib.h>
#include <stdio.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/rtc.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>


//=======================Data type define ===========================
#define FALSE 0
#define TRUE  1

typedef unsigned char UINT8;
typedef unsigned int  UINT32;
//===================================================================


//=======================Port define ================================
#define IOPORT_PATH "/dev/port"

UINT32 ACPI_PM1_STATUS_PORT = 0x66;
UINT32 ACPI_PM1_CMD_PORT    = 0x66;
UINT32 ACPI_PM1_DATA_PORT   = 0x62;

#define OBF_IBE_CHECK  0
#define PM_OBF  0x01
#define PM_IBE  0x02
//===================================================================


//=======================Global data=================================
FILE *f_Port=NULL;
UINT8  Page_256Byte[256];
UINT32 Page_Index  = 9;
UINT8  Page_Data_x = 7;
UINT8  Page_Data_y = 7;
#define KEY_BOARD_SUPPORT  1
//===================================================================

//===================================================================
#define CLEAR_ALL()            printf("\033[2J");
#define CURSOR_MOVE_U(x)       printf("\033[%dA", x);
#define CURSOR_MOVE_D(x)       printf("\033[%dB", x);
#define CURSOR_MOVE_L(x)       printf("\033[%dD", x);
#define CURSOR_MOVE_R(x)       printf("\033[%dC", x);
#define CURSOR_MOVE_R(x)       printf("\033[%dC", x);
#define CURSOR_MOVE(x,y)       printf("\033[%d;%dH", x,y);
#define CURSOR_HIDE(x)         printf("\033[?25l");

//===================================================================
#if KEY_BOARD_SUPPORT
#define key_ESC      0x1B
#define key_ENTER    0x0A
#define key_TABLE    0x09
#define key_Decrease 0x2D
#define key_Increase 0x3D
#define key_0       0x30
#define key_1       0x31
#define key_2       0x32
#define key_3       0x33
#define key_4       0x34
#define key_5       0x35
#define key_6       0x36
#define key_7       0x37
#define key_8       0x38
#define key_9       0x39
#define key_A       0x41
#define key_B       0x42
#define key_C       0x43
#define key_D       0x44
#define key_E       0x45
#define key_F       0x46
#define key_a       0x61
#define key_b       0x62
#define key_c       0x63
#define key_d       0x64
#define key_e       0x65
#define key_f       0x66

static struct termios initial_settings, new_settings;
static int peek_character = -1;

int kbhit(void)
{
    char ch;
    int nread;

    if ( peek_character != -1 )
        return(1);

    new_settings.c_cc[VMIN] = 0;
    tcsetattr( 0, TCSANOW, &new_settings );
    nread = read( 0, &ch, 1 );
    new_settings.c_cc[VMIN] = 1;
    tcsetattr( 0, TCSANOW, &new_settings );
    if ( nread == 1 )
    {
        peek_character = ch;
        return(1);
    }
    return(0);
}

int readch(void)
{
    char ch;

    if ( peek_character != -1 )
    {
        ch = peek_character;
        peek_character = -1;
        return(ch);
    }
	
    read( 0, &ch, 1 );
    return(ch);
}

void init_keyboard(void)
{
    tcgetattr( 0, &initial_settings );
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr( 0, TCSANOW, &new_settings );
}

void close_keyboard(void)
{
    tcsetattr( 0, TCSANOW, &initial_settings );
}
#endif
UINT8 device_open(void)
{
	FILE *fd;

	fd = fopen(IOPORT_PATH, "rb+");
	if(fd <= 0)
	{
		f_Port = NULL;
		printf("device_init : open IOPORT file error!\n");
		return 0;
	}
	
	f_Port = fd;
	return 1;
}

void device_close(void)
{
	fclose(f_Port);
}

UINT8 io_write(int offset, UINT8 data)
{
	int ret;
	UINT8 *p;

	ret = fseek(f_Port, offset , SEEK_SET);
	if(0 != ret)
	{
		printf("iowrite : IO port error 1\n");
		return 0;
	}

	p = (UINT8 *)&data;
	ret = fwrite(p, 1, 1, f_Port);
	if(0 > ret)
	{
		printf("iowrite : IO port error 2\n");
		return 0;
	}
	
	return 1;
}

UINT8 io_read(int offset, UINT8 *data)
{
	int ret;
	
	ret = fseek(f_Port, offset , SEEK_SET);
	if(0 != ret)
	{
		printf("ioread : IO port error 1\n");
		return 0;
	}

	ret = fread(data, 1, 1, f_Port);
	if(1 != ret)
	{
		printf("ioread : IO port error 2\n");
		return 0;
	}
	return 1;
}

#if OBF_IBE_CHECK
UINT8 wait_pm_obf(void)
{
	int count=500;
	UINT8 read_buf;
	do{
		read_buf = io_read(ACPI_PM1_STATUS_PORT);
		if(read_buf&PM_OBF)
		{
			return 1;
		}
		usleep(100);
	}while(count--);

	printf("wait obf : timeout, read_buf=%X\n",read_buf);
	return 0;
}

UINT8 wait_pm_ibe(void)
{
	int count=500;
	UINT8 read_buf;
	do{
		read_buf = io_read(ACPI_PM1_STATUS_PORT);
		if(!(read_buf&PM_IBE))
		{
			return 1;
		}
		usleep(100);
	}while(count--);

	printf("wait ibe : timeout\n");
	return 0;
}
#endif

UINT8 sned_cmd_to_pm(UINT8 cmd)
{
	#if OBF_IBE_CHECK
	if(wait_pm_ibe())
	{
		if(io_write(ACPI_PM1_CMD_PORT, cmd))
		{
			if(wait_pm_ibe())
			{
				return 1;
			}
		}
	}
	return 0;
	#else
	if(io_write(ACPI_PM1_CMD_PORT, cmd))
	{
		return 1;
	}
	return 0;
	#endif
}

UINT8 sned_data_to_pm(UINT8 data)
{
	#if OBF_IBE_CHECK
	if(wait_pm_ibe())
	{
		if(io_write(ACPI_PM1_DATA_PORT, data))
		{
			if(wait_pm_ibe())
			{
				return 1;
			}
		}
	}
	return 0;
	#else
	if(io_write(ACPI_PM1_DATA_PORT, data))
	{
		return 1;
	}
	return 0;
	#endif
}

UINT8 read_data_from_pm(UINT8 *data)
{
	#if OBF_IBE_CHECK
	if(wait_pm_obf())
	{
		if(io_read(ACPI_PM1_DATA_PORT, data))
		{
			return 1;
		}
	}
	return 0;
	#else
	if(io_read(ACPI_PM1_DATA_PORT, data))
	{
		return 1;
	}
	return 0;
	#endif
}

UINT8 Read_EC_RAM_BYTE(UINT8 index, UINT8 *data)
{
	if(sned_cmd_to_pm(0x80))
	{
		//usleep(100);
		if(sned_data_to_pm(index))
		{
			//usleep(100);
			if(read_data_from_pm(data))
			{
				return 1;
			}
		}
	}
	return 0;
}

UINT8 Write_EC_RAM_BYTE(UINT8 index, UINT8 data)
{
	if(sned_cmd_to_pm(0x81))
	{
		//usleep(100);
		if(sned_data_to_pm(index))
		{
			//usleep(100);
			if(sned_data_to_pm(data))
			{
				return 1;
			}
		}
	}
	return 0;
}

UINT8 Read_BYTE(char **argv)
{
	int ret;
	char *e;
	UINT8 address, read_buf;

	if(NULL!=argv[2])
	{
		address = strtol(argv[2], &e, 0);
		if(e && *e)
		{
			printf("index is error\n");
			return 0;
		}
	}
	else
	{
		printf("please input read index\n");
		return 0;
	}

	ret = Read_EC_RAM_BYTE(address, &read_buf);
	printf("Read %s, Addr(0x%X)=0x%X\n",(ret)?"OK":"Fail", address, read_buf);
	return ret;
}

UINT8 Write_BYTE(char **argv)
{
	int ret;
	char *e;
	UINT8 address, write_buf;

	if(NULL!=argv[2])
	{
		address = strtol(argv[2], &e, 0);
		if(e && *e)
		{
			printf("index is error\n");
			return 0;
		}
	}
	else
	{
		printf("please input write index\n");
		return 0;
	}

	if(NULL!=argv[3])
	{
		write_buf = strtol(argv[3], &e, 0);
		if(e && *e)
		{
			printf("data is error\n");
			return 0;
		}
	}
	else
	{
		printf("please input write date\n");
		return 0;
	}

	ret = Write_EC_RAM_BYTE(address, write_buf);
	printf("Write %s, Addr(0x%X)=0x%X\n", (ret)?"OK":"Fail", address, write_buf);
	return ret;
}

UINT8 Read_256Byte(void)
{
	int ret;
	
	ret = fseek(f_Port, Page_Index*0x100 , SEEK_SET);
	if(0 != ret)
	{
		//printf("ioread : IO port error 1\n");
		return 0;
	}

	ret = fread(Page_256Byte, 256, 1, f_Port);
	if(1 != ret)
	{
		//printf("ioread : IO port error 2\n");
		return 0;
	}
	return 1;
}

#define TOOL_UI_X  6
#define TOOL_UI_Y  4
void Display_256Byte(void)
{
	int i,j;
	time_t current_time;
	UINT8 time_str[60];

	printf("\033[%d;%dHIO_Addr=0x%04X", 
		TOOL_UI_Y, TOOL_UI_X+5, Page_Index*0x100);

	printf("\033[%d;%dH\033[31m%X%X\033[0m", 
		TOOL_UI_Y+2, TOOL_UI_X, Page_Data_y,Page_Data_x);

	printf("\033[%d;%dH00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F",
		TOOL_UI_Y+2, TOOL_UI_X+5);

	for(i=0; i<16; i++)
	{
		printf("\033[%d;%dH", i+TOOL_UI_Y+4, TOOL_UI_X);
		printf("%02X   ", i);
		for(j=0; j<16; j++)
		{
			printf("%02X ", Page_256Byte[i*16+j]);
			if((i==Page_Data_y) && (j==Page_Data_x))
			{
				printf("\033[3D\033[31m%02X \033[0m", Page_256Byte[i*16+j]);
			}
		}
	}

	for(i=0; i<16; i++)
	{
		printf("\033[%d;%dH |", i+TOOL_UI_Y+4, TOOL_UI_X+56);
		for(j=0; j<16; j++)
		{
			if(isprint(Page_256Byte[i*16+j]))
			{
				printf("%c", Page_256Byte[i*16+j]);
				if((i==Page_Data_y) && (j==Page_Data_x))
				{
					printf("\033[3D\033[31m%c\033[0m", Page_256Byte[i*16+j]);
				}
			}
			else
			{
				printf(".");
				if((i==Page_Data_y) && (j==Page_Data_x))
				{
					printf("\033[3D\033[31m.\033[0m");
				}
			}
		}
		printf("|");
	}

	current_time = time(0);
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %X", localtime(&current_time));
	printf("\033[%d;%dH%s", TOOL_UI_Y+21, TOOL_UI_X+5, "Morgen@Chromium");
	printf("\033[%d;%dH%s\n", TOOL_UI_Y+21, TOOL_UI_X+33, time_str);
}

void Polling_IO_Page(void)
{
	char key_code;
	while(1)
	{
		if(Read_256Byte())
		{
			Display_256Byte();
		}

		// Delay 500ms
		usleep(500000);
		if (kbhit())
		{
			key_code = readch();
			//printf("\e[%d;%dH%X\n",1, 5, key_code);
		}

		if(key_ESC == key_code)
		{
			break;
		}
		else if(key_Increase == key_code)
		{
			Page_Index++;
			if(Page_Index>0xFF)
			{
				Page_Index=0;
			}
		}
		else if(key_Decrease == key_code)
		{
			if(Page_Index>0)
			{
				Page_Index--;
			}
		}
		usleep(500000);
		key_code = 0;
	}
}


void main(int argc, char *argv[])
{
	if(0 == device_open())
	{
		printf("open port file error!\n");
		goto error;
	}

	if(NULL!=argv[1])
	{
		if(!strcasecmp(argv[1], "/r"))
		{
			if(Read_BYTE(argv))
			{
				goto end;
			}
		}
		else if(!strcasecmp(argv[1], "/w"))
		{
			if(Write_BYTE(argv))
			{
				goto end;
			}
		}
		else if(!strcasecmp(argv[1], "/p"))
		{
			CLEAR_ALL();
			CURSOR_HIDE();
			init_keyboard();
			Polling_IO_Page();
			close_keyboard();
			goto end;
		}
		goto error;
	}
	goto error;

error:
	printf("Usage:\n");
	printf("1. IOtest /r index\n");
	printf("2. IOtest /w index data\n");
	printf("3. IOtest /p\n");
end:	
	device_close();
	return ;
}
