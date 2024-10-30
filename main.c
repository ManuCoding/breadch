#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>

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
	char buf[PATH_MAX];
	*count=0;
	while(fgets(buf,PATH_MAX,f)) {
		if(!buf[0] || !buf[1]) continue;
		paths[*count]=malloc(PATH_MAX);
		memcpy(paths[*count],buf,PATH_MAX);
		char* line=paths[*count];
		for(size_t i=0; i<PATH_MAX; i++) {
			if(line[i]=='\n' || line[i]=='\0' || line[i]=='#') {
				line[i]='\0';
				if(i==0) {
					free(line);
					line=NULL;
				}
				i=PATH_MAX;
			}
		}
		if(line) {
			line[PATH_MAX-1]='\0';
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
	fprintf(stderr,"\x1b[0;7m %s\x1b[0m\n",options[0]);
	for(size_t i=1; i<count; i++) {
		fprintf(stderr," %s\n",options[i]);
	}
	fprintf(stderr,"\x1b[%zuA",count);

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
			case 'N'-'@': // C-n
			case 'j':
				movedown:
				if(selection+1<(int)count) {
					fprintf(stderr,"\x1b[0m %s\n\x1b[7m %s\x1b[0m\r",options[selection],options[selection+1]);
					selection++;
				}
			break;
			case 'P'-'@': // C-p
			case 'k':
				moveup:
				if(selection>0) {
					fprintf(stderr,"\x1b[0m %s\r\x1b[1A\x1b[7m %s\x1b[0m\r",options[selection],options[selection-1]);
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
	fprintf(stderr,"\n");

no_choice:
	fprintf(stderr,"\x1b[%zuB",count-selection);
	return -1;
chose:
	fprintf(stderr,"\x1b[%zuB",count-selection);
	return selection;
}

int main() {
	size_t count=0;
	char** crumbs=read_crumbs(&count);

	if(count) {
		int choice=select_menu(crumbs,count);
		if(choice>-1) printf("%s",crumbs[choice]);
		if(choice<0) printf("%s",getcwd(crumbs[0],PATH_MAX));
	} else {
		fprintf(stderr,"No set breadcrumb\n");
	}

	return 0;
}

