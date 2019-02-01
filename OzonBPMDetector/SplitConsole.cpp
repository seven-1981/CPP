#include <iostream>
#include <string>
#include <cmath>
#include <stdio.h>
#include <thread>
#include <chrono>
#include <fstream>
#include "SplitConsole.hpp"
#include "bpm_param.hpp"

//Extern parameter list
extern ParamList param_list;

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
		this->linenumber[i] = 0;

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

	//Line numbers at left side must be considered
	//Line number: '00000> ' length 7
	int width = this->size.columns - (floor(log10(MAX_LINENBR)) + 3);
	
	//Check if length exceeds the console window width, if yes, truncate
	if (s.length() >= width)
		s = s.substr(0, width);
	else
	{
		std::string r(width - s.length(), ' ');
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

void SplitConsole::flush_buffer(int split, bool read)
{
	//Get current line number
	int linenbr = this->linenumber[split];
	int count = 0;

	//Send the buffer content to the console
	for (int i = this->row_start[split]; i <= this->row_end[split]; i++)
	{
		//Build linenumber
		int nbr = linenbr + (count++);
		nbr %= MAX_LINENBR;

		//Fill with leading zeros
		int width_max = floor(log10(MAX_LINENBR));
		int zeros = 0;
		if (nbr != 0)
			zeros = width_max - floor(log10(nbr));
		else
			zeros = 4;

		//Build zeros string
		std::string z = "";
		for (int j = 0; j < zeros; j++)
			z += "0";
			
		//Create string to display
		std::string s = z + std::to_string(nbr) + "> ";

		//If in read mode, no linenumber must be displayed
		if (read == true)
			s = "";

		//If nothing in buffer, no linenumber must be displayed
		if (this->buffer[split][i] == "")
			s = "";		
		
		set_console_location(0, i);
		std::cout << s << this->buffer[split][i];
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
			flush_buffer(i, false);
	}

	//Remove CR and truncate string if necessary
	std::string s = process_string(in);

	//Verify if split is ok
	if (split > this->split)
		split = this->split;

	//Handle logging into file
	log_message(s, split);

	//Handle buffer - false for output
	handle_buffer(s, split, false);

	//Increase line number, if end of split reached
	if (this->cursor[split] >= this->row_end[split])
		this->linenumber[split]++;

	//Send the buffer content to the console
	flush_buffer(split, false);
	
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
	//Increase line number, if end of split reached
	if (this->cursor[split] >= this->row_end[split])
		this->linenumber[split]++;
	//Write contents to console
	flush_buffer(split, true);

	this->awaiting_input = false;
	
	//Unlock mutex
	this->mtx.unlock();

	return user_value;
}

void SplitConsole::log_message(std::string& message, int split)
{
	if (param_list.get<bool>("split logging") == true)
	{
		//Create logfile name
		std::string fn = "console_log_" + std::to_string(split) + ".txt";
		//Open file for output
		std::ofstream fs(fn, std::ios_base::ate | std::ios_base::app);
		if (fs.is_open() == false)
			return;
		//Get timestamp
		std::string msg = "";
		create_timestamp(msg);
		//Build message
		msg = "<" + msg + "> " + message;
		//Write log message to file
		fs << msg << "\n";
		//Close file handle
		fs.close();
	}
}

void SplitConsole::create_timestamp(std::string& msg)
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();

	typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<8>>::type> Days; /* UTC: +8:00 */

	Days days = std::chrono::duration_cast<Days>(duration);
	duration -= days;
	auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;
	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	duration -= seconds;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	duration -= milliseconds;
	auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
	duration -= microseconds;
	auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
	
	std::string h = std::to_string(hours.count() + 17 % 24);
	if (h.length() == 1)
		h = "0" + h;
	std::string m = std::to_string(minutes.count());
	if (m.length() == 1)
		m = "0" + m;
	std::string s = std::to_string(seconds.count());
	if (s.length() == 1)
		s = "0" + s;
	std::string ms = std::to_string(milliseconds.count());
	if (ms.length() == 1)
		ms = "00" + ms;
	else if (ms.length() == 2)
		ms = "0" + ms;
	
	msg += h; msg += ":";
	msg += m; msg += ":";
	msg += s; msg += ".";
	msg += ms;
}