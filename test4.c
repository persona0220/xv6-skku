#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv){
	int pid;

	setnice(1, 40);

	setnice(getpid(), 0);

	pid = fork();

	if(pid==0){
		printf(1, "2\n");
		yield();
		printf(1, "4\n");
	}
	else{
//		setnice(pid, 10);
		printf(1, "1\n");
		yield();
		//setnice(pid, 10);
		printf(1, "3\n");
	}

	exit();
}
