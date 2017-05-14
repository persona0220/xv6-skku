#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv){
	int pid;

	setnice(1, 40);

	setnice(getpid(), 20);

	pid = fork();

	if(pid==0){
		printf(1, "2\n");
	}
	else{
		setnice(pid, 30);
		printf(1, "1\n");
		wait();
		printf(1, "3\n");
	}

	exit();
}
