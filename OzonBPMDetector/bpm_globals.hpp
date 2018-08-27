//This file holds all globals for the famous OZON BPM counter

#ifndef _BPM_GLOBALS
#define _BPM_GLOBALS

//Define debug mode
#define DEBUG_MAIN
//#define DEBUG_GPIO
//#define DEBUG_WRITER
//#define DEBUG_FC_QUEUE
//#define DEBUG_FC_STATE
//#define DEBUG_FC_MACHINE
#define DEBUG_FUNCTIONS
//#define DEBUG_AUDIO

//Defines for Function Caller (some kind of simple operating system)
#define FC_QUEUE_SIZE		32
#define FC_QUEUE_WAIT_TIME_MS	100

//Define the home path of the project
#define FILE_PATH "/home/pi/BPM"

//Define for special split console
#define NUMBER_OF_SPLITS 4
#define SPLIT_ERRORS 	 3
#define SPLIT_FC	 2
#define SPLIT_AUDIO	 2
#define SPLIT_MAIN 	 1
#define SPLIT_INPUT	 0

//Define data transfer buffer size
#define NUM_OF_BUFFERS 4

//Enum for input and output
enum eDirection
{
	eOUT 	=	false,
	eIN	=	true
};

//Enum for errors
enum eError
{
	//Success message
	eSuccess			=	0xFF,

	//General errors
	eMemoryAllocationFailed = 0xE0,
	eNumberConversionFailure = 0xE1,
	
	//WiringPi errors
	eWiringPiNotInitialized		=	0x00,
	eGPIONumberNotValid		=	0x01,
	eGPIOConfiguredAsInput		=	0x02,
	eGPIOWriterNotInitialized	=	0x03,
	eGPIOWriterInvalidControlMode	=	0x04,
	eGPIOWriterInvalidTiming	=	0x05,
	eGPIOWriterResetInvalidOption   =       0x06,
	eGPIOWriterInvalidCharacter     =	0x07,

	//FC State errors
	eFCStateError_BindFunctionNullPtr		= 0x10,
	eFCStateError_FCStateNotInitialized		= 0x11,
	eFCStateError_FCStateNextIdNotDefined		= 0x12,
	eFCStateError_InvalidIDNumber			= 0x13,
	eFCStateError_FCMachineNotInitialized		= 0x14,

	//FC Queue errors
	eFCQueueError_QueueIsFull		= 0x20,
	eFCQueueError_QueueIsEmpty		= 0x21,
	eFCQueueError_QueueEventInvalid		= 0x22,
	eFCQueueError_InvalidIndex		= 0x23,
	eFCQueueError_FunctionNullPtr		= 0x24,
	eFCQueueError_FCEventNotInitialized	= 0x25,

	//Audio errors
	eAudio_ErrorOpeningAudioDevice  = 0x30,
	eAudio_NoSoundCardsFound	= 0x31,
	eAudio_CantOpenSoundcard	= 0x32,
	eAudio_CantGetCardInfo		= 0x33,
	eAudio_ErrorAllocatingHwStruc   = 0x34,
	eAudio_ErrorInitHwStruc		= 0x35,
	eAudio_ErrorSettingAccessMode	= 0x36,
	eAudio_ErrorSettingFormat	= 0x37,
	eAudio_ErrorSettingSampleRate	= 0x38,
	eAudio_ErrorSettingChannels	= 0x39,
	eAudio_ErrorSettingHwParameters = 0x3A,
	eAudio_ErrorActivateAudioInterf = 0x3B,
	eAudio_ErrorCapturingAudio	= 0x3C,
	eAudio_ErrorInputStreamNotOpen  = 0x3D,
	eAudio_ErrorBufferNotReady	= 0x3E,

	//Analyzer errors
	eAnalyzer_NotInitialized	= 0x50,
	eAnalyzer_NotReadyYet		= 0x51,
	
	//Data buffer errors
	eBuffer_Overflow		= 0x60,
	eBuffer_NotInitialized		= 0x61,
	eBuffer_IsFull			= 0x62,
	eBuffer_NotReady		= 0x63
};

//Enum for event IDs - the ID tells what register array index to evaluate
enum eEvents
{
	eGoToState1			=	0,
	eGoToState2			=	1,
	eGoToState3			=	2,
	eGoToState4			=	3,
	eQuit				=	4,
	eChange				=	5,
	eManMode			=	6,
	eRecordSamples			=	7,
	eCopyAudioToBuffer		=	8,
	eReadBufferReady		=	9,
	eCopyBufferToAnalyzer		=      10,
	eApplyFilter			=      11,
	eMaximizeVolume			=      12,
	eGetBPMValue			=      13,

	eLastEvent /* used for array size determination */
};

//Enum for timers
enum eTimers
{
	eTestTimer1			=	eLastEvent,
	eInitDone			=	eLastEvent + 1,
	eSupervisionTimer		=	eLastEvent + 2,
	eLastTimer /* used for array size determination */
};

//Enum for states
enum eStates
{
	eStateInit			=	0,
	eStateBPMCounter		=	1,
	eStateTimer			= 	2,
	eStateClock			=	3,
	eStateManual			=	4,
	eStateQuit			=	5,
	eNumberOfStates /* used for array size determination */
};

//Enum for event function pointer signatures
enum eSignatures
{
	//Simple functions, no arguments
	eVoidFunction,
	eIntFunction,
	eStringFunction,
	
	//Functions with arguments
	eVoidIntFunction,
	
	//Member functions, no arguments
	eBPMAudioFunction,
	eBPMAnalyzeFunction,
	eBPMAnalyzeFunctionDouble,

	//Member functions with arguments
	eBPMAnalyzepShortFunction,
	eBPMBufferpShortFunction,
	eBPMAnalyzerdoubleFunction
};

//Macros for events and timers
#define SEND_EVENT(id) appl_info.os_queue->push(*(event_info[id]))
#define SEND_INT_EVENT(id, value) event_info[id]->arguments.int_argument = value; \
appl_info.os_queue->push(*(event_info[id]))
#define SEND_PSHORT_EVENT(id, value) event_info[id]->arguments.pshort_argument = value; \
appl_info.os_queue->push(*(event_info[id]))
#define SEND_DOUBLE_EVENT(id, value) event_info[id]->arguments.double_argument = value; \
appl_info.os_queue->push(*(event_info[id]))
#define SEND_INT_DATA(id, value, clear) appl_info.os_queue->set_reg(id, value, clear)
#define SEND_STR_DATA(id, value, clear) appl_info.os_queue->set_sreg(id, value, clear)
#define SEND_DBL_DATA(id, value, clear) appl_info.os_queue->set_dreg(id, value, clear)
#define GET_AUDIO_BUFFER bpm_info.bpm_audio->flush_buffer()
#define GET_BUFFER bpm_info.bpm_buffer->get_data()
#define GET_INT_DATA(id) appl_info.os_queue->get_reg(id)
#define GET_STR_DATA(id) appl_info.os_queue->get_sreg(id)
#define GET_DBL_DATA(id) appl_info.os_queue->get_dreg(id)
#define START_TIMER(id) appl_info.os_queue->start_timer(id)
#define STOP_TIMER(id) appl_info.os_queue->stop_timer(id)
#define RESET_TIMER(id) appl_info.os_queue->reset_timer(id)

//Macros used for state machine events - handling of transitions
#define CURRENT_STATE appl_info.os_machine->set_trans(-1)
#define LOOP_OR_EXIT 	if (appl_info.os_machine->get_trans() != -1) 				\
				return StateExit;						\
			else									\
				return StateLoop;
#define NEXT_STATE return appl_info.os_machine->get_trans()

//Define characters for the 7 segmented digits
//The segments are defined as a byte (8 bits)
//          0 1
//        ------
//  5 32 /     /
//      /6 64 / 1 2
//      ------
//4 16 /     /
//    /     / 2 4
//    ------        * 7 128 (dot)
//      3 8
//
//Each of the 7 segments are represented as a bit
//LSB = "0", MSB = "*"
//We use predefined ASCII array for the conversion	
static int aChars[] =   {   0, 134,  34,  65,  73, 210,  70,  32,  41,  11,  33, 112,  16,  64, 128,  82,
//                               !    "    #    $    %    &    '    (    )    *    +    ,    -    .    /
			   63,   6,  91,  79, 102, 109, 125,   7, 127, 111,   9,  13,  97,  72,  67,  83,
//			    0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?  
			   95, 119, 127,  57,  94, 121, 113,  61, 118,  48,  30, 117,  56,  21,  55,  63,
//			    @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
			  115, 107,  51, 109, 120,  62,  58,  42,  86, 110,  91,  57, 100,  15,  35,   8,
//			    P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _
			    2,  95, 124,  88,  94, 123, 113, 111, 116,  16,  12, 117,  48,  20,  84,  92,
//			    `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
			  115, 103,  80, 109, 120,  28,  28,  20,  86, 110,  91, 105,  48,  75,  99,   0 };
//			    p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~  del

//Define address lines pins
#define ADDRESS_PIN_1	 17
#define ADDRESS_PIN_2	 27
#define ADDRESS_PIN_3	 22
#define ADDRESS_PIN_4	  5

//Define segment lines pins
#define SEGMENT_PIN_1	 18
#define SEGMENT_PIN_2	 23
#define SEGMENT_PIN_3	 24
#define SEGMENT_PIN_4	 25
#define SEGMENT_PIN_5	 12
#define SEGMENT_PIN_6	 16
#define SEGMENT_PIN_7	 20
#define SEGMENT_PIN_8	 21

//Define external reset input
#define RESET_PIN_1	  6

//Struct representing one 7seg number
struct SevenSegNum
{
	int hundreds;
	int tens;
	int ones;
	int tenths;
	float number;
};

//Struct for timing configuration
struct SevenSegTiming
{
	long int hold_addr_seg;
	long int pause_addr_seg;
	long int hold_seg_addr;
	long int pause_seg_addr;
};

//Default timing values
#define HOLD_ADDR_SEG	400
#define PAUSE_ADDR_SEG	  50
#define HOLD_SEG_ADDR	400
#define PAUSE_SEG_ADDR    50

//Time we wait after exiting prompt while loop
#define EXIT_WAIT 3000

//Time for short pause on startup
#define PAUSE_WAIT 1000

//Defines for sound card
#define PCM_DEVICE "USB"
#define PCM_SUBDEVICE 0

//Defines for audio parameters
#define PCM_BUF_SIZE       88200
#define PCM_SAMPLE_RATE    44100
#define PCM_CHANNELS 	   1
//On WIN32, the Linux alsa constants don't exist
#ifndef _WIN32
	#define PCM_AUDIO_FORMAT   SND_PCM_FORMAT_S16_LE
	#define PCM_CAPTURE_MODE   SND_PCM_STREAM_CAPTURE
	#define PCM_ACCESS_MODE    SND_PCM_ACCESS_RW_INTERLEAVED
#endif

//Defines for .wav file creation
//Note: these are depending on the audio parameters and can't be changed
//independently without messing up the capturing
//Length of header
#define PCM_WAV_FMT_LENGTH	16
//Format of .wav - 1 = canonical PCM
#define PCM_WAV_FORMAT_TAG	 1
//Sample rate of data
#define PCM_WAV_SAMPLE_RATE  44100
//Number of bits per sample
#define PCM_WAV_BITS            16
//Size of one frame = channels * (bits/sample + 7) / 8, without division rest
#define PCM_WAV_FRAME_SIZE       2
//Number of bytes per second = sample rate * frame size
#define PCM_WAV_BYTES_SEC    88200

#endif