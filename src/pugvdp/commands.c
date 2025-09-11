///////////////////////////////////////////////////////////////////////////////
// Command handler framework
///////////////////////////////////////////////////////////////////////////////
#include "commands.h"
//-----------------------------------------------------------------------------

int16_t       selected_cmd = -1;
t_vdphandler  selected_cmd_handler = NULL;
uint8_t       selected_cmd_initial_args_sz = 0;
uint8_t       selected_cmd_later_args_sz = 0;
uint          cmd_arg_byte_idx = 0;
bool          selected_cmd_first_data = true;

uint8_t       args_buf[HOST_FIFO_SZ];
//-----------------------------------------------------------------------------
// pointers to handler functions for each command, in numeric order by command number
// 
t_vdphandler command_functions[256] = { 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
};
//-----------------------------------------------------------------------------
// min bytecount of initial function args for each command before data is 
// passed to handler function. (If zero, the handler is called as soon
// as the cmd register is selected.)
//
uint8_t command_initial_args_sz[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
//-----------------------------------------------------------------------------
// modules call this function to register their 
// handler functions for various commands.
//
void command_init(uint8_t cmd_index, uint8_t initial_args_size, t_vdphandler cmd_fcn )
{
  command_functions[cmd_index] = cmd_fcn;
  command_initial_args_sz[cmd_index] = initial_args_size;
}
//-----------------------------------------------------------------------------
// called when host specifies a cmd
//
void command_select(uint8_t cmd_index)
{
  // if command handler is defined, select command 
  if(command_functions[cmd_index])
  {
    selected_cmd = cmd_index;
    selected_cmd_handler = command_functions[cmd_index];      
    selected_cmd_first_data = true; // so handler knows its the first call
    selected_cmd_initial_args_sz = command_initial_args_sz[cmd_index];
    // if command takes no args, call handler immediately.
    if(!selected_cmd_initial_args_sz)
    {
      selected_cmd_handler(NULL, 0, true);
    }
  } else // undefined command!
  {
    // deselect any previous command, clear args buffer
    selected_cmd = -1;
    selected_cmd_handler = NULL;
    selected_cmd_initial_args_sz = 0;
  }
  cmd_arg_byte_idx = 0;  // clear any old buffered args/data
}
//-----------------------------------------------------------------------------
// Called whenever bytes received from host. Selects command as appropriate,
// and passes data to selected command handler as soon as enough data has 
// accumulated to satisfy the minimum args req. for that command.
//
// (If the command is changed before that happens, the bytes are discarded.)
//
void data_receive(uint16_t *dat, uint sz)
{
  // peek/seek in fifo to see what we got.
  uint8_t i=0;
  uint16_t w;
  while(sz)
  {
    w = dat[i++];
    sz--;
    uint8_t a0 = w&1;
    uint8_t d = (w>>1)&0xff;
    if(a0)  // if a0 == 1: this is arguments or data
    {
      if(selected_cmd_handler)            // if a command is selected
      {
        args_buf[cmd_arg_byte_idx++] = d; // buffer the data byte,
      }
      else
      {
        cmd_arg_byte_idx = 0;             // else trash the data byte!
      }
    } else // if a0 == 0: this is a command selection.
    {
      // if we'd buffered enough args for a first run of a selected
      // command, or if we have more bytes for another subsequent call,
      // pass them to the previous command's handler before proceeding.
      if(
          (selected_cmd_initial_args_sz && selected_cmd_first_data && (cmd_arg_byte_idx >= selected_cmd_initial_args_sz)) ||
          (cmd_arg_byte_idx && (!selected_cmd_first_data))
        )
      {
        selected_cmd_handler(args_buf, cmd_arg_byte_idx, selected_cmd_first_data);        
      }
      // select command, clear any old args
      command_select(d);
    }   
  }

  // done with fifo. If we got enough args for a first run of command handler, 
  // or if this is a subsequent run, and we have any data at all, pass them to handler.
  if(
      (selected_cmd_initial_args_sz && selected_cmd_first_data && (cmd_arg_byte_idx >= selected_cmd_initial_args_sz)) ||
      (cmd_arg_byte_idx && (!selected_cmd_first_data))
    )
  {
    selected_cmd_handler(args_buf, cmd_arg_byte_idx, selected_cmd_first_data);
    selected_cmd_first_data = false;    
    cmd_arg_byte_idx = 0;
  }
}
///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////
