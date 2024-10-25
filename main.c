#include <ncurses.h>

int main() {
	initscr();
	cbreak();
	noecho();

	int y,x;
	getyx(stdscr,y,x);

	int win_width=COLS;
	int win_height=5;

	WINDOW* win=newwin(win_height,win_width,y+1,0);
	box(win,0,0);
	mvwprintw(win,1,1,"Testing...");

	mvprintw(1,1,"okokok");
	wrefresh(win);
	refresh();
	getch();

	delwin(win);
	endwin();
	return 0;
}
