#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>

#include <cairo/cairo.h>

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lpc_plugin.h"

#define RTK_URI LPC_URI
#define RTK_GUI "#ui"

#define DIALS_N 5
#define CBTNS_N 4

#define DIALS_FIRST LPC_ORDER
#define CBTNS_FIRST LPC_WHISPER
#define PORTS_LAST LPC_PREEMPHASIS

#define LBL_MAXLEN 24

typedef struct {
	const float default_val;
	const float min_val;
	const float max_val;
	const float step;
	const char* name;
	const char* unit;
} DialProp;

typedef struct {
	const bool default_val;
	const char* name;
} CBtnProp;

const DialProp DIAL_PROPS[DIALS_N] = {
	{18,1,128,1,"Order",""},
	{2048,512,4096,1,"Buffer Size"," sample"},
	{0,-24,24,0.01,"Pitch Shift"," semi"},
	{440,400,480,0.01,"Tuning"," Hz"},
	{2,1,7,0.01,"Bend Range"," semi"}
};

const CBtnProp CBTN_PROPS[CBTNS_N] = {
	{0,"Whisper"},
	{0,"OLA"},
	{0,"Glottal Pulse"},
	{0,"Preemphasis"}
};

const char* DIAL_VAL_STR[DIALS_N] = {"10","2048 samples","0 semi","440 Hz","2 semi"};

typedef struct {
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	RobWidget* rw;
	RobWidget* dial_panel;
	RobWidget* cbtn_panel;
	
	RobTkXYp* graph;
	
	RobTkDial* dials[DIALS_N];
	RobTkLbl* dlabels[DIALS_N];
	RobTkLbl* dlabels2[DIALS_N];
	RobTkCBtn* cbtns[CBTNS_N];
	
	float* graph_xpts;
	float* graph_ypts;
} LPC_UI;

#define GSP_W(PTR) robtk_dial_widget (PTR)
#define GLB_W(PTR) robtk_lbl_widget (PTR)
#define GSL_W(PTR) robtk_select_widget (PTR)
#define GCB_W(PTR) robtk_cbtn_widget (PTR)
#define GPB_W(PTR) robtk_pbtn_widget (PTR)
#define GDA_W(PTR) robtk_darea_widget (PTR)
#define GXY_W(PTR) robtk_xydraw_widget (PTR)

static void darea_expose(cairo_t* cr, void *handle)
{
	cairo_set_source_rgb(cr,1,0,0);
	cairo_rectangle(cr,0,0,100,100);
	cairo_fill(cr);
}

static void update_dial_label(LPC_UI* self, int i)
{
	RobTkDial* dial = self->dials[i];
	RobTkLbl* label = self->dlabels2[i];
	float value = robtk_dial_get_value(dial);
	char text[LBL_MAXLEN];
	if (DIAL_PROPS[i].step == 1.0f) {
		snprintf(text, LBL_MAXLEN, "%i%s", (int)value, DIAL_PROPS[i].unit);
	} else {
		snprintf(text, LBL_MAXLEN, "%.2f%s", value, DIAL_PROPS[i].unit);
	}
	robtk_lbl_set_text(label, text);
}

static bool dial_callback(RobWidget* rw, void* handle)
{
	LPC_UI* self = (LPC_UI*)handle;
	RobTkDial* dial = (RobTkDial*)rw->self;
	float value = robtk_dial_get_value(dial);
	for (uint32_t i = 0; i < DIALS_N; i++) {
		if (self->dials[i] == dial) {
			update_dial_label(self,i);
			self->write_function(
				self->controller, DIALS_FIRST+i,
				sizeof(float), 0, (const void*)&value
			);
			break;
		}
	}
	return true;
}

static bool cbtn_callback(RobWidget* rw, void* handle)
{
	LPC_UI* self = (LPC_UI*)handle;
	RobTkCBtn* cbtn = (RobTkCBtn*)rw->self;
	float value = (float)robtk_cbtn_get_active(cbtn);
	
	for (uint32_t i = 0; i < CBTNS_N; i++) {
		if (self->cbtns[i] == cbtn) {
			self->write_function(
				self->controller, CBTNS_FIRST+i,
				sizeof(float), 0, (const void*)&value
			);
			break;
		}
	}
	return true;
}

static LV2UI_Handle
instantiate(
		void *const               ui_toplevel,
		const LV2UI_Descriptor*   descriptor,
		const char*               plugin_uri,
		const char*               bundle_path,
		LV2UI_Write_Function      write_function,
		LV2UI_Controller          controller,
		RobWidget**               widget,
		const LV2_Feature* const* features)
{
	LPC_UI* self = (LPC_UI*)calloc(1,sizeof(LPC_UI));
	self->write_function = write_function;
	self->controller = controller;
	
	self->rw = rob_hbox_new(false,0);
	robwidget_make_toplevel(self->rw,ui_toplevel);
	robwidget_toplevel_enable_scaling(self->rw);
	//robtk_queue_scale_change(self->rw,2.0);
	//robtk_expose_y
	*widget = self->rw;
	
	self->graph_xpts = (float*)calloc(10,sizeof(float));
	self->graph_ypts = (float*)calloc(10,sizeof(float));
	
	for (int i = 0; i < 10; i++) {
		self->graph_xpts[i] = ((float)i)/9.0f;
		self->graph_ypts[i] = 0;//pow((float)(i-5),2.0f);
	}
	
	//self->graph=robtk_pbtn_new("Hello")
	self->graph = robtk_xydraw_new(256,256);
	//robtk_xydraw_set_area(self->graph,0,0,100,100);
	robtk_xydraw_set_mapping(self->graph,1,0,0.5,0.5);
	robtk_xydraw_set_points(self->graph, 10, self->graph_xpts, self->graph_ypts);
	rob_hbox_child_pack(self->rw,GXY_W(self->graph),true,true);
	//robwidget_set_alignment(GXY_W(self->graph),0,0.5);
	//robwidget_set_size_request(GXY_W(self->graph), graph_size_request);
	
	self->cbtn_panel = rob_vbox_new(false,0);
	rob_hbox_child_pack(self->rw,self->cbtn_panel,false,true);
	robwidget_set_alignment(self->cbtn_panel,0,0);
	
	self->dial_panel = rob_table_new(4,6,false);
	rob_hbox_child_pack(self->rw,self->dial_panel,false,true);
	robwidget_set_alignment(self->dial_panel,0,0);

	for (int i = 0; i < DIALS_N; i++) {
		DialProp prop = DIAL_PROPS[i];
		// 1: line from center if 0, dot if 1
		//    2: small shade in dot if 1
		// 4: arc around knob
		//    8: arc to default if 1
		//16: fill bg
		self->dials[i] = robtk_dial_new(prop.min_val, prop.max_val, prop.step);
		
		// show arc
		self->dials[i]->displaymode = 1|2|4;
		if (i == LPC_PITCH-DIALS_FIRST) {
			// begin arc at dial default (zero)
			self->dials[i]->displaymode |= 8;
			float detents[3] = {-12,0,12};
			robtk_dial_set_detents(self->dials[i],3,detents);
		} else if (DIAL_PROPS[i].step != 1.0) {
			robtk_dial_set_detent_default(self->dials[i], true);
		}
		
		robtk_dial_set_default(self->dials[i], prop.default_val);
		robtk_dial_set_callback(self->dials[i], dial_callback, self);
		//robtk_dial_set_constained(self->dials[i], false);
		
		self->dlabels[i] = robtk_lbl_new(prop.name);
		self->dlabels2[i] = robtk_lbl_new(DIAL_VAL_STR[i]);
		robtk_lbl_set_fontdesc(self->dlabels2[i],"Sans 10px");
		
		unsigned int x = i*2;
		unsigned int y = 0;
		if (i >= 3) {
			y = 3;
			x -= 5;
		}
		rob_table_attach(self->dial_panel,GLB_W(self->dlabels[i]),
			x,x+2,y,y+1,0,0,RTK_SHRINK,RTK_SHRINK);
		rob_table_attach(self->dial_panel,GSP_W(self->dials[i]),
			x,x+2,y+2,y+3,0,0,RTK_SHRINK,RTK_SHRINK);
		rob_table_attach(self->dial_panel,GLB_W(self->dlabels2[i]),
			x,x+2,y+1,y+2,0,0,RTK_SHRINK,RTK_SHRINK);
	}
	
	for (int i = 0; i < CBTNS_N; i++) {
		CBtnProp prop = CBTN_PROPS[i];
		
		self->cbtns[i] = robtk_cbtn_new(prop.name, GBT_LED_LEFT, true);
		robtk_cbtn_set_active(self->cbtns[i], prop.default_val);
		robtk_cbtn_set_callback(self->cbtns[i], cbtn_callback, self);
		
		rob_vbox_child_pack(self->cbtn_panel,GCB_W(self->cbtns[i]),false,true);
	}
	
	return self;
}

static void
cleanup(LV2UI_Handle handle)
{
	LPC_UI* self = (LPC_UI*)handle;
	for (int i = 0; i < DIALS_N; i++) {
		robtk_dial_destroy(self->dials[i]);
		robtk_lbl_destroy(self->dlabels[i]);
		robtk_lbl_destroy(self->dlabels2[i]);
	}
	for (int i = 0; i < CBTNS_N; i++) {
		robtk_cbtn_destroy(self->cbtns[i]);
	}
	
	robtk_xydraw_destroy(self->graph);
	rob_table_destroy(self->dial_panel);
	rob_box_destroy(self->cbtn_panel);
	rob_box_destroy(self->rw);
	free(self->graph_xpts);
	free(self->graph_ypts);
	free(self);
}

#define LVGL_RESIZEABLE

static void ui_enable (LV2UI_Handle handle) { }
static void ui_disable (LV2UI_Handle handle) { }

static enum LVGLResize
plugin_scale_mode(LV2UI_Handle handle)
{
	return LVGL_LAYOUT_TO_FIT;
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	LPC_UI* self = (LPC_UI*)handle;
	if (format != 0) {
		return;
	}
	const float value = *((const float*)buffer);
	if (port_index >= DIALS_FIRST && port_index < CBTNS_FIRST) {
		int dial_index = port_index-DIALS_FIRST;
		robtk_dial_set_value(self->dials[dial_index], value);
		update_dial_label(self, dial_index);
	} else if (port_index >= CBTNS_FIRST && port_index <= PORTS_LAST) {
		robtk_cbtn_set_active(self->cbtns[port_index-CBTNS_FIRST], value > 0);
	}
}


static const void*
extension_data (const char* uri)
{
	return NULL;
}
