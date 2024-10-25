#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_LINE_LENGTH 1024

char** read_crumbs(size_t* count) {
	FILE* f=fopen("~/.breadcrumbs","r");
	if(!f) {
		printf("No set breadcrumb\n");
	}

	char** paths=NULL;
	char buf[MAX_LINE_LENGTH];
	*count=0;
	return paths;
}

typedef struct termios Termios;
Termios initial_state={0};

void revert_state() {
	tcsetattr(0,TCSAFLUSH,&initial_state);
}

int main() {
	tcgetattr(0,&initial_state);
	atexit(revert_state);

	Termios raw=initial_state;
	raw.c_lflag&=~(ICANON|ECHO); // Disable canonical mode and echo
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);

	int ch=0;
	while(ch!='q') {
		ch=getchar();
		printf("%c",ch);
	}
	printf("\n");

	return 0;
}

