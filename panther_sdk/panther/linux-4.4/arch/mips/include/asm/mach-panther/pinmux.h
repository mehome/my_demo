#ifndef __PANTHER_PINMUX_H__
#define __PANTHER_PINMUX_H__

int uart_pinmux_selected(int uart_port_id);
int i2c_pinmux_selected(void);
int gspi_pinmux_selected(void);
int i2sclk_pinmux_selected(void);
int i2sdat_pinmux_selected(void);
#if 0 // suspend/resume try to cancel popup noise
int i2s_mclk_suspend(void);
int i2s_mclk_resume(void);
#endif
int spdif_pinmux_selected(void);
int tsi_pinmux_selected(void);

int pinmux_pin_func_gpio(int pin_number);
int pinmux_pin_func_i2c(int pin_number);

#if !defined(CONFIG_PANTHER_FPGA)
void ext_phy_pinmux(void);
#endif

#endif // __PANTHER_PINMUX_H__

