///////////////////////////////////////////////////////////////////////////////
// 
// PugVDP v0.1 - Retrocomputer video display processor based on a Pi Pico 2!!
//
// Adapted from Luke Wren's PicoDVI project on GitHub. (He did all of the
// heavy lifting regarding video generation! Thanks, Luke!!)
//
// Started July 2025 by Craig Iannello
//
// Starting with the colour_terminal demo, I first changed the font to an 
// 8x16 IBM VGA font, bolted-on a PIO-based 8-bit bus interface, and added 
// some command handlers and video modes. The intent is to use an RP2350, 
// hooked directly to the bus of my Pugputer 6309, to act as the display chip.
//
// Hopefully, other people will find this useful for their retrocomputers!
//
///////////////////////////////////////////////////////////////////////////////
#include "defines.h"
#include "mode_text.h"
#include "mode_bitmap.h"
#include "commands.h"
#include "bus_interface.pio.h"


// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------
struct dvi_inst   dvi0;
t_vidmode         vdp_video_mode = MODE_TEXT;  // defines.h
PIO               pio = DEFAULT_PIO;
uint              pio_offset = 0;

// Dumb-simple fifo for enqueing bytes received from host in
// pio_irq_handler.  (I was having synchronization issus with more 
// sophisticated solutions, so this is it for now. :D )
uint16_t dumb_fifo[HOST_FIFO_SZ];
uint dumb_head=0,dumb_tail=0;
uint last_dumb_head = 0;

// mainloop dequeues chunks to here for passing to command/data handler.
uint16_t dumb_buf[HOST_FIFO_SZ];

// ----------------------------------------------------------------------------
// CORE1: Display refresh loop
// ----------------------------------------------------------------------------
void core1_main() 
{
  dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
  dvi_start(&dvi0);
  while (true) 
  {
    switch(vdp_video_mode)
    {
      case MODE_TEXT:
        text_rasterize_screen();
        break;
      default:
        break;
    }
  }
}
// ----------------------------------------------------------------------------
// CORE0 PIO IRQ handler - called when host writes a commond or data byte
// to the PugVDP. We queue these for processing by the CORE0 mainloop, below.
// The FIFO needs to be thread-safe, since bytes may be added without warning
// in this interrupt hander, even as bytes are being read out by app.
void pio_irq_handler()
{
  // grab words from pio rx fifo
  while (!pio_sm_is_rx_fifo_empty(pio, SM)) 
  {
    dumb_fifo[dumb_head++] = pio_sm_get(pio, SM) >> 23;
    if(dumb_head>=HOST_FIFO_SZ)
      dumb_head=0;
  }      
  // Toggle LED
  gpio_xor_mask(1u << LED_PIN);
  // Clear PIO0 IRQ 0 
  pio_interrupt_clear(pio, 0);  
}
// ----------------------------------------------------------------------------
// Set up the PIO bus interface
//
void bus_interface_program_init(PIO pio, uint sm, uint pio_offset, uint pin_base) 
{
  pio_sm_config c = bus_interface_program_get_default_config(pio_offset);
  // Set base pin for IN and WAIT instructions
  sm_config_set_in_pins(&c, pin_base);
  // bus interface as inputs
  pio_sm_set_consecutive_pindirs(pio, sm, pin_base, NUM_PINS, false);
  // Shift right, autopush after 10 bits
  //sm_config_set_in_shift(&c, true, true, 10);
  // Use only RX FIFO
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
  sm_config_set_clkdiv(&c, 1.0f);
  // Load and start
  pio_sm_init(pio, sm, pio_offset, &c);
  pio_sm_set_enabled(pio, sm, true);
}
// ----------------------------------------------------------------------------
// CORE0: System init and command loop
// ----------------------------------------------------------------------------
int __not_in_flash("main") main() 
{
  vreg_set_voltage(VREG_VSEL);
  sleep_ms(10);
  // Run system at TMDS bit clock
  set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);
  // Set the LED pin as an output, bus pins as inputs.
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  for (int i = GPIO_BASE; i < GPIO_BASE + NUM_PINS; ++i) 
  {
    gpio_init(i);
    gpio_set_dir(i, GPIO_IN);
    gpio_disable_pulls(i);
  }
  // Start up a PIO bus interface which generates an IRQ here
  // on CORE0 whenever the host does a write. (IRQ handler will
  // queue these up for processing in the CORE0 mainloop, below.) 
  pio_set_irq0_source_enabled(pio, DEFAULT_IRQ_SOURCE, true);
  irq_set_exclusive_handler(DEFAULT_PIO_IRQ_0, pio_irq_handler);
  irq_set_enabled(DEFAULT_PIO_IRQ_0, true);
  pio_offset = pio_add_program(pio, &bus_interface_program);
  bus_interface_program_init(pio, SM, pio_offset, GPIO_BASE);
  // Init DVI video generation
  // todo: Ensure we're taking advantage of RP2350's hardware HSTX and
  // TMDS encoding. (PicoDVI currently does do the hardware TDMS, but not
  // the HSTX.)
  dvi0.timing = &DVI_TIMING;
  dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
  dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

  // init text display mode
  text_init();

  // Start up the DVI display generator
  hw_set_bits(&bus_ctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC1_BITS);
  multicore_launch_core1(core1_main);

  while (1)  // Begin PugVDP mainloop / command processor
  {
    last_dumb_head = dumb_head;     // grab a stable copy of buf head.
    if(dumb_tail != last_dumb_head) // If got data from host,
    {
      uint sz = 0;
      while(dumb_tail!=last_dumb_head)
      {
        dumb_buf[sz++]=dumb_fifo[dumb_tail]; // dequeue it,
        if(++dumb_tail>=HOST_FIFO_SZ)
          dumb_tail=0;
      }
      if(sz)
      {
        data_receive(dumb_buf, sz);  // and pass to handler in commands.c .        
      }
    } else
    {
      __wfi();  // else, sleep until next interrupt.
    }
  }

}
  
///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////
