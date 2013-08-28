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

#define WIDTH		20					// WIDTH shouldn't be greater than 63.
#define HEIGHT		20
#define COLOR_TIME	1
#define COLOR_SCORE	2
#define COLOR_BRICK	3
#define ALLONES		(~((uint64_t)0))
#define BARRIER_W	(((uint64_t)1)<<WIDTH)
#define BARRIER_H	(((uint64_t)1)<<63)
#define FULLROW		(BARRIER_W-1)
#define EXIT(n)		do{ endwin(); exit(n); }while(0)
#define WATTRON(w, n)	wattron(w, COLOR_PAIR(n))
#define WATTROFF(w, n)	wattroff(w, COLOR_PAIR(n))

WINDOW *display;
bool fail = false;
uint64_t secs = 0, score = 0, map[HEIGHT+1], curr[4] = {0}, curr_h = HEIGHT;

void update_time(){
	WATTRON(stdscr, COLOR_TIME);
	mvwprintw(stdscr, 3, 2+WIDTH+2+2, "%02ld:%02ld:%02ld", secs/3600, (secs/60)%60, secs%60);
	WATTROFF(stdscr, COLOR_TIME);
}

void update_score(){
	WATTRON(stdscr, COLOR_SCORE);
	mvwprintw(stdscr, 5, 2+WIDTH+2+2, "%8ld", score);
	WATTROFF(stdscr, COLOR_SCORE);
}

void show_bricks(){
	wclear(display);
	box(display, 0, 0);
	WATTRON(display, COLOR_BRICK);
	for(int i = 0; i<HEIGHT; i++)
		for(int j = WIDTH-1; j>=0; j--)
			if(map[i]&(((uint64_t)1)<<j))
				mvwprintw(display, HEIGHT-i, WIDTH-j, " ");
	for(int i = 0; i<4; i++)
		for(int j = WIDTH-1; j>=0; j--)
			if(curr[i]&(((uint64_t)1)<<j))
				mvwprintw(display, HEIGHT-(curr_h+i), WIDTH-j, " ");
	WATTROFF(display, COLOR_BRICK);
	wrefresh(display);
}

int kbhit(void){
	int ch = wgetch(stdscr);
	if(ch==EOF)
		return FALSE;
	ungetch(ch);
	return TRUE;
}

void new_brick(){
	for(int i = 0; i<4; i++)
		map[curr_h+i] |= curr[i];
	int j = curr_h+4, k = 0;
	for(int i = curr_h; i<j; i++){
		map[i-k] = map[i];
		if(map[i]==FULLROW){
			score++;
			k++;
			j = HEIGHT;
		}
	}
	for(int i = HEIGHT-k; i<HEIGHT; i++)
		map[i] = 0;
	/*
	 * TODO: Add a complete method to generate a new brick.
	 */
	curr_h = HEIGHT-4;
	curr[0] = 0x4;
	curr[1] = 0xE;
}

void mv_left(){
	for(int i = 0; i<4; i++){
		uint64_t temp = curr[i]<<1;
		if(((temp&BARRIER_W)!=0)||((temp&map[curr_h+i])!=0))
			return;
	}
	for(int i = 0; i<4; curr[i++] <<= 1);
	show_bricks();
}

void mv_right(){
	for(int i = 0; i<4; i++){
		uint64_t temp = curr[i]>>1;
		if(((curr[i]%2)!=0)||((temp&map[curr_h+i])!=0))
			return;
	}
	for(int i = 0; i<4; curr[i++] >>= 1);
	show_bricks();
}

void mv_down(){
	if(curr_h==0){
		new_brick();
		return;
	}
	for(int i = 0; i<4; i++)
		if((curr[i]&map[curr_h-1+i])!=0){
			new_brick();
			return;
		}
	curr_h--;
	show_bricks();
}

void *Timer(void *args){
	while(1){
		update_time();
		wrefresh(stdscr);
		usleep(1000000);
		secs++;
		mv_down();
		show_bricks();
	}
	return NULL;
}

int main(){
	for(int i = 0; i<HEIGHT; map[i++] = 0);
	map[HEIGHT] = ALLONES;
	initscr();
	curs_set(0);
	noecho();
	start_color();
	leaveok(stdscr, TRUE);
	wrefresh(stdscr);
	init_pair(COLOR_TIME, COLOR_YELLOW, COLOR_BLACK);
	init_pair(COLOR_SCORE, COLOR_RED, COLOR_BLACK);
	init_pair(COLOR_BRICK, COLOR_BLACK, COLOR_GREEN);
	display = newwin(HEIGHT+2, WIDTH+2, 1, 1);
	leaveok(display, TRUE);
	new_brick();
	show_bricks();
	mvwprintw(stdscr, 2, 2+WIDTH+2+2, "Time:");
	mvwprintw(stdscr, 4, 2+WIDTH+2+2, "Score:");
	update_score();
	wrefresh(stdscr);
	pthread_t timer;
	pthread_create(&timer, NULL, Timer, NULL);
	nodelay(stdscr, FALSE);
	while(!fail){
		while(!kbhit());
		/*
		 * TODO: Add input handle here.
		 */
		switch(getch()){
		case 'w':
			break;
		case 'a':
			mv_left();
			break;
		case 's':
			mv_down();
			break;
		case 'd':
			mv_right();
			break;
		case 'q':
			EXIT(0);
		}
	}
	EXIT(0);
}
