/***************************************************************************//**
*  \file       test_ioctl.c
*
*  \details    Userspace application to test the Device drive(ioctl.c)
*
*  \author     EmbeTronicX/andjelas
*
*  \Tested with Linux raspberrypi 6.12.47+rpt-rpi-v7
*
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>

#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)

int main()
{
        int fd ;
        printf("*********************************\n");
        printf("*******WWW.EmbeTronicX.com*******\n");
 
        printf("\nOpening Driver\n");
		fd = open("/dev/etx_device", O_RDWR);
        
		if(fd < 0) {
                perror("Cannot open device"); 
				return 1; 
        }

		
		int32_t cells[9];
		int32_t novi_centar;		
		
		
        printf("Enter 9 cell states (0=mrtva, 1=ziva) row-wise\n");
		for(int i=0; i<9; ++i)
		{
			if (scanf("%d", &cells[i]) != 1 || (cells[i] != 0 && cells[i] != 1)) 
			{
				fprintf(stderr, "Invalid input\n");
				close(fd);
				return 1;
			}
		}
			
		
		if(ioctl(fd, WR_VALUE, cells) < 0)
		{
			perror("Ioctl write failed");		
			close(fd);
			return 1;
		}
		
        if(ioctl(fd, RD_VALUE,&novi_centar)<0)
		{
			if(errno==ENODATA)
				fprintf(stderr, "Driver: no data written before read\n");
			else 
				perror("ioctl read failed");
			close(fd);
			return 1;
		}
		
		printf("New state of central cell: %d\n", novi_centar);
		
        printf("Closing Driver\n");
        close(fd);
		return 0;
}
