#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <signal.h>

int fd = 0;
unsigned char mode = 0;
unsigned char lsb = 0; 
char readBuf[2];
char writeBuf[10];
struct spi_ioc_transfer xfer;
int fd_sen = 0;
int i,j;

void sig_handler(int signo)
{
	if (signo == SIGINT)
	{
		close(fd_sen);
		close(fd);
		printf("Application Terminationg.... \n");
		exit(1);
	}
}

void init_gpio()
{
	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, "42", 2);
	close(fd); 
	
	// Set GPIO Direction
	fd = open("/sys/class/gpio/gpio42/direction", O_WRONLY);
	write(fd, "out", 3);
	close(fd);
	
	// Set Value
	fd = open("/sys/class/gpio/gpio42/value", O_WRONLY);
	write(fd, "0", 1);
	close(fd);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, "43", 2);
	close(fd); 
	
	// Set GPIO Direction
	fd = open("/sys/class/gpio/gpio43/direction", O_WRONLY);
	write(fd, "out", 3);
	close(fd);
	
	// Set Value
	fd = open("/sys/class/gpio/gpio43/value", O_WRONLY);
	write(fd, "0", 1);
	close(fd);

	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, "55", 2);
	close(fd); 
	
	// Set GPIO Direction
	fd = open("/sys/class/gpio/gpio55/direction", O_WRONLY);
	write(fd, "out", 3);
	close(fd);
	
	// Set Value
	fd = open("/sys/class/gpio/gpio55/value", O_WRONLY);
	write(fd, "0", 1);
	close(fd);

}

void init_spi()
{
	fd = open("/dev/spidev1.0", O_RDWR);
	mode = SPI_MODE_0;

	if(ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)
	{
		perror("SPI Set Mode Failed");
		close(fd);
		return;
	}
	
	if(ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb) < 0)
	{
		perror("SPI Set LSB Failed");
		close(fd);
		return;
	}

	writeBuf[0] = 0x09; 
	writeBuf[1] = 0x00;
	writeBuf[2] = 0x0a; 
	writeBuf[3] = 0x03;
	writeBuf[4] = 0x0b; 
	writeBuf[5] = 0x07;
	writeBuf[6] = 0x0c; 
	writeBuf[7] = 0x01;
	writeBuf[8] = 0x0f; 
	writeBuf[9] = 0x00; 
	
	// Setup Transaction 
	xfer.tx_buf = (unsigned int)writeBuf;
	xfer.rx_buf = (unsigned int)NULL;
	xfer.len = 10; // Bytes to send
	xfer.cs_change = 0;
	xfer.delay_usecs = 0;
	xfer.speed_hz = 10000000;
	xfer.bits_per_word = 8;
	
	// Send Message
	
	xfer.tx_buf = (unsigned int) (&writeBuf[0]);
	if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
	perror("SPI Message Send Failed");
		
}

void set_spi_rows(int rows)
{
	int j;
	if(rows!=0)
	{
		for(j=1; j<=rows;++j)
		{
			writeBuf[0] = j; 
			writeBuf[1] = 0xFF;
			if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
				perror("SPI Message Send Failed");
		}
		for(j=rows+1; j<=8;++j)
		{
			writeBuf[0] = j; 
			writeBuf[1] = 0x00;
			if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
				perror("SPI Message Send Failed");
		}
	}
	
	else
	{
		for(j=1; j<=8;++j)
		{
			writeBuf[0] = j; 
			writeBuf[1] = (0x01<<(j-1) | (0x80>>(j-1)));
			if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
				perror("SPI Message Send Failed");
		}
		for(j=1; j<=8;++j)
		{
			writeBuf[0] = j; 
			writeBuf[1] = 0x00;
			if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
				perror("SPI Message Send Failed");
		}
	}
}


int main(int argc, char* argv[])
{
	int defaultDist;
	if (argc != 2)
	{
		defaultDist = 5;
	}
	else
	{
		defaultDist = atoi(argv[1]);
	}
	
	printf("Starting Application....\n");
	printf("Safe Distance: %d\n", defaultDist);
	
	if (signal(SIGINT, sig_handler) == SIG_ERR)
	{
  		printf("\ncan't catch SIGINT\n");
  		exit(1);
	}
	
	init_gpio();
	init_spi();
	
	xfer.len = 2; // Bytes to send

	fd_sen = open ("/dev/sensor", O_RDWR);
	if(fd_sen<=0)
	{
		printf("File open failed....\n");
		exit(1);
	}

	while (1)
	{
		read(fd_sen, &i, sizeof(int));
		if(i>defaultDist*8)
		{
			set_spi_rows(8);
		} 
		else if(i>defaultDist*7)
		{	
			set_spi_rows(7);
		} 
		else if(i>defaultDist*6)
		{
			set_spi_rows(6);
		}
		else if(i>defaultDist*5)
		{
			set_spi_rows(5);
		}
		else if(i>defaultDist*4)
		{
			set_spi_rows(4);
		}
		else if(i>defaultDist*3)
		{
			set_spi_rows(3);
		}
		else if(i>defaultDist*2)
		{
			set_spi_rows(2);
		}
		else if(i>defaultDist)
		{
			set_spi_rows(1);
		}
		else
		{
			set_spi_rows(0);
		}
	}
}
