#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define BUF_SIZE 16

//Parent process reads from an input file and sends characters to the child process
//Child process writes the characters to an output file
int main(int argc, char *argv[])
{

	int fd[2], i, status;
	pid_t pid;	
	char buffer[BUF_SIZE];
	FILE *fp;
	if(argc != 3)
	{
		printf("usage: copy input_file output_file\n");
		exit(1);
	}

	//Create a pipe with the pipe call.
	if (pipe(fd)<0)
	{
		perror("Failed to create pipe");
	}

	//Fork a new process
	pid = fork();

		
	//Child process
	if (pid == 0)
	{
		//Close 'write' file descriptor of pipe
		close(fd[1]);

		//Open output file to which data will be copied
		fp = fopen(argv[2], "w");

		//Read in some characters from parent through the pipe
		while ((i = read(fd[0], buffer, BUF_SIZE)) != 0)
		{
			//Write i bytes to the output file
			fwrite(buffer, 1, i, fp);

			//Display in the console the number of chars written to the output file
			printf("Child: %d chars are written to the output file\n", i);
		}

		//Close output file
		close(fp);

		//Close 'read' file descriptor of pipe
		close(fd[0]);

		exit(0);
	}
	//Parent process
	else if(pid > 0)
	{	
		//Close 'read' file descriptor of pipe
		close(fd[0]);

		//Open input file using fopen
		fp = fopen(argv[1], "r");

		while ((i= fread(buffer,sizeof(char),BUF_SIZE,fp))!=0)
		{
			//Display in the console the number of chars read from the input file
			printf("Parent: %d chars are read from the input file\n",i);

			//Write i bytes of data in the buffer to the child through the pipe
			write(fd[1], buffer, i);

		}

		//Close input file
		close(fp);

		//Close 'write' file descriptor of pipe
		close(fd[1]);

		//Wait for child to finish
		wait(&status);

		return(0);
	}
	else
	{
		perror("failed to fork:");
		exit(1);
	}
}
