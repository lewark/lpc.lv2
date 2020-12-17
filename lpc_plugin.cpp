#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "lpc.h"
#include "lpc_plugin.h"

#define MIN_BUFFER_SIZE 512
#define MAX_BUFFER_SIZE 4096

typedef struct {
	const LV2_Atom_Sequence* control;
	const float* input;
	float* output;
	float* latency;
	
	const float* order;
	const float* buffer_size;
	const float* whisper;
	const float* ola;
	const float* glottal;
	const float* preemphasis;
	const float* pitch;
	const float* tuning;
	const float* bend_range;

	
	LV2_URID midi_MidiEvent;
	
	double rate;
	
	lpc_data lpc_instance;
	
	int midi_note;
	float bender;
	
	float* inbuffer;
	float* outbuffer;
	int buffer1_pos;
	
	float* inbuffer2;
	float* outbuffer2;
	int buffer2_pos;
	
	float* window;
	
} LPCPlugin;

static float get_window_value(int i, int buffer_size) {
	return pow(sin((float)i*M_PI/(float)buffer_size),2.0f);
}

static float get_cached_window_value(LPCPlugin* self, int i, int buffer_size, int cache_size)
{
	// TODO: might want to interpolate these cached values
	return self->window[(int)((float)i*(float)cache_size/(float)buffer_size)];
}

static LV2_Handle instantiate(
		const LV2_Descriptor* descriptor,
		double rate,
		const char* bundle_path,
		const LV2_Feature* const* features)
{
	//std::cout << "LPC instantiate" << std::endl;
	LV2_URID_Map* map = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			map = (LV2_URID_Map*)features[i]->data;
			break;
		}
	}
	if (!map) {
		return NULL;
	}
	
	LPCPlugin* self = (LPCPlugin*) calloc(1,sizeof(LPCPlugin));
	
	self->lpc_instance = lpc_create();
	
	self->midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
	
	self->rate = rate;
	
	self->inbuffer = (float*)malloc(MAX_BUFFER_SIZE*sizeof(float));
	self->outbuffer = (float*)malloc(MAX_BUFFER_SIZE*sizeof(float));
	
	self->inbuffer2 = (float*)malloc(MAX_BUFFER_SIZE*sizeof(float));
	self->outbuffer2 = (float*)malloc(MAX_BUFFER_SIZE*sizeof(float));
	
	self->window = (float*)malloc(MAX_BUFFER_SIZE*sizeof(float));
	for (int i = 0; i < MAX_BUFFER_SIZE; i++)
	{
		(self->window)[i] = get_window_value(i,MAX_BUFFER_SIZE);
	}
	
	return (LV2_Handle) self;
}

static void connect_port (
		LV2_Handle instance,
		uint32_t port,
		void* data)
{
	LPCPlugin* self = (LPCPlugin*) instance;
	//std::cout << "connect" << port << " " << data << std::endl;
	
	switch ((PortIndex) port) {
		case LPC_CONTROL:
			self->control = (const LV2_Atom_Sequence*)data;
			break;
		case LPC_INPUT:
			self->input = (const float*)data;
			break;
		case LPC_OUTPUT:
			self->output = (float*)data;
			break;
		case LPC_ORDER:
			self->order = (const float*)data;
			break;
		case LPC_WHISPER:
			self->whisper = (const float*)data;
			break;
		case LPC_LATENCY:
			self->latency = (float*)data;
			break;
		case LPC_OLA:
			self->ola = (const float*)data;
			break;
		case LPC_GLOTTAL:
			self->glottal = (const float*)data;
			break;
		case LPC_PREEMPHASIS:
			self->preemphasis = (const float*)data;
			break;
		case LPC_PITCH:
			self->pitch = (const float*)data;
			break;
		case LPC_TUNING:
			self->tuning = (const float*)data;
			break;
		case LPC_BEND_RANGE:
			self->bend_range = (const float*)data;
			break;
		case LPC_BUFFER_SIZE:
			self->buffer_size = (const float*)data;
			break;
	}
	
}

static void activate (LV2_Handle instance)
{
	//std::cout << "LPC activate" << std::endl;
	
	LPCPlugin* self = (LPCPlugin*) instance;
	
	memset(self->inbuffer, 0, MAX_BUFFER_SIZE * sizeof(float));
	memset(self->outbuffer, 0, MAX_BUFFER_SIZE * sizeof(float));
	
	memset(self->inbuffer2, 0, MAX_BUFFER_SIZE * sizeof(float));
	memset(self->outbuffer2, 0, MAX_BUFFER_SIZE * sizeof(float));
	
	self->buffer1_pos = 0;
	self->buffer2_pos = 0;
	
	self->bender = 0.0f;
	self->midi_note = -1;
}

// returns wavelength in samples for lpc_synthesize
static float get_midi_pitch(LPCPlugin* self)
{
	float note = (float)self->midi_note;
	float frequency = (*self->tuning)*pow(2.0f,(note-69.0f)/12.0f);
	return ((float)self->rate)/frequency;
}

static void midi_note_on(LPCPlugin* self, uint8_t note) {
	self->midi_note = (int)note;
}

static void midi_note_off(LPCPlugin* self, uint8_t note) {
	if (self->midi_note == (int)note) {
		self->midi_note = -1;
	}
}

static void midi_bender(LPCPlugin* self, const uint8_t* const msg) {
	int bend_value = (int)msg[1] + (((int)msg[2]) << 7);
	self->bender = ((float)(bend_value - 8192)) / 8192.0f;
}

static void process_chunk(LPCPlugin* self,
						  lpc_data lpc_instance,
						  float* inbuffer,
						  float* outbuffer,
						  int buffer_size)
{
	const int order = (int) *(self->order);
	const float whisper = *(self->whisper);
	const float glottal = *(self->glottal);
	const float preemphasis = *(self->preemphasis);
	const float pitch_shift = *(self->pitch);

	float coefs[order];
	float power;
	float pitch;
	
	if (preemphasis > 0) {
		lpc_preemphasis(inbuffer, buffer_size, 0.5f);
	}
	
	lpc_analyze(lpc_instance, inbuffer,
				buffer_size, coefs, order, &power, &pitch);
	
	// NOTE: Pitch 0: whisper, other values are wavelength in samples
	// (sample rate over frequency)
	if (whisper > 0) {
		pitch = 0.0f;
	} else if (pitch > 0) {
		float new_pitch = pitch;
		
		if (self->midi_note >= 0) {
			new_pitch = get_midi_pitch(self);
		}
		
		// separate pitch shift option is provided for convenience (like
		// x42-autotune), bender is always applied in addition to pitch shift
		// to make sequencing easier
		float pitch_mult = pow(2.0f, (pitch_shift + self->bender*(*self->bend_range))/12.0f);
		
		// pitch is wavelength (1/freq), so divide by the pitch scale
		new_pitch /= pitch_mult;
		
		// decrease power as wavelength increases to correct for power scaling
		power = power * (pitch/new_pitch);
		
		pitch = new_pitch;
	}
	
	lpc_synthesize(lpc_instance, outbuffer,
					buffer_size, coefs, order, power, pitch, (int)glottal);
	
	if (preemphasis > 0) {
		lpc_deemphasis(outbuffer, buffer_size, 0.5f);
	}
}

static void run (
		LV2_Handle instance,
		uint32_t samples)
{
	LPCPlugin* self = (LPCPlugin*) instance;
	
	const float* const input = self->input;
	float* const output = self->output;
	
	float* inbuffer = (float*)self->inbuffer;
	float* outbuffer = (float*)self->outbuffer;
	
	float* inbuffer2 = (float*)self->inbuffer2;
	float* outbuffer2 = (float*)self->outbuffer2;
	
	float* window = (float*)self->window;
	
	int* buffer1_pos = &(self->buffer1_pos);
	int* buffer2_pos = &(self->buffer2_pos);
	float* latency = self->latency;
	
	const float* ola = self->ola;
	
	int buffer_size = std::min(std::max((int)(*self->buffer_size),
		MIN_BUFFER_SIZE),MAX_BUFFER_SIZE);
	
	if (self->control != NULL) {
		LV2_ATOM_SEQUENCE_FOREACH(self->control, ev) {
			if (ev->body.type == self->midi_MidiEvent) {
				const uint8_t* const msg = (const uint8_t*)(ev + 1);
				switch (lv2_midi_message_type(msg)) {
					case LV2_MIDI_MSG_NOTE_ON:
						if (msg[2] == 0) {
							midi_note_off(self,msg[1]);
						} else {
							midi_note_on(self,msg[1]);
						}
						break;
					case LV2_MIDI_MSG_NOTE_OFF:
						midi_note_off(self,msg[1]);
						break;
					case LV2_MIDI_MSG_BENDER:
						midi_bender(self,msg);
						break;
					default: break;
				}
			}
		}
	}
	
	for (int i = 0; i < samples; i++) {
		
		(*buffer1_pos) = (*buffer1_pos) % buffer_size;
		(*buffer2_pos) = ((*buffer1_pos) + buffer_size/2) % buffer_size;
		
		if (*buffer1_pos == 0) {
			process_chunk(self, self->lpc_instance, inbuffer, outbuffer, buffer_size);
		}
		if (*buffer2_pos == 0 && *ola > 0.0f) {
			process_chunk(self, self->lpc_instance, inbuffer2, outbuffer2, buffer_size);
		}
		
		if (*ola > 0) {
			float ola_mult1 = get_cached_window_value(
				self,*buffer1_pos,buffer_size,MAX_BUFFER_SIZE);
			float ola_mult2 = get_cached_window_value(
				self,*buffer2_pos,buffer_size,MAX_BUFFER_SIZE);
			
			inbuffer[*buffer1_pos] = input[i]; // * ola_mult1;
			inbuffer2[*buffer2_pos] = input[i]; // * ola_mult2;
			
			output[i] = (outbuffer[*buffer1_pos] * ola_mult1 +
						outbuffer2[*buffer2_pos] * ola_mult2);
		}
		else {
			inbuffer[*buffer1_pos] = input[i];
			inbuffer2[*buffer2_pos] = 0.0f;
			
			output[i] = outbuffer[*buffer1_pos];
		}
		
		(*buffer1_pos)++;
	}
	
	*latency = (float)buffer_size;
}

static void deactivate (LV2_Handle instance)
{
	//std::cout << "LPC deactivate" << std::endl;
	
	//const LPCPlugin* self = (const LPCPlugin*) instance;
	
}


static void cleanup (LV2_Handle instance)
{
	//std::cout << "LPC cleanup" << std::endl;
	
	LPCPlugin* self = (LPCPlugin*) instance;
	
	lpc_destroy(self->lpc_instance);
	
	free(self->inbuffer);
	free(self->outbuffer);
	free(self->inbuffer2);
	free(self->outbuffer2);
	
	free(self->window);
	
	free(self);
}

static const void* extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	LPC_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch(index) {
		case 0: return &descriptor;
		default: return NULL;
	}
}
