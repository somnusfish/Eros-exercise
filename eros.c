/*
 * README:
 * I regard stdscr as a special window to use the same API to control the windows and
 * the stdscr.
 */
#include <ncurses.h>
#include <stdint.h>

#define WIDTH		20
#define HEIGHT		20
#define COLOR_TIME	1
#define COLOR_SCORE	2
#define COLOR_BRICK	3

uint64_t time = 0, score = 0;

int main(){
	initscr();
	curs_set(0);
	start_color();
	wrefresh(stdscr);
	init_pair(COLOR_TIME, COLOR_YELLOW, COLOR_BLACK);
	init_pair(COLOR_SCORE, COLOR_RED, COLOR_BLACK);
	init_pair(COLOR_BRICK, COLOR_BLACK, COLOR_GREEN);
	WINDOW *display = newwin(HEIGHT, WIDTH, 1, 1);
	box(display, 0, 0);
	wattron(display, COLOR_PAIR(COLOR_BRICK));
	mvwprintw(display, 3, 3, "   ");
	mvwprintw(display, 4, 4, " ");
	wattroff(display, COLOR_PAIR(COLOR_BRICK));
	wrefresh(display);
	mvwprintw(stdscr, 2, 2+WIDTH+2, "Time:");
	wattron(stdscr, COLOR_PAIR(COLOR_TIME));
	mvwprintw(stdscr, 3, 2+WIDTH+2, "%02d:%02d:%02d", time/3600, (time/60)%60, time%60);
	wattroff(stdscr, COLOR_PAIR(COLOR_TIME));
	mvwprintw(stdscr, 4, 2+WIDTH+2, "Score:");
	wattron(stdscr, COLOR_PAIR(COLOR_SCORE));
	mvwprintw(stdscr, 5, 2+WIDTH+2, "%8d", score);
	wattroff(stdscr, COLOR_PAIR(COLOR_SCORE));
	wrefresh(stdscr);
	getch();
	endwin();
	return 0;
}
