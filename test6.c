#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv){
	int pid;
	int pid2;

	setnice(1, 40);

	setnice(getpid(), 20);

	pid = fork();

	if(pid == 0){
		setnice(getpid(), 30);
		printf(2, "2\n");
	}
	else{
		printf(2, "1\n");
		pid2 = fork();	//20

		if(pid2 == 0){
			setnice(getpid(), 35);
			printf(2, "3\n");
		}
		else{
			setnice(getpid(), 38);		
			printf(2, "4\n");
		
		}
		
	}


	
	exit();
}
