#include <ncurses.h>
#include <stdint.h>

#define WIDTH	20
#define HEIGHT	20

uint64_t time = 0, score = 0;

int main(){
	initscr();
	curs_set(0);
	start_color();
	refresh();
	init_pair(1, COLOR_YELLOW, COLOR_BLACK);
	init_pair(2, COLOR_RED, COLOR_BLACK);
	WINDOW *display = newwin(HEIGHT, WIDTH, 2, 2);
	box(display, 0, 0);
	wrefresh(display);
	mvprintw(2, 2+WIDTH+2, "Time:");
	attron(COLOR_PAIR(1));
	mvprintw(3, 2+WIDTH+2, "%02d:%02d:%02d", time/3600, (time/60)%60, time%60);
	attroff(COLOR_PAIR(1));
	mvprintw(4, 2+WIDTH+2, "Score:");
	attron(COLOR_PAIR(2));
	mvprintw(5, 2+WIDTH+2, "%8d", score);
	attroff(COLOR_PAIR(2));
	refresh();
	getch();
	endwin();
	return 0;
}
