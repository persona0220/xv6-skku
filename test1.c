#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv){
	int pid;
	int pid2;

	setnice(1, 40);

	setnice(getpid(), 20);

	pid = fork();

	if(pid==0){
		printf(1, "4\n");
	}
	else{
		setnice(pid, 30);
		printf(1, "1\n");

		pid2 = fork();

		if(pid2==0){
			printf(1, "3\n");
		}
		else{
			setnice(pid2, 25);
			printf(1, "2\n");
		}
	}

	exit();
}
