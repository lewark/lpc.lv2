#ifndef lpc_plugin_h
#define lpc_plugin_h


#ifndef PEG_STRUCT
#define PEG_STRUCT
typedef struct {
  float min;
  float max;
  float default_value;
  char toggled;
  char integer;
  char logarithmic;
} peg_data_t;
#endif

#endif /* lpc_plugin_h */
