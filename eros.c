/*
 * README:
 * I regard stdscr as a special window to use the same API to control the windows and
 * the stdscr.
 */
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define WIDTH		20					// WIDTH shouldn't be greater than 63.
#define HEIGHT		20
#define CONG_WIDTH	54
#define CONG_HEIGHT	6
#define COLOR_TIME	1
#define COLOR_SCORE	2
#define COLOR_BRICK	3
#define COLOR_LEVEL	4
#define COLOR_HINT	5
#define COLOR_CONG	6
#define ALLONES		(~((uint64_t)0))
#define BARRIER_W	(((uint64_t)1)<<WIDTH)
#define BARRIER_H	(((uint64_t)1)<<63)
#define FULLROW		(BARRIER_W-1)
#define EXIT(n)		do{ endwin(); exit(n); }while(0)
#define RELAX()		usleep(10000);
#define WBKGD(w, n)	wbkgd(w, COLOR_PAIR(n))
#define WATTRON(w, n)	wattron(w, COLOR_PAIR(n))
#define WATTROFF(w, n)	wattroff(w, COLOR_PAIR(n))

WINDOW *display;
bool fail = false;
uint64_t secs = 0, score = 0, map[HEIGHT+2] = {0}, curr[4] = {0}, curr_shape, curr_pos, level = 0;
int curr_h = HEIGHT, curr_w;
uint64_t brick[7][4][4] = {
	{{0x4, 0xE, 0x0, 0x0}, {0x4, 0xC, 0x4, 0x0}, {0x0, 0xE, 0x4, 0x0}, {0x4, 0x6, 0x4, 0x0}},
	{{0x3, 0x3, 0x0, 0x0}, {0x3, 0x3, 0x0, 0x0}, {0x3, 0x3, 0x0, 0x0}, {0x3, 0x3, 0x0, 0x0}},
	{{0x0, 0xF, 0x0, 0x0}, {0x4, 0x4, 0x4, 0x4}, {0x0, 0xF, 0x0, 0x0}, {0x4, 0x4, 0x4, 0x4}},
	{{0x3, 0x2, 0x2, 0x0}, {0x4, 0x7, 0x0, 0x0}, {0x2, 0x2, 0x6, 0x0}, {0x0, 0x7, 0x1, 0x0}},
	{{0x6, 0x2, 0x2, 0x0}, {0x0, 0x7, 0x4, 0x0}, {0x2, 0x2, 0x3, 0x0}, {0x1, 0x7, 0x0, 0x0}},
	{{0x6, 0x3, 0x0, 0x0}, {0x1, 0x3, 0x2, 0x0}, {0x6, 0x3, 0x0, 0x0}, {0x1, 0x3, 0x2, 0x0}},
	{{0x3, 0x6, 0x0, 0x0}, {0x2, 0x3, 0x1, 0x0}, {0x3, 0x6, 0x0, 0x0}, {0x2, 0x3, 0x1, 0x0}}
};

void update_time(){
	WATTRON(stdscr, COLOR_TIME);
	mvwprintw(stdscr, 3, 2+(WIDTH<<1)+2+2, "%02ld:%02ld:%02ld", secs/3600, (secs/60)%60, secs%60);
	WATTROFF(stdscr, COLOR_TIME);
}

void update_score(){
	WATTRON(stdscr, COLOR_SCORE);
	mvwprintw(stdscr, 5, 2+(WIDTH<<1)+2+2, "%8ld", score);
	WATTROFF(stdscr, COLOR_SCORE);
}

void update_level(){
	WATTRON(stdscr, COLOR_LEVEL);
	mvwprintw(stdscr, 7, 2+(WIDTH<<1)+2+2, "%8ld", level);
	WATTROFF(stdscr, COLOR_LEVEL);
}

void show_bricks(){
	wclear(display);
	WATTRON(display, COLOR_HINT);
	bool bottom = false;
	int temp_curr_h = curr_h;
	while(!bottom){
		for(int i = 0; i<4; i++){
			if(temp_curr_h-1+i<0)
				continue;
			if((curr[i]&map[temp_curr_h-1+i])!=0){
				bottom = true;
				break;
			}
		}
		temp_curr_h--;
	}
	temp_curr_h++;
	for(int i = 3; i>=0; i--)
		for(int j = WIDTH-1; j>=0; j--)
			if(curr[i]&(((uint64_t)1)<<j))
				mvwprintw(display, HEIGHT-temp_curr_h-i, ((WIDTH-j)<<1)-2, "  ");
	WATTROFF(display, COLOR_HINT);
	WATTRON(display, COLOR_BRICK);
	for(int i = 1; i<=HEIGHT; i++)
		for(int j = WIDTH-1; j>=0; j--)
			if(map[i]&(((uint64_t)1)<<j))
				mvwprintw(display, HEIGHT-i, ((WIDTH-j)<<1)-2, "  ");
	for(int i = 3; i>=0; i--)
		for(int j = WIDTH-1; j>=0; j--)
			if(curr[i]&(((uint64_t)1)<<j))
				mvwprintw(display, HEIGHT-curr_h-i, ((WIDTH-j)<<1)-2, "  ");
	WATTROFF(display, COLOR_BRICK);
	wrefresh(display);
}

int kbhit(void){
	wtimeout(stdscr, 0);
	int ch = wgetch(stdscr);
	nodelay(stdscr, FALSE);
	if(ch==EOF)
		return false;
	ungetch(ch);
	return true;
}

void new_brick(){
	for(int i = 0; i<4; i++)
		if(curr_h+i>0)
			map[curr_h+i] |= curr[i];
	int temp, j, k = 0;
	temp = curr_h<1? 1 : curr_h;
	j = temp+4<=HEIGHT+1? temp+4 : HEIGHT;
	for(int i = temp; i<j; i++){
		map[i-k] = map[i];
		if(map[i]!=FULLROW)
			continue;
		k++;
		j = HEIGHT+1;
	}
	for(int i = HEIGHT+1-k; i<=HEIGHT; i++)
		map[i] = 0;
	score += level*k*k;
	level = (score/HEIGHT)+1;
	if(k>0)
		update_score();
	wrefresh(stdscr);
	int irand = rand();
	curr_shape = irand%7;
	curr_pos = irand%4;
	curr_w = (WIDTH-4)>>1;
	for(int i = 0; i<4; i++)
		curr[i] = brick[curr_shape][curr_pos][i]<<curr_w;
	curr_h = HEIGHT-3;
	for(int i = 3; i>=0; i--)
		if(curr[i]==0)
			curr_h++;
		else
			break;
	for(int i = 0; i<4; i++)
		if((curr[i]&map[curr_h+i])!=0){
			fail = true;
			break;
		}
}

void mv_left(){
	for(int i = 0; i<4; i++){
		if(curr_h+i<0)
			continue;
		if(((curr[i]<<1)&(BARRIER_W|map[curr_h+i]))!=0)
			return;
	}
	for(int i = 0; i<4; curr[i++] <<= 1);
	curr_w++;
	show_bricks();
}

void mv_right(){
	for(int i = 0; i<4; i++){
		if(curr_h+i<0)
			continue;
		if(((curr[i]%2)!=0)||(((curr[i]>>1)&map[curr_h+i])!=0))
			return;
	}
	for(int i = 0; i<4; curr[i++] >>= 1);
	curr_w--;
	show_bricks();
}

void mv_down(){
	for(int i = 0; i<4; i++){
		if(curr_h-1+i<0)
			continue;
		if((curr[i]&map[curr_h-1+i])!=0){
			new_brick();
			return;
		}
	}
	curr_h--;
	show_bricks();
}

void mv_bottom(){
	bool bottom = false;
	while(!bottom){
		for(int i = 0; i<4; i++){
			if(curr_h-1+i<0)
				continue;
			if((curr[i]&map[curr_h-1+i])!=0){
				bottom = true;
				break;
			}
		}
		curr_h--;
	}
	curr_h++;
	mv_down();
	show_bricks();
}

int test_collision(uint64_t test_brick[4]){
	for(int i = 0; i<4; i++){
		if((test_brick[i]&(BARRIER_W|map[curr_h+i]))!=0)
			return true;
		if(curr_w<0){
			uint64_t temp = brick[curr_shape][(curr_pos+1)%4][i];
			for(int j = 1; j<=(-curr_w); j++){
				if((temp%2)!=0)
					return true;
				temp >>= 1;
			}
		}
	}
	return false;
}

void turn(){
	uint64_t temp[4];
	for(int i = 0; i<4; i++){
		temp[i] = brick[curr_shape][(curr_pos+1)%4][i];
		if(curr_w>=0)
			temp[i] <<= curr_w;
		else
			temp[i] >>= (-curr_w);
	}
	if(!test_collision(temp)){
		for(int i = 0; i<4; i++)
			curr[i] = temp[i];
		curr_pos = (curr_pos+1)%4;
	}
	show_bricks();
}

int main(){
	map[0] = map[HEIGHT+1] = ALLONES;
	srand((unsigned)time(NULL));
	initscr();
	curs_set(0);
	noecho();
	start_color();
	leaveok(stdscr, TRUE);
	wrefresh(stdscr);
	init_pair(COLOR_TIME,  COLOR_YELLOW, COLOR_BLACK);
	init_pair(COLOR_SCORE, COLOR_RED,    COLOR_BLACK);
	init_pair(COLOR_BRICK, COLOR_BLACK,  COLOR_GREEN);
	init_pair(COLOR_LEVEL, COLOR_BLUE,   COLOR_BLACK);
	init_pair(COLOR_HINT,  COLOR_BLACK,  COLOR_WHITE);
	init_pair(COLOR_CONG,  COLOR_CYAN,   COLOR_MAGENTA);
	WINDOW *display_border = newwin(HEIGHT+2, (WIDTH<<1)+2, 1, 1);
	box(display_border, 0, 0);
	wrefresh(display_border);
	display = newwin(HEIGHT, WIDTH<<1, 2, 2);
	leaveok(display, TRUE);
	new_brick();
	show_bricks();
	mvwprintw(stdscr, 2, 2+(WIDTH<<1)+2+2, "Time:");
	update_time();
	mvwprintw(stdscr, 4, 2+(WIDTH<<1)+2+2, "Score:");
	update_score();
	mvwprintw(stdscr, 6, 2+(WIDTH<<1)+2+2, "Level:");
	update_level();
	wrefresh(stdscr);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t count = 0;
	while(!fail){
		struct timeval temptv;
		gettimeofday(&temptv, NULL);
		tv.tv_sec = tv.tv_sec>temptv.tv_sec? -1 : tv.tv_sec;
		if(((temptv.tv_sec-tv.tv_sec)*1000000+temptv.tv_usec-tv.tv_usec)>=1000000/level){
			tv = temptv;
			secs += (++count)==level;
			count *= count!=level;
			update_time();
			update_level();
			wrefresh(stdscr);
			mv_down();
			show_bricks();
		}
		if(kbhit()){
			switch(getch()){
			case 'w':
			case 'W':
				turn();
				break;
			case 'a':
			case 'A':
				mv_left();
				break;
			case 's':
			case 'S':
				mv_down();
				break;
			case 'd':
			case 'D':
				mv_right();
				break;
			case 10:				// For case ENTER pressed
				mv_bottom();
				break;
			case 'q':
				fail = true;
				break;
			case 0x1B:				// For case arrow pressed
				if(getch()==0x5B)
					switch(getch()){
					case 0x41:
						turn();
						break;
					case 0x44:
						mv_left();
						break;
					case 0x43:
						mv_right();
						break;
					case 0x42:
						mv_down();
						break;
					}
				break;
			}
		}
		RELAX();
	}
	int row, col;
	getmaxyx(stdscr, row, col);
	WINDOW *cong = newwin(CONG_HEIGHT, CONG_WIDTH, (row-CONG_HEIGHT)>>1, (col-CONG_WIDTH)>>1);
	WBKGD(cong, COLOR_CONG);
	wclear(cong);
	box(cong, 0, 0);
	mvwprintw(cong, 1, (CONG_WIDTH-16)>>1, "Congratulations!");
	mvwprintw(cong, 2, 1, "  You\'ve getten %ld scores at level %ld.", score, level);
	mvwprintw(cong, 3, 1, "  Press ENTER to share with friends or just press ");
	mvwprintw(cong, 4, 1, "any other key to quit.");
	wrefresh(cong);
	if(getch()!=10)
		EXIT(0);
	/*
	 * TODO: share the scores and level on Renren.
	 */
	EXIT(0);
}
