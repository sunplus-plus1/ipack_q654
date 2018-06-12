#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int dump(unsigned int const *addr, const int count)
{
	int i;
	if(NULL == addr||count <=0)
	{
		printf("%s : error para \n",__FUNCTION__);
		return -1;
	}
	
	for(i=0;i<count;i=i+4)
	{
		printf("%08x %08x %08x %08x\n",(unsigned int)addr[i],(unsigned int)addr[i+1],(unsigned int)addr[i+2],(unsigned int)addr[i+3]);
	}
	
	return 0;
}


int main(int argc, char* argv[])
{
	int i,ret;
	int fd,fd_out;
	char filename[128];
	char outputname[128];
	char buf[2112];
	char buf1[2112];
	unsigned int filesize;
	int offset;
	if(argc < 3)
	{
		printf("\nUsage: merge nand.raw output.bin \n");
	}
	else if(argc == 3)
	{
		strcpy(filename,argv[1]);
		strcpy(outputname,argv[2]);
		
		printf("file name = %s \n",filename);
		printf("output file name = %s \n",outputname);
	}
	
	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		printf("can't open the file %s",filename);
		exit(1);
	}

	fd_out = open(outputname, O_RDWR|O_CREAT, 0644);
	if(fd < 0)
	{
		printf("can't open the file %s",outputname);
		exit(1);
	}
	
	lseek(fd,0,SEEK_SET);
	filesize = lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
	
	printf("filesize =  %d \n",filesize);

	memset((void *)buf, 0xc3, 2112);
/*
	ret = read(fd,buf,2112);
	printf("ret=0x%x\n",ret);

	dump((unsigned int *)buf,16);	
*/		
	
	printf("\n pack start \n");
	memset((void *)buf1, 0xff, 2112);
	offset = 0;
	while(offset < filesize)
	{
		read(fd,buf,2112);
		//dump((unsigned int *)buf,16);
		write(fd_out,buf,2112);
		write(fd_out,buf1,1984);
		offset += 2112;
		//printf(".");
	}

	printf("\n pack OK \n");
	close(fd);
	fsync(fd_out);
	close(fd_out);
	
	return 0;
}
