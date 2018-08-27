#include <iostream>
#include <string>
#include <cmath>
#include <stdio.h>
#include <thread>
#include <chrono>
#include <fstream>
#include "SplitConsole.hpp"

//Depending on the operating system, there are different methods used for
//handling the console window
#ifdef _WIN32
	#include <Windows.h>
	#include <conio.h>
#else
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <sys/select.h>
	#include <termios.h>
	#include <stropts.h>
#endif

SplitConsole::SplitConsole()
{
	//Default constructor, we assume 2 splits
	dimensions dim = get_console_size();
	this->size.columns = dim.columns;
	this->size.rows = dim.rows;
	this->split = 2;
	init_splits();
}

SplitConsole::SplitConsole(int split)
{
	//Constructor for 'split' amount of splits
	dimensions dim = get_console_size();
	this->size.columns = dim.columns;
	this->size.rows = dim.rows;
	if (split > MAX_SPLIT)
		split = MAX_SPLIT;
	this->split = split;
	init_splits();
}

dimensions SplitConsole::get_console_size()
{
	#ifdef _WIN32
		//Create struct and get console window size
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		dimensions retval;

		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		retval.columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		retval.rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

		return retval;
	#else
		//Create struct and get console window size
		dimensions retval;
		struct winsize ws;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
		retval.columns = ws.ws_col;
		retval.rows = ws.ws_row;

		return retval;
	#endif
}

void SplitConsole::init_splits()
{
	//Initialization of the console split class instance
	//Initialize variables
	for (int i = 0; i < MAX_SPLIT; i++)
	{
		this->row_start[i] = 0;
		this->row_end[i] = 0;
		this->cursor[i] = 0;

		for (int j = 0; j < MAX_SIZE; j++)
			buffer[i][j] = "";
	}
	
	//Check if split exceeds the allowed amount
	if (this->split > MAX_SPLIT)
		this->split = MAX_SPLIT;

	//Calculate the number of rows for each split
	float frac = (float)(this->size.rows) / (float)(this->split);
	int div = ceil(frac);

	//Calculate the starting row indices for each split
	for (int i = 0; i < this->split; i++)
		this->row_start[i] = (int)((i * div));

	//Calculate the ending row indices for each split
	for (int i = 0; i < this->split; i++)
	{
		if (i == (this->split - 1))
			this->row_end[i] = this->size.rows;
		else
			this->row_end[i] = this->row_start[i + 1] - 2;
	}

	//Set the cursor to the starting positions
	for (int i = 0; i < this->split; i++)
		this->cursor[i] = this->row_start[i];
	
	//Erase the screen
	erase_screen();

	//Draw the horizontal lines
	draw_lines();

	//Init cursor pos for input
	this->inp_cursor.columns = 0;
	this->inp_cursor.rows = 0;
	this->awaiting_input = false;
}

void SplitConsole::set_console_location(int col, int row)
{
	#ifdef _WIN32
		//Use windows method to set cursor position
		COORD coord;
		coord.X = col;
		coord.Y = row;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
	#else
		//We use special escape sequences to control the cursor
		//Note: this part is probably only Linux compatible
		std::cout << "\033[" << std::to_string(row) << ";" << std::to_string(col) << "H" << std::flush;
	#endif
}

void SplitConsole::erase_screen()
{
	//Clear the whole screen
	#ifdef _WIN32
		std::string s(this->size.columns, ' ');
		set_console_location(0, 0);

		for (int i = 0; i < this->size.rows; i++)
			std::cout << s;
	#else
		//We use escape sequences for clearing screen
		std::cout << "\033[2J\033[1;1H";
	#endif
}

void SplitConsole::draw_lines()
{
	//Method for drawing the horizontal lines
	std::string s(this->size.columns, '-');
	int loc;
	for (int i = 0; i < this->split - 1; i++)
	{
		loc = this->row_end[i] + 1;
		set_console_location(0, loc);
		std::cout << s;
	}
}

std::string SplitConsole::process_string(std::string in)
{
	//Go through the passed string and remove carriage returns
	std::string s;
	for (char& c : in)
		if (c != '\n')
			s = s + c;
	
	//Check if length exceeds the console window width, if yes, truncate
	if (s.length() >= this->size.columns)
		s = s.substr(0, this->size.columns);
	else
	{
		std::string r(this->size.columns - s.length(), ' ');
		s = s + r;
	}

	return s;
}

void SplitConsole::handle_buffer(std::string in, int split, bool inp)
{
	//Check if we have reached the end of the actual split rows
	//If yes, we have to copy (move) the buffer elements one index up
	//If no, just pass the string to the buffer
	//The third argument is for distinguishing input from output splits
	//In input splits, the checking for "end is reached" is done differently
	if (inp == false)
	{
		//We check only for "bigger than"
		if (this->cursor[split] > this->row_end[split])
		{
			for (int i = this->row_start[split]; i < this->row_end[split]; i++)
			{
				this->buffer[split][i] = this->buffer[split][i + 1];
			}

			this->buffer[split][this->row_end[split]] = in;
		}
		else
		{
			this->buffer[split][this->cursor[split]] = in;
			//Increment cursor position
			this->cursor[split]++;
		}
	}
	else
	{
		//We check for "bigger than or equal"
		if (this->cursor[split] >= this->row_end[split])
		{
			for (int i = this->row_start[split]; i < this->row_end[split]; i++)
			{
				this->buffer[split][i] = this->buffer[split][i + 1];
			}

			this->buffer[split][this->row_end[split]] = in;
		}
		else
		{
			this->buffer[split][this->cursor[split]] = in;
			//Increment cursor position
			this->cursor[split]++;
		}
	}
}

void SplitConsole::flush_buffer(int split)
{
	//Send the buffer content to the console
	for (int i = this->row_start[split]; i <= this->row_end[split]; i++)
	{
		set_console_location(0, i);
		std::cout << this->buffer[split][i];
		std::cout << std::flush;
	}
}

void SplitConsole::WriteToSplitConsole(std::string in, int split)
{
	//Method for writing a line to the console window
	//Location determined with 'split' parameter and the internal cursor 
	//Lock the mutex
	this->mtx.lock();
	
	//Check if the size of the console window has changed in the meantime
	//If yes, we have to re-initialize
	dimensions new_dim = get_console_size();
	if ((this->size.columns != new_dim.columns) || (this->size.rows != new_dim.rows))
	{
		this->size.columns = new_dim.columns;
		this->size.rows = new_dim.rows;
		init_splits();
		for (int i = 0; i < this->split; i++)
			flush_buffer(i);
	}

	//Remove CR and truncate string if necessary
	std::string s = process_string(in);

	//Verify if split is ok
	if (split > this->split)
		split = this->split;

	//Handle buffer - false for output
	handle_buffer(s, split, false);

	//Send the buffer content to the console
	flush_buffer(split);
	
	//Verify if std::cin is pending
	if (this->awaiting_input == true)
		set_console_location(this->inp_cursor.columns, this->inp_cursor.rows);

	//Unlock the mutex
	this->mtx.unlock();
}

std::string SplitConsole::ReadFromSplitConsole(std::string prompt, int split)
{
	//Lock the mutex
	this->mtx.lock();

	//Verify if split is ok
	if (split > this->split)
		split = this->split;

	//Declare response
	std::string user_value = "";

	//Display the prompt
	set_console_location(0, this->cursor[split]);
	std::cout << prompt << std::flush;
	
	//Save coordinates
	this->inp_cursor.columns = prompt.length() + 1;
	this->inp_cursor.rows = this->cursor[split];
	this->awaiting_input = true;
	
	//Unlock mutex
	this->mtx.unlock();

	//Get input from user
	std::cin >> user_value;
	std::string s = process_string(prompt + user_value);

	//Lock mutex
	this->mtx.lock();

	//Handle buffer - true for input
	handle_buffer(s, split, true);
	//Write contents to console
	flush_buffer(split);

	this->awaiting_input = false;
	
	//Unlock mutex
	this->mtx.unlock();

	return user_value;
}
