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

int select_menu(char** options,size_t count) {
	tcgetattr(0,&initial_state);
	atexit(revert_state);

	Termios raw=initial_state;
	raw.c_lflag&=~(ICANON|ECHO); // Disable canonical mode and echo
	tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw);

	if(count<1) return -1;
	printf("\x1b[0;3m %s\x1b[0m\n",options[0]);
	for(size_t i=1; i<count; i++) {
		printf(" %s\n",options[i]);
	}
	printf("\x1b[%dA",count);

	int ch=0;
	int selection=0;
	while(ch!='q') {
		ch=getchar();
		switch(ch) {
			case 27: // escape
				ch=getchar();
				if(ch=='[') switch(getchar()) {
					case 'A':
						goto moveup;
					break;
					case 'B':
						goto movedown;
					break;
				} else if(ch==27) { // double escape to exit
					goto no_choice;
				}
			break;
			case 'j':
				movedown:
				if(selection+1<count) {
					printf("\x1b[0m %s\n\x1b[3m %s\x1b[0m\r",options[selection],options[selection+1]);
					selection++;
				}
			break;
			case 'k':
				moveup:
				if(selection>0) {
					printf("\x1b[0m %s\r\x1b[1A\x1b[3m %s\x1b[0m\r",options[selection],options[selection-1]);
					selection--;
				}
			break;
			case '\n':
			case 'i':
			case '\t':
				goto chose;
			case 'q':
				goto no_choice;
		}
	}
	printf("\n");

no_choice:
	printf("\x1b[%dB",count-selection);
	return -1;
chose:
	printf("\x1b[%dB",count-selection);
	return selection;
}

int main() {
	size_t count=0;
	char** crumbs=read_crumbs(&count);

	if(count) {
		size_t choice=select_menu(crumbs,count);
		printf("chose: %d\n",choice);
	} else {
		printf("No set breadcrumb\n");
	}

	return 0;
}

