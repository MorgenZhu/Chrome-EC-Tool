#include <stdlib.h>
#include <stdio.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/rtc.h>


//=======================Data type define ===========================
#define FALSE 0
#define TRUE  1

typedef unsigned char UINT8;
typedef unsigned int  UINT32;
//===================================================================


//=======================Port define ================================
#define IOPORT_PATH "/dev/port"

UINT8 ACPI_PM1_STATUS_PORT = 0x66;
UINT8 ACPI_PM1_CMD_PORT    = 0x66;
UINT8 ACPI_PM1_DATA_PORT   = 0x62;

#define OBF_IBE_CHECK  0
#define PM_OBF  0x01
#define PM_IBE  0x02
//===================================================================


//=======================Global data=================================
FILE *f_Port=NULL;
//===================================================================


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

void main(int argc, char *argv[])
{
	int ret;
	int i;
	char *e;
	UINT8 address, read_buf, write_buf;
	
	if(0 == device_open())
	{
		printf("open port file error!\n");
		goto error;
	}

	if(NULL!=argv[1])
	{
		if(!strcasecmp(argv[1], "/r"))
		{
			if(NULL!=argv[2])
			{
				address = strtol(argv[2], &e, 0);
				if(e && *e)
				{
					printf("index is error\n");
					goto error;
				}
			}
			else
			{
				printf("please input read index\n");
				goto error;
			}

			ret = Read_EC_RAM_BYTE(address, &read_buf);
			printf("Read %s, Addr(0x%X)=0x%X\n",(ret)?"OK":"Fail", address, read_buf);
			goto end;
		}
		else if(!strcasecmp(argv[1], "/w"))
		{
			if(NULL!=argv[2])
			{
				address = strtol(argv[2], &e, 0);
				if(e && *e)
				{
					printf("index is error\n");
					goto error;
				}
			}
			else
			{
				printf("please input write index\n");
				goto error;
			}

			if(NULL!=argv[3])
			{
				write_buf = strtol(argv[3], &e, 0);
				if(e && *e)
				{
					printf("data is error\n");
					goto error;
				}
			}
			else
			{
				printf("please input write date\n");
				goto error;
			}

			ret = Write_EC_RAM_BYTE(address, write_buf);
			printf("Write %s, Addr(0x%X)=0x%X\n", (ret)?"OK":"Fail", address, write_buf);
			goto end;
		}
		goto error;
	}
	goto error;

error:
	printf("Usage:\n");
	printf("1. IOtest /r index\n");
	printf("2. IOtest /w index data\n");
end:	
	device_close();
	return ;
}
