#include "FCQueue.hpp"
#include "FCEvent.hpp"
#include "SplitConsole.hpp"
#include "bpm_param.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <future>
#include <type_traits>

//Extern split console instance
extern SplitConsole my_console;
//Extern parameter list
extern ParamList param_list;

FCQueue::FCQueue()
{
	//Constructor
	FCEvent e;
	FCTimer t;

	//Set the software interrupt routine to null pointer
	this->SIR = nullptr;

	//Initialize all queue elements and the register
	for (int i = 0; i < FC_QUEUE_SIZE; i++)
	{
		this->queue[i] = e;
	}

	//Initialize all register elements
	for (int i = 0; i < (eLastTimer - 1); i++)
	{
		this->reg[i] = 0;
		this->sreg[i] = "";
		this->dreg[i] = 0.0;
		this->clear[i] = true;
	}

	//Initialize all timers
	for (int i = 0; i < (eLastTimer - eLastEvent); i++)
	{
		this->timers[i]  = t;
	}

	//Set read and write pointers
	this->read_index = 0;
	this->write_index = 0;

	//Set flags
	this->is_full = false;
	this->is_empty = true;
	this->stop = false;
	this->toggle = false;

	//Save starting time point for timers
	this->start = std::chrono::high_resolution_clock::now();

	//Write debug line
	if (param_list.get<bool>("debug queue") == true)
		my_console.WriteToSplitConsole("Created and initialized FC queue.", param_list.get<int>("split main"));
}

FCQueue::~FCQueue()
{
	//Destructor
	//Write debug line
	if (param_list.get<bool>("debug queue") == true)
		my_console.WriteToSplitConsole("Removed FC queue.", param_list.get<int>("split main"));
}

eError FCQueue::push(FCEvent e)
{
	//We check if queue is already full
	if (this->is_full == true)
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_QueueIsFull", param_list.get<int>("split errors"));

		return eFCQueueError_QueueIsFull;
	}

	//Lock mutex - lock causes wait if already locked
	this->mtx_queue.lock();

	//We push the event into the queue
	this->queue[this->write_index] = e;

	//Write debug line
	if (param_list.get<bool>("debug queue") == true)
		my_console.WriteToSplitConsole("Pushed event into queue, ID = " + std::to_string(e.ID), param_list.get<int>("split main"));

	//Reset the empty flag
	this->is_empty = false;

	//Increase index - wrap around after queue size
	this->write_index++;
	if (this->write_index >= FC_QUEUE_SIZE)
	{
		this->write_index = 0;
	}

	//Check if queue is full
	//Happens, if write pointer catches read pointer
	if (this->write_index == this->read_index)
	{
		this->is_full = true;

		//Write debug line
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("Warning! Queue is full!", param_list.get<int>("split errors"));
	}

	//Release mutex
	this->mtx_queue.unlock();

	return eSuccess;
}

FCEvent FCQueue::pop()
{
	//Generate return value
	FCEvent retval;

	//We check if the queue is empty
	if (this->is_empty == true)
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_QueueIsEmpty", param_list.get<int>("split errors"));
		
		//We return error since there's nothing to pop
		retval.success = false;
		return retval;
	}

	//Lock mutex - lock causes wait if already locked
	this->mtx_queue.lock();

	//We generate the return value
	retval = this->queue[this->read_index];

	if (param_list.get<bool>("debug queue") == true)
		my_console.WriteToSplitConsole("Popped event from queue, ID = " + std::to_string(retval.ID), param_list.get<int>("split main"));

	//Reset the full flag
	this->is_full = false;

	//Increase index - wrap around after queue size
	this->read_index++;
	if (this->read_index >= FC_QUEUE_SIZE)
	{
		this->read_index = 0;
	}

	//Check if queue is empty
	//Happens, if read pointer catches write pointer
	if (this->write_index == this->read_index)
	{
		this->is_empty = true;

		//Write debug line
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("Warning! Queue is empty!", param_list.get<int>("split errors"));
	}

	//Release mutex
	this->mtx_queue.unlock();

	//Return the event
	return retval;
}

bool FCQueue::check_full()
{
	return this->is_full;
}

bool FCQueue::check_empty()
{
	return this->is_empty;
}

int FCQueue::get_reg(int id)
{
	//We check if id is valid
	if (id < 0 || id >= (eLastTimer - 1))
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		//We just return 0, if the ID is not defined
		return 0;
	}

	int retval = this->reg[id];

	//Clear the register or not
	if (this->clear[id] == true)
		this->reg[id] = 0;

	return retval;
}

std::string FCQueue::get_sreg(int id)
{
	//We check if id is valid
	if (id < 0 || id >= (eLastTimer - 1))
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		//We just return empty string, if the ID is not defined
		return "";
	}

	std::string retval = this->sreg[id];

	//Check clear flag
	if (this->clear[id] == true)
		this->sreg[id] = "";

	return retval;
}

double FCQueue::get_dreg(int id)
{
	//We check if id is valid
	if (id < 0 || id >= (eLastTimer - 1))
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		//We just return 0.0, if the ID is not defined
		return 0.0;
	}

	double retval = this->dreg[id];

	//Check clear flag
	if (this->clear[id] == true)
		this->dreg[id] = 0.0;

	return retval;
}

eError FCQueue::set_reg(int id, int value, bool clear)
{
	//We check if id is valid
	if (id < 0 || id >= (eLastTimer - 1))
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		return eFCQueueError_InvalidIndex;
	}

	//Lock mutex
	this->mtx_queue.lock();

	//Set the desired register
	this->reg[id] = value;

	//Set the clear flag
	this->clear[id] = clear;

	//Unlock mutex
	this->mtx_queue.unlock();

	return eSuccess;
}

eError FCQueue::set_sreg(int id, std::string value, bool clear)
{
	//We check if id is valid
	if (id < 0 || id >= (eLastTimer - 1))
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		return eFCQueueError_InvalidIndex;
	}

	//Lock mutex
	this->mtx_queue.lock();

	//Set the desired register
	this->sreg[id] = value;

	//Set the clear flag
	this->clear[id] = clear;

	//Unlock mutex
	this->mtx_queue.unlock();

	return eSuccess;
}

eError FCQueue::set_dreg(int id, double value, bool clear)
{
	//We check if id is valid
	if (id < 0 || id >= (eLastTimer - 1))
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		return eFCQueueError_InvalidIndex;
	}

	//Lock mutex
	this->mtx_queue.lock();

	//Set the desired register
	this->dreg[id] = value;

	//Set the clear flag
	this->clear[id] = clear;

	//Unlock mutex
	this->mtx_queue.unlock();

	return eSuccess;
}

void FCQueue::queue_stop()
{
	//Lock the mutex
	this->mtx_queue.lock();

	if (param_list.get<bool>("debug queue") == true)
		my_console.WriteToSplitConsole("Stopping queue!", param_list.get<int>("split main"));	

	//Set the stop flag to true
	this->stop = true;

	//Unlock the mutex
	this->mtx_queue.unlock();
}

eError FCQueue::queue_loop()
{
	//Event for evaluation
	FCEvent eval_event;
	eError retval = eSuccess;

	//Operating system main loop
	//Using this condition, we could ensure that all the queued events are executed
	//before stopping, but this should not be necessary
	while (this->stop == false && retval == eSuccess)
	{
		//Check if there's something in the queue
		if (this->check_empty() != true)
		{
			//Read the event from the queue
			//Should always return success, but you never know...
			eval_event = this->pop();

			//Check if it was popped successfully, if not, the queue was empty
			//or an event was popped that was not properly initialized
			if (eval_event.success == false)
			{
				retval = eFCQueueError_FCEventNotInitialized;
				//We jump to the end of this while loop iteration
				continue;
			}

			//Evaluate event info
			//Determine the type of function to be executed
			retval = eval_obj(eval_event);

			if (param_list.get<bool>("debug queue") == true)
			{
				int disp_index = this->write_index - this->read_index;
				if (disp_index < 0)
					disp_index = FC_QUEUE_SIZE + disp_index;
				my_console.WriteToSplitConsole("# of queue elements: " + std::to_string(disp_index), param_list.get<int>("split main"));
			
				std::this_thread::sleep_for(std::chrono::milliseconds(PAUSE_WAIT));
			}
		}

		//Event executed - toggle supervision bit
		if (this->toggle == false)
			this->toggle = true;
		else
			this->toggle = false;

		//Wait for a short amount of time - not necessary to have lightspeed queue handling
		std::this_thread::sleep_for(std::chrono::milliseconds(FC_QUEUE_WAIT_TIME_MS));
	}

	//If this loop ends, it means that there went something wrong inside or a stop command was issued
	//This way, the executing function can verify, if it was an exceptional termination or a normal shutdown
	return retval;
}

eError FCQueue::SIR_loop()
{
	//Declare return value
	eError retval = eSuccess;

	//Operating system SIR (software interrupt routine) loop
	while (this->stop == false && retval == eSuccess)
	{
		//Handle the timers
		this->handle_timers();

		//Handle software interrupts
		this->handle_SIR();

		//Wait for a short amount of time - not necessary to have lightspeed queue handling
		std::this_thread::sleep_for(std::chrono::milliseconds(FC_QUEUE_WAIT_TIME_MS));
	}

	//If this loop ends, it means that there went something wrong inside or a stop command was issued
	//This way, the executing function can verify, if it was an exceptional termination or a normal shutdown
	return retval;
}

std::future<eError> FCQueue::start_queue()
{
	//It is not possible to return values from thread functions
	//Therefore, we use a std::async. This basically launches a thread for us and provides
	//a std::future object. After termination of std::async, the future object holds the return value
	//The return value is of type 'eError'
	//We can read the return value in main using the member function get() (of the future object)
	std::future<eError> result = std::async(std::launch::async, &FCQueue::queue_loop, this);
	return result;
}

std::future<eError> FCQueue::start_SIR()
{
	//It is not possible to return values from thread functions
	//Therefore, we use a std::async. This basically launches a thread for us and provides
	//a std::future object. After termination of std::async, the future object holds the return value
	//The return value is of type 'eError'
	//We can read the return value in main using the member function get() (of the future object)
	std::future<eError> result = std::async(std::launch::async, &FCQueue::SIR_loop, this);
	return result;
}

eError FCQueue::config_timer(int id, FCTimer timer)
{
	//We check if id is valid
	if (id < eLastEvent || id >= eLastTimer)
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		return eFCQueueError_InvalidIndex;
	}

	//Check if timer is properly configured
	if (timer.success == false)
		return eFCQueueError_FCEventNotInitialized;

	//Lock mutex
	this->mtx_timers.lock();

	//We push the timer into the timer array, so it will be
	//perodically checked by the queue loop. If the timer elapses,
	//the underlying function is executed.
	this->timers[id - eLastEvent] = timer;

	//Release mutex
	this->mtx_timers.unlock();

	return eSuccess;
}

eError FCQueue::start_timer(int id)
{
	//We check if id is valid
	if (id < eLastEvent || id >= eLastTimer)
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		return eFCQueueError_InvalidIndex;
	}

	//Store starting time point
	if (this->timers[id - eLastEvent].started == false)
	{
		//Lock mutex
		this->mtx_timers.lock();

		auto elapsed = std::chrono::high_resolution_clock::now() - start;
		long long us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		this->timers[id - eLastEvent].start_value = us;

		//Start the timer
		this->timers[id - eLastEvent].started = true;

		//Release mutex
		this->mtx_timers.unlock();
	}

	return eSuccess;
}

eError FCQueue::stop_timer(int id)
{
	//We check if id is valid
	if (id < eLastEvent || id >= eLastTimer)
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		return eFCQueueError_InvalidIndex;
	}

	//Check if timer has been started - if yes, stop it
	if (this->timers[id - eLastEvent].started == true)
	{
		//Lock mutex
		this->mtx_timers.lock();

		//Stop the timer
		this->timers[id - eLastEvent].started = false;

		//Release mutex
		this->mtx_timers.unlock();
	}

	return eSuccess;
}

eError FCQueue::reset_timer(int id)
{
	//We check if id is valid
	if (id < eLastEvent || id >= eLastTimer)
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_InvalidIndex", param_list.get<int>("split errors"));

		return eFCQueueError_InvalidIndex;
	}

	//Check if timer has been started - if yes, reset it
	if (this->timers[id - eLastEvent].started == true)
	{
		//Lock mutex
		this->mtx_timers.lock();

		//Set the starting value to zero, it will be reset upon timer start
		this->timers[id - eLastEvent].start_value = 0;

		//Stop the timer
		this->timers[id - eLastEvent].started = false;

		//Release mutex
		this->mtx_timers.unlock();
	}

	return eSuccess;
}

void FCQueue::handle_timers()
{
	//Check if any timer is set up and if yes, if the timers are elapsed
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	long long us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

	//Lock mutex
	this->mtx_timers.lock();

	//Go through the list of timers and execute elapsed
	for (int i = 0; i < (eLastTimer - eLastEvent); i++)
	{
		if (timers[i].success == true && timers[i].started == true)
		{
			//Check if elapsed
			if ((us - timers[i].start_value) > timers[i].timer_value)
			{
				timers[i].timer_value = 0;
				timers[i].start_value = 0;
				timers[i].started = false;
				
				//Check which function is configured and execute it
				//Here, we ignore the return value
				eval_obj(timers[i]);
			}
		}
	}

	//Unlock mutex
	this->mtx_timers.unlock();
}

eError FCQueue::set_SIR(void (*function)())
{
	//Method for setting the software interrupt routine
	//We check if function is valid
	if (function == nullptr)
	{
		if (param_list.get<bool>("debug queue") == true)
			my_console.WriteToSplitConsole("FCQueueError_FunctionNullPtr", param_list.get<int>("split errors"));

		return eFCQueueError_FunctionNullPtr;
	}

	//Set the software interrupt routine
	this->SIR = function;
	
	return eSuccess;
}

void FCQueue::handle_SIR()
{
	//If the SIR is null, we do not execute anything
	//Trigger software interrupt routine
	if (this->SIR == nullptr)
	{
		//We don't do anything
	}
	else
	{
		//Trigger software interrupt routine
		this->SIR();
	}
}

template <typename T>
eError FCQueue::eval_obj(T obj)
{
	//Declare return value
	eError retval = eSuccess;

	//Evaluate event info
	//Determine the type of function to be executed
	switch (obj.select)
	{
		case eVoidFunction:
			this->clear[obj.ID] = obj.clear;
			this->reg[obj.ID] = -1;
			obj.functions.void_function();
			break;
		case eIntFunction:
			this->clear[obj.ID] = obj.clear;
			this->reg[obj.ID] = obj.functions.int_function();
			break;
		case eStringFunction:
			this->clear[obj.ID] = obj.clear;
			this->sreg[obj.ID] = obj.functions.string_function();
			break;

		case eVoidIntFunction:
			this->clear[obj.ID] = obj.clear;
			this->reg[obj.ID] = -1;
			obj.functions.void_int_function(obj.arguments.int_argument);
			break;

		case eBPMAudioFunction:
			this->clear[obj.ID] = obj.clear;
			this->reg[obj.ID] = (*obj.instances.audio_instance.*(obj.functions.audio_function))();
			break;
		case eBPMAnalyzeFunction:
			this->clear[obj.ID] = obj.clear;
			this->reg[obj.ID] = (*obj.instances.analyze_instance.*(obj.functions.analyze_function))();
			break;
		case eBPMAnalyzeFunctionDouble:
			this->clear[obj.ID] = obj.clear;
			this->dreg[obj.ID] = (*obj.instances.analyze_instance.*(obj.functions.double_analyze_function))();
			break;

		case eBPMAnalyzepShortFunction:
			this->clear[obj.ID] = obj.clear;
			this->reg[obj.ID] = (*obj.instances.analyze_instance.*(obj.functions.analyze_pshort_function))(obj.arguments.pshort_argument);
			break;		
		case eBPMAnalyzerdoubleFunction:
			this->clear[obj.ID] = obj.clear;
			this->reg[obj.ID] = (*obj.instances.analyze_instance.*(obj.functions.analyze_double_function))(obj.arguments.double_argument);
			break;

		default:
			//This is an error
			if (param_list.get<bool>("debug queue") == true)
				my_console.WriteToSplitConsole("FCQueueError_QueueEventInvalid", param_list.get<int>("split main"));
			retval = eFCQueueError_QueueEventInvalid;
		break;
	}

	return retval;
}