#include "lv2/core/lv2.h"
#include <math.h>
#include <stdint.h>
//#include <iostream>

#include "lpc.h"

#define LPC_URI "http://example.com/plugins/lpc_plugin"

const int order = 18;

typedef enum {
	LPC_INPUT = 0,
	LPC_OUTPUT = 1
} PortIndex;

typedef struct {
	const float* input;
	float* output;
	lpc_data lpc_instance;
} LPCPlugin;

static LV2_Handle instantiate(
		const LV2_Descriptor* descriptor,
		double rate,
		const char* bundle_path,
		const LV2_Feature* const* features)
{
	LPCPlugin* lpc_plugin = (LPCPlugin*) calloc(1, sizeof(LPCPlugin));
	
	lpc_plugin->lpc_instance = lpc_create();
	
	return (LV2_Handle) lpc_plugin;
}

static void connect_port (
		LV2_Handle instance,
		uint32_t port,
		void* data)
{
	LPCPlugin* lpc_plugin = (LPCPlugin*) instance;
	
	switch ((PortIndex) port) {
		case LPC_INPUT:
			lpc_plugin->input = (const float*)data;
			break;
		case LPC_OUTPUT:
			lpc_plugin->output = (float*)data;
			break;
	}
	
}

static void activate (LV2_Handle instance)
{
	//LPCPlugin* lpc_plugin = (LPCPlugin*) instance;
	
}

static void run (
		LV2_Handle instance,
		uint32_t samples)
{
	const LPCPlugin* lpc_plugin = (const LPCPlugin*) instance;
	
	const float* const input = lpc_plugin->input;
	float* const output = lpc_plugin->output;
	
	float coefs[order];
	float power;
	float pitch;
	
	lpc_analyze(lpc_plugin->lpc_instance, (float*)input, samples, coefs, order, &power, &pitch);
	
	// NOTE: Pitch 0: whisper, other values are wavelength in samples (sample rate over frequency)
	//pitch = 0.0f;
	//std::cout << pitch << std::endl;
	
	lpc_synthesize(lpc_plugin->lpc_instance, output, samples, coefs, order, power, pitch);
}

static void deactivate (LV2_Handle instance)
{
	
}


static void cleanup (LV2_Handle instance)
{
	LPCPlugin* lpc_plugin = (LPCPlugin*) instance;
	
	lpc_destroy(lpc_plugin->lpc_instance);
	
	free(instance);
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
