#ifndef _FCQUEUE_H
#define _FCQUEUE_H

#include "bpm_globals.hpp"
#include "bpm_audio.hpp"
#include "bpm_analyze.hpp"
#include "FCState.hpp"
#include "FCEvent.hpp"
#include "FCTimer.hpp"
#include <thread>
#include <mutex>
#include <string>
#include <future>

//Class for simple queue
class FCQueue
{
private:
	//Queue of events
	FCEvent queue[FC_QUEUE_SIZE];
	//Indices for access
	int read_index;
	int write_index;
	//If queue is full, this flag is set
	bool is_full;
	//If queue is empty, this flag is set
	bool is_empty;
	//If queue has to be stopped, set this flag
	bool stop;
	//Register for method feedbacks
	int reg[eLastTimer];
	//String register
	std::string sreg[eLastTimer];
	//Double register
	double dreg[eLastTimer];
	//Clear flags
	bool clear[eLastTimer];
	//Timers
	FCTimer timers[eLastTimer - eLastEvent];
	std::chrono::high_resolution_clock::time_point start;
	//SIR - software interrupt routine
	void (*SIR)();
	
	//Mutexes for thread sync
	std::mutex mtx_queue;
	std::mutex mtx_timers;

	//Private methods - aux method for start_queue
	eError queue_loop();
	eError SIR_loop();

	//Handle timers
	void handle_timers();

	//Handle software interrupts
	void handle_SIR();

public:
	//Constructor
	FCQueue();
	//Destructor
	~FCQueue();

	//Methods for populating queue
	eError push(FCEvent e);
	FCEvent pop();

	//Getter and setter methods
	bool check_empty();
	bool check_full();
	int get_reg(int id);
	std::string get_sreg(int id);
	double get_dreg(int id);
	eError set_reg(int id, int value, bool clear = true);
	eError set_sreg(int id, std::string value, bool clear = true);
	eError set_dreg(int id, double value, bool clear = true);

	//Stop method
	void queue_stop();

	//Static methods (doesn't require instance)
	std::future<eError> start_queue();
	std::future<eError> start_SIR();

	//Create timer method
	eError config_timer(int id, FCTimer timer);
	//Start timer
	eError start_timer(int id);
	//Stop timer
	eError stop_timer(int id);
	//Reset timer
	eError reset_timer(int id);

	//Set software interrupt routine
	eError set_SIR(void(*function)());

	//Evaluate the event
	//Check the function pointer and execute - template method
	//is used for both, FCEvent and derived FCTimer
	template <typename T>
	eError eval_obj(T obj);

	//Toggle bit for execution supervision
	bool toggle;
};	

#endif


