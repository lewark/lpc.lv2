#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>

#include <cairo/cairo.h>

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lpc_plugin.h"

#define RTK_URI LPC_URI
#define RTK_GUI "#ui"

#define N_DIALS 5
#define N_CBTNS 4

typedef struct {
	const int id;
	const float default_val;
	const float min_val;
	const float max_val;
	const float step;
	const char* name;
	const char* unit;
} DialProp;

typedef struct {
	const int id;
	const bool default_val;
	const char* name;
} CBtnProp;

const DialProp DIAL_PROPS[N_DIALS] = {
	{4,18,1,128,1,"Order",""},
	{5,2048,512,4096,1,"Buffer Size","samples"},
	{10,0,-24,24,1,"Pitch Shift","semi"},
	{11,440,400,480,1,"Tuning","Hz"},
	{12,2,1,7,1,"Bend Range","semi"}
};

const CBtnProp CBTN_PROPS[N_CBTNS] = {
	{6,0,"Whisper"},
	{7,0,"OLA"},
	{8,0,"Glottal Pulse"},
	{9,0,"Preemphasis"}
};

const char* DIAL_VAL_STR[N_DIALS] = {"10","2048 samples","0 semi","440 Hz","2 semi"};

typedef struct {
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	RobWidget* rw;
	RobWidget* dial_panel;
	RobWidget* cbtn_panel;
	
	RobTkXYp* graph;
	
	RobTkDial* dials[N_DIALS];
	RobTkLbl* dlabels[N_DIALS];
	RobTkLbl* dlabels2[N_DIALS];
	RobTkCBtn* cbtns[N_CBTNS];
	
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
	
	self->rw = rob_hbox_new(FALSE,0);
	robwidget_make_toplevel(self->rw,ui_toplevel);
	robwidget_toplevel_enable_scaling(self->rw);
	//robtk_queue_scale_change(self->rw,2.0);
	//robtk_expose_y
	*widget = self->rw;
	
	self->graph_xpts = (float*)calloc(10,sizeof(float));
	self->graph_ypts = (float*)calloc(10,sizeof(float));
	
	for (int i = 0; i < 10; i++) {
		self->graph_xpts[i] = (float)i;
		self->graph_ypts[i] = pow((float)(i-5),3.0f);
	}
	
	self->graph = robtk_xydraw_new(100,100);
	robtk_xydraw_set_points(self->graph, 10, self->graph_xpts, self->graph_ypts);
	rob_hbox_child_pack(self->rw,GXY_W(self->graph),FALSE,TRUE);
	robwidget_set_alignment(GXY_W(self->graph),0,0.5);\
	robtk_xydraw_set_area(self->graph,0,-5,10,5);
	//robwidget_set_size_request(GXY_W(self->graph), graph_size_request);
	
	self->cbtn_panel = rob_vbox_new(FALSE,0);
	rob_hbox_child_pack(self->rw,self->cbtn_panel,FALSE,TRUE);
	robwidget_set_alignment(self->cbtn_panel,0,0);
	
	self->dial_panel = rob_table_new(4,6,FALSE);
	rob_hbox_child_pack(self->rw,self->dial_panel,FALSE,TRUE);
	robwidget_set_alignment(self->dial_panel,0,0);

	for (int i = 0; i < N_DIALS; i++) {
		DialProp prop = DIAL_PROPS[i];
		
		self->dials[i] = robtk_dial_new(prop.min_val, prop.max_val, prop.step);
		robtk_dial_set_value(self->dials[i], prop.default_val);
		
		self->dlabels[i] = robtk_lbl_new(prop.name); 
		self->dlabels2[i] = robtk_lbl_new(DIAL_VAL_STR[i]); 
		
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
	
	for (int i = 0; i < N_CBTNS; i++) {
		CBtnProp prop = CBTN_PROPS[i];
		
		self->cbtns[i] = robtk_cbtn_new(prop.name, GBT_LED_LEFT, TRUE);
		robtk_cbtn_set_active(self->cbtns[i], prop.default_val);
		
		rob_vbox_child_pack(self->cbtn_panel,GCB_W(self->cbtns[i]),FALSE,TRUE);
	}
	
	return self;
}

static void
cleanup(LV2UI_Handle handle)
{
	LPC_UI* self = (LPC_UI*)handle;
	for (int i = 0; i < N_DIALS; i++) {
		robtk_dial_destroy(self->dials[i]);
		robtk_lbl_destroy(self->dlabels[i]);
		robtk_lbl_destroy(self->dlabels2[i]);
	}
	for (int i = 0; i < N_CBTNS; i++) {
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
}


static const void*
extension_data (const char* uri)
{
	return NULL;
}
