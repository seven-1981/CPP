#ifndef _FCSOCKET_H
#define _FCSOCKET_H

#include <future>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include "bpm_globals.hpp"

#define BUFFER_SIZE 32
#define PORT_NO 3333
#define BACKLOG 5

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
	FCSocket();
	~FCSocket();

	//Check if socket has been initialized properly
	eError check_init();
	//Start and stop methods
	std::future<eError> start_listening();
	void stop_listening();
	//Getter method for buffer data
	void get_buffer(std::string& s);

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
	eError listening();

	//Helper function for buffer processing
	bool process_message(std::string& message);
	void check_buffer();
};

#endif