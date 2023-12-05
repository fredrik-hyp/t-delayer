#ifndef __DELAYER_COMMON_H__
#define __DELAYER_COMMON_H__

typedef enum _process_state {
	// mc_receiver() -> disc_buffer -> discwriter()
	// discreader() *idle*
	// mc_sender() *idle*
	DELAYER_STATE_PAUSE,

//deprecated    // ALT1 mc_receiver() -> livestream_buffer -> mc_sender() -> discbuffer -> discwriter()
	// ALT2 mc_receiver() -> discbuffer -> discwriter() -> send_buffer -> mc_sender()
	DELAYER_STATE_LIVESTREAMING,

	// time to flush buffers?
	//DELAYER_STATE_RECONFIG,

	// mc_receiver() -> disc_buffer -> discwriter()
	// discreader() -> send_buffer -> mc_sender()
	DELAYER_STATE_DELAYED
} e_process_state;

#endif // __DELAYER_COMMON_H__
