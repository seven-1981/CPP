#ifndef _FCSOCKET_H
#define _FCSOCKET_H

//No Socket on windows
#ifndef _WIN32

#include <iostream>
#include <future>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include "bpm_globals.hpp"
#include "bpm_param.hpp"
#include "SplitConsole.hpp"

#define BUFFER_SIZE 32
#define PORT_NO 3333
#define BACKLOG 5

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

//Enum for state machine
enum FCSocketState
{
	FCSocket_Init	=	0,
	FCSocket_Accept,
	FCSocket_Read,
	FCSocket_Response,
	FCSocket_Disconnect
	
};

class FCSocket
{
public:
	FCSocket()
	{
		//Set init flags
		this->initialized = true;
		this->stop = false;
		this->state = FCSocket_Init;
		this->data = "";
		this->mtx = false;

		//Open socket
		my_console.WriteToSplitConsole("Opening socket...", param_list.get<int>("split main"));
		this->sockfd = socket(AF_INET, SOCK_STREAM, 0);

		//Check if socket created successfully
		if (this->sockfd < 0)
		{
			this->initialized = false;
			my_console.WriteToSplitConsole("Error opening socket! " + std::to_string(this->sockfd), param_list.get<int>("split errors"));
		}

		//Initialize server address and port
		bzero((char*) &this->serv_addr, sizeof(this->serv_addr));
		this->portno = PORT_NO;
		this->serv_addr.sin_family = AF_INET;
		this->serv_addr.sin_addr.s_addr = INADDR_ANY;
		this->serv_addr.sin_port = htons(this->portno);

		//Bind socket to address/port
		int bind_retval = bind(this->sockfd, (sockaddr*) &this->serv_addr, sizeof(this->serv_addr));
		if (bind_retval < 0)
		{
			this->initialized = false;
			my_console.WriteToSplitConsole("Error binding socket! " + std::to_string(bind_retval), param_list.get<int>("split errors"));
		}

		//Start listening on socket
		listen(this->sockfd, BACKLOG);
	}

	~FCSocket()
	{

	}

	eError check_init()
	{
		eError retval = eSocketCreationFailed;
		if (this->initialized == true)
			retval = eSuccess;
		return retval;
	}

	std::future<eError> start_listening()
	{
		//Start thread
		std::future<eError> result = std::async(std::launch::async, &FCSocket::listening, this);
		return result;
	}

	void stop_listening()
	{
		//Set stop flag
		this->stop = true;

		//Close connections
		shutdown(this->newsockfd, SHUT_RD);
		shutdown(this->sockfd, SHUT_RD);
		close(this->newsockfd);
		close(this->sockfd);
	}

	void get_buffer(std::string& s)
	{
		//Check mutex status
		while (this->mtx == true)
			//do nothing...
		this->mtx = true;
		s = this->data;
		this->data = "";
		this->mtx = false;
	}

private:
	//Socket handle
	int sockfd;
	//Address for server and client
	sockaddr_in serv_addr, cli_addr;
	//Port number
	int portno;
	//Socket for incoming data
	int newsockfd;
	socklen_t clilen;
	//Buffer for data
	char buffer[BUFFER_SIZE];

	//Initialized flag
	bool initialized;
	//Flag for loop
	bool stop;

	//Internal state machine
	FCSocketState state;

	//Data for outside users
	std::string data;
	//Mutex for protecting data
	bool mtx;

	//Char* for parameter response
	std::string response_ok = "- OK -\r\n";
	std::string response_not_ok = "- NOK -\r\n";
	std::string response_welcome = "- - - BPM-COUNTER - welcome - - -\r\n";

	//Loop function for TCP connections
	eError listening()
	{
		//Check proper initialization
		eError retval = check_init();
		if (retval != eSuccess)
			return retval;

		//Return values
		int read_retval, write_retval;
		//Message
		std::string message;
		bool cmd_ok;

		while (this->stop == false && retval == eSuccess)
		{
			switch(this->state)
			{
				case FCSocket_Init:
					this->state = FCSocket_Accept;
					break;

				case FCSocket_Accept:
					//Accept incoming connection
					//Accept blocks thread until connection happens from client
					this->clilen = sizeof(this->cli_addr);
					this->newsockfd = accept(this->sockfd, (sockaddr*) &this->cli_addr, &this->clilen);
					if (this->newsockfd < 0)
					{
						my_console.WriteToSplitConsole("Error on accept! " + std::to_string(this->newsockfd), param_list.get<int>("split errors"));
						retval = eSocketOperationFailed;
					}
					else
					{
						//Connection accepted
						//Send welcome message
						write_retval = write(this->newsockfd, this->response_welcome.c_str(), this->response_welcome.length());
						if (write_retval < 0)
						{
							my_console.WriteToSplitConsole("Error writing to socket!", param_list.get<int>("split errors"));
							retval = eSocketOperationFailed;
						}
						else
						{
							my_console.WriteToSplitConsole("Client connected.", param_list.get<int>("split main"));
							this->state = FCSocket_Read;
						}
					}
					break;

				case FCSocket_Read:
					//Set buffer to zero
					bzero(this->buffer, BUFFER_SIZE);
			
					//Read incoming data
					//Read blocks until data is available
					read_retval = read(this->newsockfd, this->buffer, BUFFER_SIZE - 1);
					if (read_retval < 0)
					{
						my_console.WriteToSplitConsole("Error reading from socket!", param_list.get<int>("split errors"));
						retval = eSocketOperationFailed;
					}
					else if (read_retval == 0)
					{
						my_console.WriteToSplitConsole("Disconnect.", param_list.get<int>("split main"));
						
						//Set next state
						this->state = FCSocket_Disconnect;
					}
					else	
					{
						check_buffer();
						message = std::string(this->buffer);
						my_console.WriteToSplitConsole("Socket message: " + message, param_list.get<int>("split main"));
					
						//Process the message
						cmd_ok = process_message(message);

						//Provide data to outside user
						//If read access is ongoing, we wait...
						while (this->mtx == true)
							//do nothing...
						this->mtx = true;
						this->data = message;
						this->mtx = false;

						//Set next state
						this->state = FCSocket_Response;
					}
					break;

				case FCSocket_Response:
					//Send back response
					if (cmd_ok == true)
						write_retval = write(this->newsockfd, this->response_ok.c_str(), this->response_ok.length());
					else
						write_retval = write(this->newsockfd, this->response_not_ok.c_str(), this->response_not_ok.length());
					
					if (write_retval < 0)
					{
						my_console.WriteToSplitConsole("Error writing to socket!", param_list.get<int>("split errors"));
						retval = eSocketOperationFailed;
					}
					else
					{
						//Set next state
						this->state = FCSocket_Read;
					}
					break;

				case FCSocket_Disconnect:
					//Client disconnected
					close(this->newsockfd);

					//Set next state
					this->state = FCSocket_Accept;
					break;
			}
			
		}

		return retval;
	}

	bool process_message(std::string& message)
	{
		//Command format:
		//'p:[param_name], [value]'
		//Check if known command
		std::string cmd = message.substr(0, 2);
		//p command - set parameter
		if (cmd == "p:")
		{
			//Extract parameter
			std::string param_val = message.substr(2, message.length() - 2);		
			//Find the comma
			std::size_t pos = param_val.find(",");
			if (pos == std::string::npos)
				return false;
	
			//Get the parameter name
			std::string param = param_val.substr(0, pos);
			//Get the value
			std::string value = param_val.substr(pos + 1, param_val.length() - pos);
			//Check if known parameter
			if (param_list.valid(param) == false)
				return false;
	
			//Double value used to extract value from string
			double val;
			try
			{
				val = std::stod(value);
			}
			catch (...)
			{
				val = 0.0;
				return false;
			}
	
			//Set parameter
			//Get underlying data type
			Type type = param_list.get_type(param);
			switch(type)
			{
				case TypeBOOL:
					if (param_list.set<bool>(param, (bool)val) == false)
						return false;
					break;
				case TypeINT:
					if (param_list.set<int>(param, (int)val) == false)
						return false;
					break;
				case TypeDOUBLE:
					if (param_list.set<double>(param, (double)val) == false)
						return false;
					break;
				default:
					break;
			}

			this->response_ok = "- PARAM OK -\r\n";
			return true;
		}
		//g command - get parameter
		else if (cmd == "g:")
		{
			//Extract parameter
			std::string param = message.substr(2, message.length() - 2);		
			//Check if known parameter
			if (param_list.valid(param) == false)
				return false;
	
			//Get parameter
			//Get underlying data type
			std::string param_response;
			Type type = param_list.get_type(param);
			switch(type)
			{
				case TypeBOOL:
					param_response = std::to_string(param_list.get<bool>(param));
					break;
				case TypeINT:
					param_response = std::to_string(param_list.get<int>(param));
					break;
				case TypeDOUBLE:
					param_response = std::to_string(param_list.get<double>(param));
					break;
				default:
					break;
			}

			this->response_ok = param_response + "\r\n";

			return true;
		}
		//c command - execute command
		else if (cmd == "c:")
		{
			//Extract parameter
			std::string command = message.substr(2, message.length() - 2);		
			//Check if known command
			std::string command2 = command.substr(0, 5);
			if (command != "quit" && command != "restart" && command != "reboot"
			 && command2 != "state")
				return false;
	
			//Set message
			message = command;			

			this->response_ok = "- CMD OK -\r\n";

			return true;
		}

		return false;
	}

	void check_buffer()
	{
		//Scan through buffer and remove nonprintable characters
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			if (!std::isprint(this->buffer[i]))
			{
				this->buffer[i] = '\0';
				break;
			}
		}
	}
};

#endif

#endif