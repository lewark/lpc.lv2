#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

typedef enum {
	LPC_CONTROL=0,
	LPC_NOTIFY=1,
	LPC_INPUT=2,
	LPC_OUTPUT=3,
	LPC_LATENCY=4,
	LPC_ORDER=5,
	LPC_BUFFER_SIZE=6,
	LPC_PITCH=7,
	LPC_TUNING=8,
	LPC_BEND_RANGE=9,
	LPC_WHISPER=10,
	LPC_OLA=11,
	LPC_GLOTTAL=12,
	LPC_PREEMPHASIS=13
} PortIndex;

typedef struct {	
	LV2_URID midi_MidiEvent;
	LV2_URID atom_Blank;
	LV2_URID atom_Object;
	LV2_URID atom_Vector;
	LV2_URID atom_Float;
	LV2_URID atom_Int;
	LV2_URID atom_eventTransfer;
	
	LV2_URID rawaudio;
	LV2_URID audiodata;
	
	LV2_URID samplerate;
	LV2_URID ui_on;
	LV2_URID ui_off;
} LPCLV2URIs;

static inline void
map_lpc_uris(LV2_URID_Map* map, LPCLV2URIs* uris) {
	uris->midi_MidiEvent     = map->map(map->handle, LV2_MIDI__MidiEvent);
	uris->atom_Blank         = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Object        = map->map(map->handle, LV2_ATOM__Object);
	uris->atom_Vector        = map->map(map->handle, LV2_ATOM__Vector);
	uris->atom_Float         = map->map(map->handle, LV2_ATOM__Float);
	uris->atom_Int           = map->map(map->handle, LV2_ATOM__Int);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->rawaudio           = map->map(map->handle, LPC_URI "#rawaudio");
	uris->audiodata          = map->map(map->handle, LPC_URI "#audiodata");
	uris->samplerate         = map->map(map->handle, LPC_URI "#samplerate");
	uris->ui_on              = map->map(map->handle, LPC_URI "#ui_on");
	uris->ui_off             = map->map(map->handle, LPC_URI "#ui_off");
}
