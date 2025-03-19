#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>

#define MAX_CRUMBS 64

static char* empty_msg="No set breadcrumb\n";

char** read_crumbs(size_t* count) {
	static char* paths[64*sizeof(size_t)];
	char* home=getenv("HOME");
	FILE* f=NULL;
	if(home) {
		char path[PATH_MAX];
		snprintf(path,sizeof(path),"%s/%s",home,".breadcrumbs");
		f=fopen(path,"r");
	}
	if(!f) {
		return paths;
	}

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

bool write_crumbs(char** crumbs,size_t count) {
	char* home=getenv("HOME");
	FILE* f=NULL;
	if(home) {
		char path[1024];
		snprintf(path,sizeof(path),"%s/%s",home,".breadcrumbs");
		f=fopen(path,"w");
	}
	if(!f) {
		return false;
	}
	for(size_t i=0; i<count; i++) {
		fprintf(f,"%s\n",crumbs[i]);
	}
	fclose(f);
	return true;
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

int main(int argc,char** argv) {
	size_t count=0;
	char** crumbs=read_crumbs(&count);
	if(argc>1) {
		if(strcmp(argv[1],"-a")==0) {
			for(size_t i=MAX_CRUMBS-1; i>0; i--) {
				crumbs[i]=crumbs[i-1];
			}
			crumbs[0]=malloc(PATH_MAX);
			getcwd(crumbs[0],PATH_MAX);
			crumbs[0][PATH_MAX-1]='\0';
			write_crumbs(crumbs,count+1);
			return 0;
		}
		if(strcmp(argv[1],"-l")==0) {
			if(count) {
				for(size_t i=0; i<count; i++) {
					printf("%s\n",crumbs[i]);
				}
			} else {
				fprintf(stderr,empty_msg);
			}
			return 0;
		}
		if(strcmp(argv[1],"-s")==0) {
			char* cwd=malloc(PATH_MAX);
			getcwd(cwd,PATH_MAX);
			cwd[PATH_MAX-1]='\0';
			for(size_t i=0; i<count; i++) {
				if(strcmp(crumbs[i],cwd)==0) {
					printf("* ");
					return 0;
				}
			}
			return 0;
		}
		if(strcmp(argv[1],"-c")==0) {
			char* options[2*sizeof(char*)];
			options[0]="no";
			options[1]="yes";
			fprintf(stderr,"Do you really want to delete crumbs?\n");
			int choice=select_menu(options,2);
			if(choice==1) {
				write_crumbs(options,0);
			}
			return 0;
		}
		if(strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--help")==0) {
			printf("BREADCH - BREADcrumb CHooser\n");
			printf("\n");
			printf("Usage: %s        choose from saved breadcrumbs\n",argv[0]);
			printf("\n");
			printf("Arguments:\n");
			printf("   -l     list saved breadcrumbs\n");
			printf("   -c     clears all set breadcrumbs\n");
			printf("   -a     saves the current working directory to breadcrumbs\n");
			printf("   -s     prints a '* ' to stdout if the current directory is a breadcrumb\n");
			printf("   -h     display this help\n");
			printf("\n");
			return 0;
		}
		fprintf(stderr,"Unknown option `%s`, see `%s -h` for usage.\n",argv[1],argv[0]);
		return 1;
	}

	if(count) {
		int choice=select_menu(crumbs,count);
		if(choice>-1) printf("%s",crumbs[choice]);
		if(choice<0) printf("%s",getcwd(crumbs[0],PATH_MAX));
	} else {
		fprintf(stderr,empty_msg);
		return 1;
	}

	return 0;
}

