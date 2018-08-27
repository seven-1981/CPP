#ifndef _SPLIT_CONSOLE_H
#define _SPLIT_CONSOLE_H

#include <iostream>
#include <string>
#include <mutex>

#define MAX_SPLIT 4
#define MAX_SIZE 128

//Struct for width and height of console window
struct dimensions
{
	int columns;
	int rows;
};

//Class for a horizontally splitted console
class SplitConsole
{
private:
	//Width and height of actual console window
	dimensions size;
	//Number of horizontal parts of the console window
	int split;

	//Coordinates for start and end of the splits, cursor position 
	int row_start[MAX_SPLIT];
	int row_end[MAX_SPLIT];
	int cursor[MAX_SPLIT];

	//For every part we use a string buffer
	std::string buffer[MAX_SPLIT][MAX_SIZE];

	//Private methods, only used internally
	//Get the size of the actual console window
	dimensions get_console_size();
	//Set the position of the cursor
	void set_console_location(int col, int row);
	//Initialization method
	void init_splits();
	//Draw the horizontal lines
	void draw_lines();
	//Set the size of the actual console window - not working for linux so far
	void set_console_size();
	//Process passed string
	std::string process_string(std::string in);
	//Handle buffer
	void handle_buffer(std::string in, int split, bool inp);
	//Flush buffer
	void flush_buffer(int split);
	//Saved coordinates for input cursor location
	dimensions inp_cursor;
	bool awaiting_input;

	//Mutex for critical sections
	std::mutex mtx;

public:
	//Constructors
	SplitConsole();
	SplitConsole(int split);

	//Public methods (write, read and erase screen)
	void WriteToSplitConsole(std::string in, int split);
	std::string ReadFromSplitConsole(std::string prompt, int split);
	void erase_screen();
};

#endif