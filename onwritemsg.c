#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>  
#include <sys/un.h>

static int OnWriteMsg(char *path,  char* cmd)
{
	int iRet = -1;
	int iConnect_fd = -1;
	int flags = -1;
	struct sockaddr_un srv_addr;

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, path);

	iConnect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(iConnect_fd<0)
	{

		return 1;
	}

	flags = fcntl(iConnect_fd, F_GETFL, 0);
	fcntl(iConnect_fd, F_SETFL, flags|O_NONBLOCK);

	if(0 != connect(iConnect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		close(iConnect_fd);
		return 1;
	}

	iRet = write(iConnect_fd, cmd, strlen(cmd));

	if (iRet != strlen(cmd))
	{
		iRet = -1;
	}
	else
	{
		iRet = 0;
	}

	close(iConnect_fd);
	
	return iRet;
}

void print_usage()
{
	
printf( "MpcVolumeEvent\n"
		"PlayDone\n"
		"MpcPlayEvent\n"
		"MpcStopEvent\n"
		"MpcPrevEvent\n"
		"MpcNextEvent\n"
		"MpcSongChange\n");
}


int main(int argc,char **argv)
{
	if(argc < 2)
	{
		print_usage();
		return -1;
	}
	OnWriteMsg("/tmp/UNIX.turing",argv[1]);
	
	return 0;
}
