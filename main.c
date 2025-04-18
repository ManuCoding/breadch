#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define MAX_CRUMBS 64

static char* empty_msg="No set breadcrumb\n";

char** read_crumbs(size_t* count) {
	static char* paths[64*sizeof(char*)];
	FILE* f=NULL;
	if(isatty(STDIN_FILENO)) {
		char* home=getenv("HOME");
		if(home) {
			char path[PATH_MAX];
			snprintf(path,sizeof(path),"%s/%s",home,".breadcrumbs");
			f=fopen(path,"r");
		}
		if(!f) {
			return paths;
		}
	} else {
		f=stdin;
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
int keys_fd=0;

void revert_state() {
	tcsetattr(keys_fd,TCSAFLUSH,&initial_state);
}

void print_line(char* line,int max) {
	int len=strlen(line);
	if(len+1<max) {
		fprintf(stderr,"\r %s\r",line);
	} else {
		int mid=max/2-2;
		if(mid<=0)
			fprintf(stderr,"\r %.*s\r",max-1,line);
		else
			fprintf(stderr,"\r %.*s\x1b[31m...\x1b[97m%.*s\r",mid,line,len-mid,line+len-mid);
	}
}

int select_menu(char** options,size_t count) {
	keys_fd=open("/dev/tty",O_RDONLY);

	tcgetattr(keys_fd,&initial_state);
	atexit(revert_state);

	Termios raw=initial_state;
	raw.c_lflag&=~(ICANON|ECHO); // Disable canonical mode and echo
	raw.c_cc[VMIN]=1;  // Read at least 1 character
	raw.c_cc[VTIME]=0; // No timeout
	tcsetattr(keys_fd,TCSAFLUSH,&raw);

	if(count<1) return -1;

	struct winsize w;
	ioctl(keys_fd,TIOCGWINSZ,&w);

	fprintf(stderr,"\x1b[0;7m");
	print_line(options[0],w.ws_col);
	fprintf(stderr,"\x1b[0m\n");

	for(size_t i=1; i<count; i++) {
		print_line(options[i],w.ws_col);
		fprintf(stderr," \n");
	}
	fprintf(stderr,"\x1b[%zuA",count);

	char ch=0;
	int selection=0;
	while(ch!='q') {
		read(keys_fd,&ch,1);
		if(!ch) break;
		ioctl(keys_fd,TIOCGWINSZ,&w);
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
					fprintf(stderr,"\x1b[0m");
					print_line(options[selection],w.ws_col);
					fprintf(stderr,"\n\x1b[7m");
					print_line(options[selection+1],w.ws_col);
					fprintf(stderr,"\x1b[0m");
					selection++;
				}
			break;
			case 'P'-'@': // C-p
			case 'k':
				moveup:
				if(selection>0) {
					fprintf(stderr,"\x1b[0m");
					print_line(options[selection],w.ws_col);
					fprintf(stderr,"\x1b[1A\x1b[7m");
					print_line(options[selection-1],w.ws_col);
					fprintf(stderr,"\x1b[0m");
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
			fprintf(stderr,"Do you really want to delete all breadcrumbs?\n");
			int choice=select_menu(options,2);
			if(choice==1) {
				write_crumbs(options,0);
			}
			return 0;
		}
		if(strcmp(argv[1],"-d")==0) {
			if(count) {
				fprintf(stderr,"\x1b[31mDELETING CRUMB\n");
				int choice=select_menu(crumbs,count);
				fprintf(stderr,"\x1b[0m");
				if(choice>-1) {
					for(size_t i=choice; i<count-1; i++) {
						crumbs[i]=crumbs[i+1];
					}
					count--;
					write_crumbs(crumbs,count);
				}
				return 0;
			} else {
				fprintf(stderr,empty_msg);
				return 1;
			}
		}
		if(strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--help")==0) {
			printf("BREADCH - BREADcrumb CHooser\n");
			printf("\n");
			printf("Usage: %s         choose from saved breadcrumbs\n",argv[0]);
			printf("       command|%s choose from command output\n",argv[0]);
			printf("Note:\n");
			printf("       empty lines and lines starting with `#` are ignored\n");
			printf("\n");
			printf("Arguments:\n");
			printf("   -l     list saved breadcrumbs\n");
			printf("   -c     clears all set breadcrumbs\n");
			printf("   -a     saves the current working directory to breadcrumbs\n");
			printf("   -s     prints a '* ' to stdout if the current directory is a breadcrumb\n");
			printf("   -d     choose a breadcrumb to delete\n");
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

