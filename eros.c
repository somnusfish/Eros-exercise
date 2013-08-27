/*
 * README:
 * I regard stdscr as a special window to use the same API to control the windows and
 * the stdscr.
 */
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#define WIDTH		20
#define HEIGHT		20
#define COLOR_TIME	1
#define COLOR_SCORE	2
#define COLOR_BRICK	3
#define EXIT(n)		do{ endwin(); exit(n); }while(0)
#define WATTRON(w, n)	wattron(w, COLOR_PAIR(n))
#define WATTROFF(w, n)	wattroff(w, COLOR_PAIR(n))

uint64_t secs = 0, score = 0;

#define UPDATE_TIME()				\
	do{					\
		WATTRON(stdscr, COLOR_TIME);	\
		mvwprintw(stdscr, 3, 2+WIDTH+2, "%02ld:%02ld:%02ld", secs/3600, (secs/60)%60, secs%60);	\
		WATTROFF(stdscr, COLOR_TIME);	\
	}while(0)
#define UPDATE_SCORE()				\
	do{					\
		WATTRON(stdscr, COLOR_SCORE);	\
		mvwprintw(stdscr, 5, 2+WIDTH+2, "%8ld", score);	\
		WATTROFF(stdscr, COLOR_SCORE);	\
	}while(0)

int kbhit(void){
	int ch;
	if((ch = wgetch(stdscr))==EOF)
		return FALSE;
	ungetch(ch);
	return TRUE;
}

void *Timer(void *args){
	while(1){
		sleep(1);
		secs++;
		UPDATE_TIME();
		wrefresh(stdscr);
	}
	return NULL;
}

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
	WATTRON(display, COLOR_BRICK);
	mvwprintw(display, 3, 3, "   ");
	mvwprintw(display, 4, 4, " ");
	WATTROFF(display, COLOR_BRICK);
	wrefresh(display);
	mvwprintw(stdscr, 2, 2+WIDTH+2, "Time:");
	UPDATE_TIME();
	mvwprintw(stdscr, 4, 2+WIDTH+2, "Score:");
	UPDATE_SCORE();
	wrefresh(stdscr);
	pthread_t timer;
	pthread_create(&timer, NULL, Timer, NULL);
	nodelay(stdscr, FALSE);
	while(!kbhit());
	EXIT(0);
}
