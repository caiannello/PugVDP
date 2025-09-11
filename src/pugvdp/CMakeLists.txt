add_executable(PugVDP
	commands.c
	helpers.c	
	main.c
	mode_text.c
	mode_bitmap.c
	tmds_encode_font_2bpp.S
	tmds_encode_font_2bpp.h
	bus_interface.pio	
)

target_compile_definitions(PugVDP PRIVATE
	DVI_VERTICAL_REPEAT=1
	)

# We have a lot in SRAM4 (particularly TMDS LUT) but don't need much stack on
# core 1. Probably even 256 bytes would be fine.
target_compile_definitions(PugVDP PRIVATE
	DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG}
	PICO_CORE1_STACK_SIZE=0x200
	)

pico_generate_pio_header(PugVDP ${CMAKE_CURRENT_LIST_DIR}/bus_interface.pio)

target_link_libraries(PugVDP
	pico_stdlib
	pico_multicore
	libdvi
    hardware_pio	
)

# create map/bin/hex file etc.
pico_add_extra_outputs(PugVDP)
