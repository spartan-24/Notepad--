
#include<ncurses.h>
#include<string>
#include<fstream>
#include<list>
#include<iterator>
#include<iostream>

#define ctrl(x) ((x) & 0x1f)

using namespace std;

int yMax;
int xMax;
int STATUS = 0;
WINDOW* topBar;
WINDOW* bottomBar;
void drawTopBar();
void drawBottomBar(int);
void drawQuitOptionsBar();
string drawNameFileBar();

/* 
   -> Doubly linked list of doubly linked list of characters   
   -> Contents loaded initially if the file is passed as argument 
   -> Used to refresh at insertion, deletion, scroll
*/

class Buffer{

   public:
	
	list< list<char> > data;
	int height;

	Buffer(){
	}

	Buffer(ifstream &f, int height){
		this->height = height;
		if(!f.is_open())
			return;
		while(1){
			string s;
			if(!getline(f, s))
				break;
			data.push_back(list<char>());
			for(char c : s)
				data.back().push_back(c);
		}
	}

	void printData(WINDOW* win, list< list<char> >::iterator &currline){
		int y, x;
		getyx(win, y, x);

		wclear(win);
		//init_pair(2, COLOR_RED, COLOR_GREEN);
		//wbkgd(win, COLOR_PAIR(2));
		keypad(win, true);
		
		// Print currline and lines above
		auto it = currline;
		int linenum = y;
		while(linenum >= 0){
			wmove(win, linenum, 0);
			for(auto j = (*it).begin(); j != (*it).end(); j++){
				wprintw(win, "%c", *j);
			}
			wprintw(win, "\n");
			linenum--;
			it--;
		}

		// Print lines below
		it = currline;
		it++;
		linenum = y + 1;
		while((it != data.end()) && (linenum < height)){
			wmove(win, linenum, 0);
			for(auto j = (*it).begin(); j != (*it).end(); j++){
				wprintw(win, "%c", *j);
			}
			wprintw(win, "\n");
			linenum++;
			it++;
		}

		wmove(win, y, x);  // Restore original coordinates
		refresh();
		wrefresh(win);
	}
	
};

/* 
   -> Main text display
*/

class MainWindow{

   public:
  
	WINDOW* win;
	Buffer buffer;
	list< list<char> >::iterator currline;
	int height;
	int width;
	int SAVED = 1;
	string filename;

	MainWindow(){
	}

	MainWindow(string f){
		filename = f;
		height = yMax - 2;
		width = xMax;
		win = newwin(height, width, 1, 0);
		keypad(win, true);
		wmove(win, 0, 0);
		//init_pair(2, COLOR_RED, COLOR_GREEN);
		//wbkgd(win, COLOR_PAIR(2));

		ifstream file;
		file.open(filename);
		buffer = Buffer(file, height);
		file.close();

		wmove(win, 0, 0);
		if(buffer.data.empty())
			buffer.data.push_back(list<char>());
		currline = buffer.data.begin();
		buffer.printData(win, currline);
		
		refresh();
		wrefresh(win);
	}

	void moveUp(){
		int y, x;
		getyx(win, y, x);
		if(currline == buffer.data.begin())   // Current line is the first.
			return;			      // Cannot scroll up.
		auto prevline = currline;
		--prevline;

		// Move the cursor up
		if(y != 0){ 
			int currSize = (int)((*(currline)).size());
			int newSize = (int)((*(prevline)).size());
			if(newSize < currSize && x > newSize)
				x = newSize;
			wmove(win, y - 1, x); 
			currline = prevline;
		}
		// Scroll up	
		else{
			int currSize = (int)((*(currline)).size());
			int newSize = (int)((*(prevline)).size());
			if(newSize < currSize && x > newSize)
				x = newSize;
			currline = prevline;
			wmove(win, y, x);
			buffer.printData(win, currline);
		}
	}
	
	void moveDown(){
		int y, x;
		getyx(win, y, x);
		auto nextline = currline;
		++nextline;
		if(nextline == buffer.data.end())    // Current line is the last.
			return; 		     // Cannot scroll down.

		// Move the cursor down
		if(y != height - 1){
			int currSize = (int)((*(currline)).size());
			int newSize = (int)((*(nextline)).size());
			if(newSize < currSize && x > newSize)
				x = newSize;
			wmove(win, y + 1, x);
			currline = nextline;
		}
		// Scroll down
		else{
			int currSize = (int)((*(currline)).size());
			int newSize = (int)((*(nextline)).size());
			if(newSize < currSize && x > newSize)	
				x = newSize;
			currline = nextline;
			wmove(win, y, x);
			buffer.printData(win, currline);
		}
	}
	
	void moveLeft(){
		int y, x;
		getyx(win, y, x);
		if(x > 0){
			x--;
		}
		else{
			x = 0;
		}
		wmove(win, y, x);
	}

	void moveRight(){
		int y, x;
		getyx(win, y, x);
		int len = (int)(*currline).size();
		if(x < len){
			x++;
		}
		else{
			x = len;  // Cannot move beyond the last char in the line
		}
		wmove(win, y, x);
	}

	void insertChar(char c){
		int y, x;
		getyx(win, y, x);

		if(c != '\n'){
			auto it = (*currline).begin();
			for(int i = 0; i < x; i++){
				it++;
			}
			(*currline).insert(it, c);
			buffer.printData(win, currline);
			wmove(win, y, x+1);
		}
		else{
			string s = "";
			auto it = (*currline).begin();	
			for(int i = 0; i < x; i++){
				it++;
			}
			auto rem = it;
			for(; it != (*currline).end(); it++){
				s += (*it);    // Load the rest of the line
			}
			(*currline).erase(rem, (*currline).end());  // Erase the rest of the line
			currline++;
			buffer.data.insert(currline, list<char>());  // Insert new line and initialize its contents
			currline--;
			for(char c : s)
				(*currline).push_back(c);
			if(y == height - 1)
				wmove(win, y, 0);
			else
				wmove(win, y+1, 0);
			buffer.printData(win, currline);
		}
	}

	void deleteChar(){
		int y, x;
		getyx(win, y, x);
		
		if(x != 0){
			auto it = (*currline).begin();
			for(int i = 0; i < x - 1; i++){
				it++;
			}
			(*currline).erase(it);
			buffer.printData(win, currline);
			wmove(win, y, x - 1);
		}
		else{
			if(y == 0)
				return;
			string s = "";
			for(auto it = (*currline).begin(); it != (*currline).end(); it++)
				s += (*it);
			auto remline = currline;
			currline--;
			x = (int)(*currline).size();
			buffer.data.erase(remline);    // Erase the line and insert its contents into previous
			for(char c : s)
				(*currline).push_back(c);
			wmove(win, y - 1, x);
			buffer.printData(win, currline);
		}
	}

	void save(){
		if(filename == ""){
			filename = drawNameFileBar();
		}
		ofstream file;
		file.open(filename, ios::out | ios::trunc);

		for(auto i = buffer.data.begin(); i != buffer.data.end(); i++){
			for(auto j = (*i).begin(); j != (*i).end(); j++){
				file << (*j);
			}
			file << "\n";
		}

		file.close();
	}

	void quit(int &end){
		if(STATUS == 1){
			end = 1;
			return;
		}
		drawQuitOptionsBar();	
		while(1){
			int c = wgetch(win);
			int endLoop = 0;
			switch(c){
				case 'y':
				case 'Y':
					save();
					end = endLoop = 1;
					break;
				case 'n':
				case 'N':
					end = endLoop = 1;
					break;
				case 'c':
				case 'C':
					end = 0;
					endLoop = 1;
					break;
				default:
					break;
			}
			if(endLoop)
				break;
		}
		drawBottomBar(STATUS);
	}

	void process(){
		while(1){
			int c = wgetch(win);
			int end = 0;
			switch(c){
				case KEY_UP:
					moveUp();
					break;
				case KEY_DOWN:
					moveDown();
					break;
				case KEY_LEFT:
					moveLeft();
					break;
				case KEY_RIGHT:
					moveRight();
					break;
				case KEY_BACKSPACE:
					deleteChar();
					if(SAVED){
						wclear(bottomBar);
						drawBottomBar(2);
						refresh();
						wrefresh(bottomBar);
						SAVED = 0;
					}

					break;
				case ctrl('s'):
					save();
					SAVED = 1;
					wclear(bottomBar);
					drawBottomBar(1);
					refresh();
					wrefresh(bottomBar);
					break;
				case ctrl('q'):
					quit(end);
					break;
				default:
					insertChar(c);
					if(SAVED){
						wclear(bottomBar);
						drawBottomBar(2);
						refresh();
						wrefresh(bottomBar);
						SAVED = 0;
					}
					break;
			}
			if(end)
				break;
		}
	}
};

void drawTopBar(){
	int y, x;
	getyx(stdscr, y, x);
	wbkgd(topBar, COLOR_PAIR(1));
	string title = "notepad-- version 1.0";
	mvwprintw(topBar, 0, (xMax - (int)title.length())/2, title.c_str());
	wmove(stdscr, y, x);
}

void drawBottomBar(int status){
	STATUS = status;
	int y, x;
	getyx(stdscr, y, x);
	wclear(bottomBar);
	wbkgd(bottomBar, COLOR_PAIR(1));
	string save = "Ctrl + S : Save";
	string quit = "Ctrl + Q : Quit";
	mvwprintw(bottomBar, 0, 0, save.c_str());
	mvwprintw(bottomBar, 0, (int)save.length() + 7, quit.c_str());
	if(status == 1){
		string saved = "File is up to date";
		mvwprintw(bottomBar, 0, (int)save.length() + (int)quit.length() + 14, saved.c_str());
	}
	else if(status == 2){
		string unsaved = "Unsaved changes";
		mvwprintw(bottomBar, 0, (int)save.length() + (int)quit.length() + 14, unsaved.c_str());
	}
	wmove(stdscr, y, x);
	refresh();
	wrefresh(bottomBar);
}

void drawQuitOptionsBar(){
	int y, x;
	getyx(stdscr, y, x);
	wclear(bottomBar);
	wbkgd(bottomBar, COLOR_PAIR(1));	
	string quitmsg = "Save file before quitting?";
	string yes = "Y: Yes";
	string no = "N: No";
	string cancel = "C: Cancel";
	mvwprintw(bottomBar, 0, 0, quitmsg.c_str());
	mvwprintw(bottomBar, 0, (int)quitmsg.length() + 5, yes.c_str());
	mvwprintw(bottomBar, 0, (int)quitmsg.length() + (int)yes.length() + 10, no.c_str());
	mvwprintw(bottomBar, 0, (int)quitmsg.length() + (int)yes.length() + (int)no.length() + 15, cancel.c_str());
	wmove(stdscr, y, x);
	refresh();
	wrefresh(bottomBar);
}

string drawNameFileBar(){
	int y, x;
	getyx(stdscr, y, x);
	wclear(bottomBar);
	wbkgd(bottomBar, COLOR_PAIR(1));
	string newfilemsg = "Enter the name of the file: ";
	string filename = "";
	mvwprintw(bottomBar, 0, 0, newfilemsg.c_str());
	while(1){
		int c = wgetch(bottomBar);
		int y, x;
		getyx(bottomBar, y, x);
		if(c == '\n'){
			break;
		}
		else{
			winsch(bottomBar, c);
			wmove(bottomBar, y, x+1);
			filename += c;
		}
	}
	drawBottomBar(STATUS);
	wmove(stdscr, y, x);
	return filename;
}

void drawBars(){
	int y, x;
	getyx(stdscr, y, x);
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	drawTopBar();
	drawBottomBar(0);
	refresh();
	wrefresh(topBar);
	wrefresh(bottomBar);
	wmove(stdscr, y, x);
}

void init(){
	initscr();
	noecho();
	cbreak();
	raw();
	start_color();
	keypad(stdscr, true);

	getmaxyx(stdscr, yMax, xMax);
	topBar = newwin(1, xMax, 0, 0);
	bottomBar = newwin(2, xMax, yMax - 1, 0);
	move(1, 0);
	drawBars();
}


int main(int argc, char *argv[]){

	init();

	if(argc > 2){
		endwin();
		cout << "More than one file provided as argument" << endl;
		return 0;
	}
	else if(argc == 2){
		MainWindow mainWin = MainWindow(argv[1]);
		mainWin.process();
	}
	else{
		MainWindow mainWin = MainWindow("");
		mainWin.process();
	}

	endwin();
	return 0;
}
