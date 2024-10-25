#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_LINE_LENGTH 1024

char** read_crumbs(size_t* count) {
	char* home=getenv("HOME");
	FILE* f=NULL;
	if(home) {
		char path[1024];
		snprintf(path,sizeof(path),"%s/%s",home,".breadcrumbs");
		f=fopen(path,"r");
	}
	if(!f) {
		printf("No set breadcrumb\n");
		return NULL;
	}

	char** paths=malloc(64*sizeof(size_t));
	char buf[MAX_LINE_LENGTH];
	*count=0;
	while(fgets(buf,MAX_LINE_LENGTH,f)) {
		if(!buf[0] || !buf[1]) continue;
		paths[*count]=malloc(MAX_LINE_LENGTH);
		memcpy(paths[*count],buf,MAX_LINE_LENGTH);
		char* line=paths[*count];
		for(size_t i=0; i<MAX_LINE_LENGTH; i++) {
			if(line[i]=='\n' || line[i]=='\0' || line[i]=='#') {
				line[i]='\0';
				if(i==0) {
					free(line);
					line=NULL;
				}
				i=MAX_LINE_LENGTH;
			}
		}
		if(line) {
			line[MAX_LINE_LENGTH-1]='\0';
			*count+=1;
		}
	}
	return paths;
}

typedef struct termios Termios;
Termios initial_state={0};

void revert_state() {
	tcsetattr(0,TCSAFLUSH,&initial_state);
}

int main() {
	size_t count=0;
	char** crumbs=read_crumbs(&count);
	for(size_t i=0; i<count; i++) {
		printf("%s\n",crumbs[i]);
	}
	return 0;
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

