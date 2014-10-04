/*
 * Code for Variscite AM335X SOM.
 *
 * Copyright (C) 2012 Variscite, Ltd. - http://www.variscite.com/
 *
 * Copyright (C) 2011 Texas Instruments, Inc. - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/micrel_phy.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/platform_data/ti_adc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/wl12xx.h>
#include <linux/ethtool.h>
#include <linux/mfd/tps65910.h>
#include <linux/pwm_backlight.h>
#include <linux/reboot.h>
#include <linux/pwm/pwm.h>
#include <linux/ti_wilink_st.h>
#include <linux/input/ti_tsc.h>
#include <linux/mfd/ti_tscadc.h>

#include <video/da8xx-fb.h>
#include <mach/hardware.h>
#include <mach/board-am335xevm.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/asp.h>

#include <plat/irqs.h>
#include <plat/board.h>
#include <plat/common.h>
#include <plat/lcdc.h>
#include <plat/usb.h>
#include <plat/mmc.h>
#include <plat/emif.h>
#include <plat/nand.h>

#include "common.h"
#include "board-flash.h"
#include "cpuidle33xx.h"
#include "mux.h"
#include "devices.h"
#include "hsmmc.h"


/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio)         (32 * (bank) + (gpio))

#define KM_CONFIG_WL12XX                0
#define KM_CONFIG_BACKLIGHT_GENERIC     1

#define VAR_LCD_UTM                     0
#define VAR_LCD_CTW6120                 1

#define NO_OF_MAC_ADDR		            3

/* module pin mux structure */
struct pinmux_config {
	const char *        string_name;    /* signal name format */
	int                 val;            /* Options for the mux register value */
};

static char am335x_mac_addr[NO_OF_MAC_ADDR][ETH_ALEN];

/* LCD */
static const struct display_panel disp_panel = {
	WVGA,
	32,
	32,
	COLOR_ACTIVE,
};

static struct lcd_ctrl_config lcd_cfg = {
	&disp_panel,
	.ac_bias		    = 255,
	.ac_bias_intrpt		= 0,
	.dma_burst_sz		= 16,
	.bpp			    = 32,
	.fdd			    = 0x80,
	.tft_alt_mode		= 0,
	.stn_565_mode		= 0,
	.mono_8bit_mode		= 0,
	.invert_line_clock	= 1,
	.invert_frm_clock	= 1,
	.sync_edge		    = 0,
	.sync_ctrl		    = 1,
	.raster_order		= 0,
};

struct da8xx_lcdc_platform_data VAR_LCD_pdata = {
	.manu_name		= "Variscite",
	.controller_data	= &lcd_cfg,
	.type			= "VAR-WVGA",
};

/* Audio */
static u8 am335x_iis_serializer_direction1[] = {
	INACTIVE_MODE,	INACTIVE_MODE,	RX_MODE,	TX_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
};

static struct snd_platform_data var_am335x_som_snd_data1 = {
	.tx_dma_offset	= 0x46000000,	/* McASP1 */
	.rx_dma_offset	= 0x46000000,
	.op_mode	= DAVINCI_MCASP_IIS_MODE,
	.num_serializer	= ARRAY_SIZE(am335x_iis_serializer_direction1),
	.tdm_slots	= 2,
	.serial_dir	= am335x_iis_serializer_direction1,
	.asp_chan_q	= EVENTQ_2,
	.version	= MCASP_VERSION_3,
	.txnumevt	= 32,
	.rxnumevt	= 32,
};

/* MMC */
static struct omap2_hsmmc_info am335x_mmc[] __initdata = {
	{
		.mmc            = 1,
		.caps           = MMC_CAP_4_BIT_DATA,
		.gpio_cd        = GPIO_TO_PIN(1, 28),
		.gpio_wp        = -EINVAL,
		.ocr_mask       = MMC_VDD_32_33 | MMC_VDD_33_34, /* 3V3 */
	},
	{
		.mmc            = 0,	/* will be set at runtime */
	},
	{
		.mmc            = 0,	/* will be set at runtime */
	},
	{}      /* Terminator */
};


#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	AM33XX_MUX(I2C0_SDA, OMAP_MUX_MODE7 | AM33XX_INPUT_EN),
	AM33XX_MUX(I2C0_SCL, OMAP_MUX_MODE7 |AM33XX_INPUT_EN ),
	AM33XX_MUX(SPI0_SCLK, OMAP_MUX_MODE7 | AM33XX_SLEWCTRL_SLOW |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(EMU1, OMAP_MUX_MODE7 |
				AM33XX_INPUT_EN | AM33XX_PULL_UP),

	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define	board_mux	NULL
#endif

/*-- SOM revision pins -------------------------------------------------------*/
static struct pinmux_config am33_var_som_rev_pin_mux[] = {
	{"lcd_data7.gpio2_13",	        OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLDOWN},
	{"lcd_vsync.gpio2_22",	        OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLDOWN},

	{NULL, 0},
};

/*-- UART0 -------------------------------------------------------------------*/
static struct pinmux_config uart0_pin_mux[] = {
	{"uart0_rxd.uart0_rxd",         OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"uart0_txd.uart0_txd",         OMAP_MUX_MODE0 | AM33XX_PULL_ENBL},
	{NULL, 0},
};

/*-- UART3 -------------------------------------------------------------------*/
static struct pinmux_config uart3_pin_mux[] = {
    {"spi0_cs1.uart3_rxd",          OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"ecap0_in_pwm0_out.uart3_txd", OMAP_MUX_MODE0 | AM33XX_PULL_ENBL},
	{NULL, 0},
};

/*-- UART5 -------------------------------------------------------------------*/
static struct pinmux_config uart5_pin_mux[] = {
    {"mdio_data.uart5_rxd",         OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP},
    {"mdio_clk.uart5_txd",          OMAP_MUX_MODE2 | AM33XX_PULL_ENBL},
    {NULL, 0}
};

/*-- Backlight ---------------------------------------------------------------*/
#define BACKLIGHT_CTRL_PIN              GPIO_TO_PIN(1, 19)

static struct pinmux_config backlight_pin_mux[] = {
    {"gpmc_a5.gpio1_21",            OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/*-- LCDC --------------------------------------------------------------------*/
#define LCDC_DISP_STBY                  GPIO_TO_PIN(1, 21)

static struct pinmux_config lcdc_pin_mux[] = {
	{"lcd_data0.lcd_data0",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data1.lcd_data1",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data2.lcd_data2",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data3.lcd_data3",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data4.lcd_data4",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data5.lcd_data5",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data6.lcd_data6",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data7.lcd_data7",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data8.lcd_data8",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data9.lcd_data9",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data10.lcd_data10",	    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data11.lcd_data11",	    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data12.lcd_data12",	    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data13.lcd_data13",	    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data14.lcd_data14",	    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_data15.lcd_data15",	    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
						                | AM33XX_PULL_DISA},
	{"lcd_vsync.lcd_vsync",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"lcd_hsync.lcd_hsync",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"lcd_pclk.lcd_pclk",		    OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"lcd_ac_bias_en.lcd_ac_bias_en", OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"gpmc_a3.gpio1_19",            OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/*-- NAND Flash --------------------------------------------------------------*/
static struct pinmux_config nand_pin_mux[] = {
	{"gpmc_ad0.gpmc_ad0",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad1.gpmc_ad1",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad2.gpmc_ad2",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad3.gpmc_ad3",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad4.gpmc_ad4",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad5.gpmc_ad5",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad6.gpmc_ad6",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad7.gpmc_ad7",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_wait0.gpmc_wait0",       OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_wpn.gpmc_wpn",	        OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_csn0.gpmc_csn0",	        OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{"gpmc_advn_ale.gpmc_advn_ale", OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{"gpmc_oen_ren.gpmc_oen_ren",   OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{"gpmc_wen.gpmc_wen",           OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{"gpmc_ben0_cle.gpmc_ben0_cle", OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{NULL, 0},
};

/*-- RMII1 -------------------------------------------------------------------*/
#define VAR_SOM_GMII1_RST_GPIO   GPIO_TO_PIN(2, 19)

static struct pinmux_config rmii1_pin_mux[] = {
	{"mii1_crs.rmii1_crs_dv",       OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_rxerr.mii1_rxerr",       OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_txen.mii1_txen",         OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"mii1_txd1.mii1_txd1",         OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"mii1_txd0.mii1_txd0",         OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"mii1_rxd1.mii1_rxd1",         OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_rxd0.mii1_rxd0",         OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN},
	{"rmii1_refclk.rmii1_refclk",   OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mdio_data.mdio_data",         OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mdio_clk.mdio_clk",           OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT_PULLUP},
	{NULL, 0},
};

/*-- I2C1 --------------------------------------------------------------------*/
static struct pinmux_config i2c1_pin_mux[] = {
    {"spi0_d1.i2c1_sda",            OMAP_MUX_MODE2 | AM33XX_SLEWCTRL_SLOW |
					                    AM33XX_PULL_ENBL | AM33XX_INPUT_EN},
	{"spi0_cs0.i2c1_scl",           OMAP_MUX_MODE2 | AM33XX_SLEWCTRL_SLOW |
					                    AM33XX_PULL_ENBL | AM33XX_INPUT_EN},
	{NULL, 0},
};

/*-- I2C2 --------------------------------------------------------------------*/
static struct pinmux_config i2c2_pin_mux[] = {
	{"uart1_ctsn.i2c2_sda",         OMAP_MUX_MODE2 | AM33XX_SLEWCTRL_SLOW |
					                    AM33XX_PULL_ENBL | AM33XX_INPUT_EN},
	{"uart1_rtsn.i2c2_scl",         OMAP_MUX_MODE2 | AM33XX_SLEWCTRL_SLOW |
					                    AM33XX_PULL_ENBL | AM33XX_INPUT_EN},
	{NULL, 0},
};

/*-- MCASP1 ------------------------------------------------------------------*/
static struct pinmux_config mcasp1_pin_mux[] = {
	{"mcasp0_aclkx.mcasp0_aclkx",   OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_fsx.mcasp0_fsx",       OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_aclkr.mcasp0_axr2",    OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mcasp0_fsr.mcasp0_axr3",      OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLDOWN},
	{NULL, 0},
};

/*-- MMC0 --------------------------------------------------------------------*/
static struct pinmux_config mmc0_pin_mux[] = {
	{"mmc0_dat3.mmc0_dat3",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat2.mmc0_dat2",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat1.mmc0_dat1",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat0.mmc0_dat0",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_clk.mmc0_clk",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_cmd.mmc0_cmd",	        OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	/*{"mcasp0_aclkr.mmc0_sdwp", OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},*/
	{"gpmc_ben1.gpio1_28",          OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

/*-- WLAN and BT -------------------------------------------------------------*/
static struct pinmux_config mmc1_wl12xx_pin_mux[] = {
	{"gpmc_ad8.mmc1_dat0",          OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad9.mmc1_dat1",          OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP},
    {"gpmc_ad10.mmc1_dat2",         OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad11.mmc1_dat3",         OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_csn2.mmc1_cmd",          OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_csn1.mmc1_clk",          OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

static struct pinmux_config uart1_wl12xx_pin_mux[] = {
	{"uart1_ctsn.uart1_ctsn",       OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT_PULLUP},
	{"uart1_rtsn.uart1_rtsn",       OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"uart1_rxd.uart1_rxd",         OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"uart1_txd.uart1_txd",         OMAP_MUX_MODE0 | AM33XX_PULL_ENBL},
	{NULL, 0},
};

static struct pinmux_config wl12xx_cb_rev_1_0_pin_mux_var_som[] = {
	{"mcasp0_axr1.gpio3_20",        OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
	{"mcasp0_ahclkx.gpio3_21",      OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
	{"mii1_rxdv.gpio3_4",           OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

static struct pinmux_config wl12xx_pin_mux_var_som[] = {
	{"mcasp0_axr1.gpio3_20",        OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
	{"mcasp0_ahclkx.gpio3_21",      OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
	{"mii1_txclk.gpio3_9",          OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/*-- Buzzer ------------------------------------------------------------------*/
#define BUZZER_PIN                      GPIO_TO_PIN(3, 7)

static struct pinmux_config buzzer_pin_mux[] = {
    {"emu0.gpio3_7",                OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
    {NULL, 0}
};

/*-- Digital outputs ---------------------------------------------------------*/
#define DIGOUT_1_PIN                    GPIO_TO_PIN(1, 23)
#define DIGOUT_2_PIN                    GPIO_TO_PIN(1, 25)
#define DIGOUT_3_PIN                    GPIO_TO_PIN(1, 27)
#define DIGOUT_4_PIN                    GPIO_TO_PIN(0, 30)

static struct gpio digout_gpios[] = {
    {DIGOUT_1_PIN,                  GPIOF_INIT_LOW, "digout_1"},
    {DIGOUT_2_PIN,                  GPIOF_INIT_LOW, "digout_2"},
    {DIGOUT_3_PIN,                  GPIOF_INIT_LOW, "digout_3"},
    {DIGOUT_4_PIN,                  GPIOF_INIT_LOW, "digout_4"}
};

static struct pinmux_config digout_pin_mux[] = {
    {"gpmc_a7.gpio1_23",            OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
    {"gpmc_a9.gpio1_25",            OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
    {"gpmc_a11.gpio1_27",           OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
    {"gpmc_wait0.gpio0_30",         OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT},
    {NULL, 0}
};

#define DIGOUT_1_STATE_PIN              GPIO_TO_PIN(3, 10)
#define DIGOUT_2_STATE_PIN              GPIO_TO_PIN(2, 18)
#define DIGOUT_3_STATE_PIN              GPIO_TO_PIN(1, 26)
#define DIGOUT_4_STATE_PIN              GPIO_TO_PIN(1, 24)

static struct gpio digout_state_gpios[] = {
    {DIGOUT_1_STATE_PIN,            GPIOF_IN,       "digout_state_1"},
    {DIGOUT_2_STATE_PIN,            GPIOF_IN,       "digout_state_2"},
    {DIGOUT_3_STATE_PIN,            GPIOF_IN,       "digout_state_3"},
    {DIGOUT_4_STATE_PIN,            GPIOF_IN,       "digout_state_4"}
};

static struct pinmux_config digout_state_pin_mux[] = {
    {"mii1_rxclk.gpio3_10",         OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
    {"mii1_rxd3.gpio2_18",          OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
    {"gpmc_a10.gpio1_26",           OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
    {"gpmc_a8.gpio1_24",            OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
    {NULL, 0}
};

/*-- Digital inputs ----------------------------------------------------------*/
#define DIGIN_1_PIN                     GPIO_TO_PIN(1, 22)
#define DIGIN_2_PIN                     GPIO_TO_PIN(1, 20)
#define DIGIN_3_PIN                     GPIO_TO_PIN(1, 18)
#define DIGIN_4_PIN                     GPIO_TO_PIN(1, 16)

static struct gpio digin_gpios[] = {
    {DIGIN_1_PIN,                   GPIOF_IN,       "digin_1"},
    {DIGIN_2_PIN,                   GPIOF_IN,       "digin_2"},
    {DIGIN_3_PIN,                   GPIOF_IN,       "digin_3"},
    {DIGIN_4_PIN,                   GPIOF_IN,       "digin_4"}
};

static struct pinmux_config digin_pin_mux[] = {
    {"gpmc_a6.gpio1_22",            OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
    {"gpmc_a4.gpio1_20",            OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
    {"gpmc_a2.gpio1_18",            OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
    {"gpma_a0.gpio1_16",            OMAP_MUX_MODE7 | AM33XX_PIN_INPUT},
    {NULL, 0}
};

/*
* @pin_mux - single module pin-mux structure which defines pin-mux
*			details for all its pins.
*/
static void setup_pin_mux(struct pinmux_config *pin_mux)
{
	int i;

	for (i = 0; pin_mux->string_name != NULL; pin_mux++)
		omap_mux_init_signal(pin_mux->string_name, pin_mux->val);
}

#define AM33_VAR_SOM_REV_BIT0_GPIO GPIO_TO_PIN(2, 13)
#define AM33_VAR_SOM_REV_BIT1_GPIO GPIO_TO_PIN(2, 22)

static struct gpio som_rev_gpios[] __initdata = {
	{AM33_VAR_SOM_REV_BIT0_GPIO, GPIOF_IN,	"som_rev_bit_0"},
	{AM33_VAR_SOM_REV_BIT1_GPIO, GPIOF_IN,	"som_rev_bit_1"},
};

int __init get_var_som_am33_rev(void)
{
	static int som_rev = -1;

	if (som_rev == (-1)) {
		int status;

		setup_pin_mux(am33_var_som_rev_pin_mux);

		status = gpio_request_array(som_rev_gpios, ARRAY_SIZE(som_rev_gpios));
		
		if (status) {
			pr_err("TESLAMETER_3MH: Error requesting Variscite rev gpio: %d\n", 
			    status);
		}
		som_rev = gpio_get_value(AM33_VAR_SOM_REV_BIT0_GPIO) | 
			(gpio_get_value(AM33_VAR_SOM_REV_BIT1_GPIO) << 1);
		gpio_free_array(som_rev_gpios, ARRAY_SIZE(som_rev_gpios));
	}
	
	return som_rev;
}

const char * __init get_var_som_am33_rev_str(void)
{
	static char som_rev_str[32];

	sprintf(som_rev_str, "1.%d", get_var_som_am33_rev());

	return som_rev_str;
}

#define VAR_SOM_WLAN_PMENA_GPIO             GPIO_TO_PIN(3, 21)
#define VAR_SOM_WLAN_IRQ_GPIO               GPIO_TO_PIN(3, 20)
#define VAR_SOM_BT_PMENA_REV_1_0_GPIO       GPIO_TO_PIN(3, 4)
#define VAR_SOM_BT_PMENA_GPIO               GPIO_TO_PIN(3, 9)

static struct wl12xx_platform_data var_som_am33_wlan_data = {
	.irq                = OMAP_GPIO_IRQ(VAR_SOM_WLAN_IRQ_GPIO),
	.wlan_enable_gpio   = VAR_SOM_WLAN_PMENA_GPIO,
	.bt_enable_gpio     = VAR_SOM_BT_PMENA_GPIO,
	.board_ref_clock    = WL12XX_REFCLOCK_26, /* 26Mhz */
};

static int __init backlight_activate(void)
{
	gpio_set_value(BACKLIGHT_CTRL_PIN, 1);

	return 0;
}
late_initcall(backlight_activate);

static int __init conf_disp_pll(int rate)
{
	struct clk *disp_pll;
	int ret = -EINVAL;

	disp_pll = clk_get(NULL, "dpll_disp_ck");
	if (IS_ERR(disp_pll)) {
		pr_err("TESLAMETER_3MH: Cannot clk_get disp_pll\n");
		goto out;
	}

	ret = clk_set_rate(disp_pll, rate);
	clk_put(disp_pll);
out:
	return ret;
}

static void backlight_init(void)
{
    int status;
    
    pr_info("TESLAMETER_3MH: Backlight init\n");
    setup_pin_mux(backlight_pin_mux);
	status = gpio_request_one(BACKLIGHT_CTRL_PIN, GPIOF_OUT_INIT_LOW, "backlight_ctrl");

	if (status) {
		pr_err("TESLAMETER_3MH: Error requesting LCD backlight control gpio: %d\n", 
		    status);
		    
		return;
	}
	gpio_set_value(BACKLIGHT_CTRL_PIN, 0);
	gpio_export(BACKLIGHT_CTRL_PIN, false);
}

static void buzzer_init(void)
{
    int status;
    
    pr_info("TESLAMETER_3MH: Buzzer init\n");
    setup_pin_mux(buzzer_pin_mux);
    status = gpio_request_one(BUZZER_PIN, GPIOF_OUT_INIT_LOW, "buzzer");
    
    if (status) {
        pr_err("TESLAMETER_3MH: Error requesting Buzzer control gpio: %d\n",
            status);
        
        return;
    }
    gpio_set_value(BUZZER_PIN, 0);
    gpio_export(BUZZER_PIN, false);
}

static void digout_init(void)
{
    int status;
    
    pr_info("TESLAMETER_3MH: DIGOUT init\n");
    setup_pin_mux(digout_pin_mux);
    status = gpio_request_array(digout_gpios, ARRAY_SIZE(digout_gpios));
    
    if (status) {
        pr_err("TESLAMETER_3MH: Error requesting DIGOUT gpios: %d\n", status);
        
        return;
    }
    gpio_export(DIGOUT_1_PIN, false);
    gpio_export(DIGOUT_2_PIN, false);
    gpio_export(DIGOUT_3_PIN, false);
    gpio_export(DIGOUT_4_PIN, false);
    setup_pin_mux(digout_state_pin_mux);
    status = gpio_request_array(digout_state_gpios, 
        ARRAY_SIZE(digout_state_gpios));
    
    if (status) {
        pr_err("TESLAMETER_3MH: Error requesting DIGOUT state gpios: %d\n",
            status);
            
        return;
    }
    gpio_export(DIGOUT_1_STATE_PIN, false);
    gpio_export(DIGOUT_2_STATE_PIN, false);
    gpio_export(DIGOUT_3_STATE_PIN, false);
    gpio_export(DIGOUT_4_STATE_PIN, false);
}

static void digin_init(void)
{
    int status;
    
    pr_info("TESLAMETER_3MH: DIGIN init\n");
    setup_pin_mux(digin_pin_mux);
    status = gpio_request_array(digin_gpios, ARRAY_SIZE(digin_gpios));
    
    if (status) {
        pr_err("TESLAMETER_3MH: Error requesting DIGIN state gpios: %d\n",
            status);
            
        return;
    }
    gpio_export(DIGIN_1_PIN, false);
    gpio_export(DIGIN_2_PIN, false);
    gpio_export(DIGIN_3_PIN, false);
    gpio_export(DIGIN_4_PIN, false);
}

static void lcdc_init(void)
{
    int status;
    
	struct da8xx_lcdc_platform_data* lcdc_data;

    pr_info("TESLAMETER_3MH: LCDC init\n");
    
	lcdc_data = &VAR_LCD_pdata;
	setup_pin_mux(lcdc_pin_mux);
	status = gpio_request_one(LCDC_DISP_STBY, GPIOF_INIT_HIGH, "lcdc_stby");

	if (status) {
        pr_err("TESLAMETER_3MH: Error requesting LCD st-by gpio: %d\n",
            status);
            
        return;
    }
    gpio_set_value(LCDC_DISP_STBY, 1);
    mdelay(20);
    
	if (conf_disp_pll(300000000)) {
		pr_info("TESLAMETER_3MH: Failed configure display PLL, not attempting to "
		        "register LCDC\n");
		return;
	}

	if (am33xx_register_lcdc(lcdc_data))
		pr_err("TESLAMETER_3MH: Failed to register LCDC device\n");
}

static void uart_init(void)
{
    pr_info("TESLAMETER_3MH: UART init\n");
    
	setup_pin_mux(uart0_pin_mux);
	setup_pin_mux(uart3_pin_mux);
    setup_pin_mux(uart5_pin_mux);
    
	omap_serial_init();
}

static void rmii1_init(void)
{
	int status;

    pr_info("TESLAMETER_3MH: RMII1 init\n");
    
	omap_mux_init_gpio(VAR_SOM_GMII1_RST_GPIO, OMAP_PIN_OUTPUT);

	status = gpio_request_one(VAR_SOM_GMII1_RST_GPIO,
		GPIOF_OUT_INIT_HIGH, "rmii1_rst");
	if (status) {
		pr_err("TESLAMETER_3MH: Error requesting rmii1 reset gpio: %d\n", status);
	}

	/* Reset RMII1 */
	mdelay(10);
	gpio_set_value(VAR_SOM_GMII1_RST_GPIO, 0);
	mdelay(10);
	gpio_set_value(VAR_SOM_GMII1_RST_GPIO, 1);

	gpio_export(VAR_SOM_GMII1_RST_GPIO, 0);

	setup_pin_mux(rmii1_pin_mux);

	return;
}

/* NAND partition information
 */
static struct mtd_partition am335x_nand_partitions[] = {
/* All the partition sizes are listed in terms of NAND block size */
	{
		.name           = "SPL",
		.offset         = 0,			/* Offset = 0x0 */
		.size           = SZ_128K,
	},
	{
		.name           = "SPL.backup1",
		.offset         = MTDPART_OFS_APPEND,	/* Offset = 0x20000 */
		.size           = SZ_128K,
	},
	{
		.name           = "SPL.backup2",
		.offset         = MTDPART_OFS_APPEND,	/* Offset = 0x40000 */
		.size           = SZ_128K,
	},
	{
		.name           = "SPL.backup3",
		.offset         = MTDPART_OFS_APPEND,	/* Offset = 0x60000 */
		.size           = SZ_128K,
	},
	{
		.name           = "U-Boot",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x80000 */
		.size           = 15 * SZ_128K,
	},
	{
		.name           = "U-Boot Env",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x260000 */
		.size           = 1 * SZ_128K,
	},
	{
		.name           = "Kernel",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x280000 */
		.size           = 40 * SZ_128K,
	},
	{
		.name           = "File System",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x780000 */
		.size           = MTDPART_SIZ_FULL,
	},
};

static struct gpmc_timings am335x_nand_timings = {
	.sync_clk = 0,

	.cs_on = 0,
	.cs_rd_off = 44,
	.cs_wr_off = 44,

	.adv_on = 6,
	.adv_rd_off = 34,
	.adv_wr_off = 44,
	.we_off = 40,
	.oe_off = 54,

	.access = 64,
	.rd_cycle = 82,
	.wr_cycle = 82,

	.wr_access = 40,
	.wr_data_mux_bus = 0,
};

static void som_nand_init(void)
{
	struct omap_nand_platform_data *pdata;
	struct gpmc_devices_info gpmc_device[2] = {
		{ NULL, 0 },
		{ NULL, 0 },
	};
	
	pr_info("TESLAMETER_3MH: NAND init\n");

	setup_pin_mux(nand_pin_mux);
	pdata = omap_nand_init(am335x_nand_partitions,
		ARRAY_SIZE(am335x_nand_partitions), 0, 0,
		&am335x_nand_timings);
	if (!pdata)
		return;
	pdata->ecc_opt =OMAP_ECC_BCH8_CODE_HW;
	pdata->elm_used = true;
	gpmc_device[0].pdata = pdata;
	gpmc_device[0].flag = GPMC_DEVICE_NAND;

	omap_init_gpmc(gpmc_device, sizeof(gpmc_device));
	omap_init_elm();
}

/* Setup McASP 1 */
static void mcasp1_init(void)
{
    pr_info("TESLAMETER_3MH: MCASP init\n");
    
	/* Configure McASP */
	setup_pin_mux(mcasp1_pin_mux);
	am335x_register_mcasp(&var_am335x_som_snd_data1, 0);
	return;
}

/* WLAN */
static void mmc1_wl12xx_init(void)
{
    pr_info("TESLAMETER_3MH: mmc1_wl12xx_init\n");
    
	setup_pin_mux(mmc1_wl12xx_pin_mux);

	am335x_mmc[1].mmc = 2;
	am335x_mmc[1].name = "wl1271";
	am335x_mmc[1].caps = MMC_CAP_4_BIT_DATA | MMC_CAP_POWER_OFF_CARD
				| MMC_PM_KEEP_POWER;
	am335x_mmc[1].nonremovable = true;
	am335x_mmc[1].gpio_cd = -EINVAL;
	am335x_mmc[1].gpio_wp = -EINVAL;
	am335x_mmc[1].ocr_mask = MMC_VDD_32_33 | MMC_VDD_33_34; /* 3V3 */

	/* mmc will be initialized when mmc0_init is called */
	return;
}

static void uart1_wl12xx_init(void)
{
    pr_info("TESLAMETER_3MH: uart1_wl12xx_init\n");
    
	setup_pin_mux(uart1_wl12xx_pin_mux);
}

#ifdef CONFIG_TI_ST
/* TI-ST for WL12xx BT */

static int plat_kim_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int plat_kim_resume(struct platform_device *pdev)
{
	return 0;
}

static int plat_kim_chip_enable(struct kim_data_s *kim_data)
{
	gpio_direction_output(kim_data->nshutdown, 0);
	msleep(1);
	gpio_direction_output(kim_data->nshutdown, 1);

	return 0;
}

static int plat_kim_chip_disable(struct kim_data_s *kim_data)
{
	gpio_direction_output(kim_data->nshutdown, 0);

	return 0;
}

static struct ti_st_plat_data wilink_pdata = {
	.nshutdown_gpio = VAR_SOM_BT_PMENA_GPIO,
	.dev_name       = "/dev/ttyO1",
	.flow_cntrl     = 1,
	.baud_rate      = 3000000,
	.suspend        = plat_kim_suspend,
	.resume         = plat_kim_resume,
	.chip_enable    = plat_kim_chip_enable,
	.chip_disable   = plat_kim_chip_disable,
};

static struct ti_st_plat_data wilink_rev_1_0_pdata = {
	.nshutdown_gpio = VAR_SOM_BT_PMENA_REV_1_0_GPIO,
	.dev_name       = "/dev/ttyO1",
	.flow_cntrl     = 1,
	.baud_rate      = 3000000,
	.suspend        = plat_kim_suspend,
	.resume         = plat_kim_resume,
	.chip_enable    = plat_kim_chip_enable,
	.chip_disable   = plat_kim_chip_disable,
};

static struct platform_device wl12xx_device = {
	.name           = "kim",
	.id             = -1,
	.dev.platform_data = &wilink_pdata,
};

static struct platform_device btwilink_device = {
	.name = "btwilink",
	.id = -1,
};

static inline void __init var_som_init_btwilink(void)
{
	pr_info("TESLAMETER_3MH: BT init\n");

	if (get_var_som_am33_rev() == 0)
		wl12xx_device.dev.platform_data = &wilink_rev_1_0_pdata;

	platform_device_register(&wl12xx_device);
	platform_device_register(&btwilink_device);
}
#endif

static void wl12xx_bluetooth_enable(void)
{
#ifndef CONFIG_TI_ST
	int status = gpio_request(var_som_am33_wlan_data.bt_enable_gpio, "bt_en\n");
	
	if (status < 0)
		pr_err("TESLAMETER_3MH: Failed to request gpio for bt_enable");

	pr_info("TESLAMETER_3MH: Configure BT Enable pin...\n");
	gpio_direction_output(var_som_am33_wlan_data.bt_enable_gpio, 0);
#else
	var_som_init_btwilink();
#endif
}

static int wl12xx_set_power(struct device *dev, int slot, int on, int vdd)
{
	if (on) {
		gpio_set_value(var_som_am33_wlan_data.wlan_enable_gpio, 1);
		mdelay(70);
	}
	else
		gpio_set_value(var_som_am33_wlan_data.wlan_enable_gpio, 0);

	return 0;
}

static void __init wl12xx_init(void)
{
#if (KM_CONFIG_WL12XX == 1) 
    pr_info("TESLAMETER_3MH: not setting wl12xx\n");
#else
	struct device *dev;
	struct omap_mmc_platform_data *pdata;
	int ret;

	/* Register WLAN enable pin */
	if (wl12xx_set_platform_data(&var_som_am33_wlan_data))
		pr_err("TESLAMETER_3MH: Error setting wl12xx data\n");

	dev = am335x_mmc[1].dev;
	if (!dev) {
		pr_err("TESLAMETER_3MH: wl12xx mmc device initialization failed\n");
		goto out;
	}

	pdata = dev->platform_data;
	if (!pdata) {
		pr_err("TESLAMETER_3MH: Platfrom data of wl12xx device not set\n");
		goto out;
	}

	if (get_var_som_am33_rev() == 0){
		setup_pin_mux(wl12xx_cb_rev_1_0_pin_mux_var_som);
		var_som_am33_wlan_data.bt_enable_gpio = VAR_SOM_BT_PMENA_REV_1_0_GPIO;
	}
	else {
		setup_pin_mux(wl12xx_pin_mux_var_som);
		var_som_am33_wlan_data.bt_enable_gpio = VAR_SOM_BT_PMENA_GPIO;
	}

	wl12xx_bluetooth_enable();

	ret = gpio_request_one(var_som_am33_wlan_data.wlan_enable_gpio,
		GPIOF_OUT_INIT_LOW, "wlan_en");
	if (ret) {
		pr_err("TESLAMETER_3MH: Error requesting wlan enable gpio: %d\n", ret);
		goto out;
	}

	pdata->slots[0].set_power = wl12xx_set_power;
out:
	return;
#endif
}

static void __init mmc0_init(void)
{
	setup_pin_mux(mmc0_pin_mux);
	omap2_hsmmc_init(am335x_mmc);
}

#define VAR_SOM_KS8051_PHY_ID       0x00221556
#define VAR_SOM_KS8051_PHY_MASK     0xffffffff

#define VAR_SOM_KSZ9021_PHY_ID      0x00221611
#define VAR_SOM_KSZ9021_PHY_MASK    0xffffffff

static int var_som_ks8051_phy_fixup(struct phy_device *phydev)
{
	if (get_var_som_am33_rev() <= 1)
		phydev->dev_flags |= MICREL_PHY_50MHZ_CLK;

	/* override strap options, set RMII mode */
	phy_write(phydev, 0x16, 0x2);

	return 0;
}

static int var_som_ksz9021_phy_fixup(struct phy_device *phydev)
{
	/* Fine-tune clock and control pad skew */
	phy_write(phydev, 0xb, 0x8104);
	phy_write(phydev, 0xc, 0xA097);

	/* Fine-tune RX data pad skew */
	phy_write(phydev, 0xb, 0x8105);
	phy_write(phydev, 0xc, 0);

	return 0;
}

static void ethernet_init(void)
{
	int mode;
	
	pr_info("TESLAMETER_3MH: Ethernet init\n");

	/* Setup fallback MAC addresses, used only if eFuse MACID is invalid.
	 */
	am335x_mac_addr[0][0]=0xF8;
	am335x_mac_addr[0][1]=0xdc;
	am335x_mac_addr[0][2]=0x7a;
	am335x_mac_addr[0][3]=0x00;
	am335x_mac_addr[0][4]=0x11;
	am335x_mac_addr[0][5]=0x22;

	am335x_mac_addr[1][0]=0xF8;
	am335x_mac_addr[1][1]=0xdc;
	am335x_mac_addr[1][2]=0x7a;
	am335x_mac_addr[1][3]=0x00;
	am335x_mac_addr[1][4]=0x11;
	am335x_mac_addr[1][5]=0x23;
	am33xx_cpsw_macidfillup(am335x_mac_addr[0],	am335x_mac_addr[1]);

	phy_register_fixup_for_uid(VAR_SOM_KS8051_PHY_ID, VAR_SOM_KS8051_PHY_MASK,
				   var_som_ks8051_phy_fixup);

	phy_register_fixup_for_uid(VAR_SOM_KSZ9021_PHY_ID, VAR_SOM_KSZ9021_PHY_MASK,
				   var_som_ksz9021_phy_fixup);

	mode = (get_var_som_am33_rev() >= 2) 
		? AM33XX_CPSW_MODE_VAR2 : AM33XX_CPSW_MODE_VAR;

	am33xx_cpsw_init(mode, "0:01", "0:07");
}

static struct regulator_init_data am335x_dummy = {
	.constraints.always_on	= true,
};

static struct regulator_consumer_supply am335x_vdd1_supply[] = {
	REGULATOR_SUPPLY("vdd_mpu", NULL),
};

static struct regulator_init_data am335x_vdd1 = {
	.constraints = {
		.min_uV			= 600000,
		.max_uV			= 1500000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE,
		.always_on		= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(am335x_vdd1_supply),
	.consumer_supplies	= am335x_vdd1_supply,
};

static struct regulator_consumer_supply am335x_vdd2_supply[] = {
	REGULATOR_SUPPLY("vdd_core", NULL),
};

static struct regulator_init_data am335x_vdd2 = {
	.constraints = {
		.min_uV			    = 600000,
		.max_uV			    = 1500000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE,
		.always_on		    = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(am335x_vdd2_supply),
	.consumer_supplies	    = am335x_vdd2_supply,
};

static struct tps65910_board am335x_tps65910_info = {
	.tps65910_pmic_init_data[TPS65910_REG_VRTC]	    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VIO]	    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDD1]	    = &am335x_vdd1,
	.tps65910_pmic_init_data[TPS65910_REG_VDD2]	    = &am335x_vdd2,
	.tps65910_pmic_init_data[TPS65910_REG_VDD3]	    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDIG1]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDIG2]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VPLL]	    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDAC]	    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX1]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX2]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX33]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VMMC]	    = &am335x_dummy,
};

static struct omap_musb_board_data musb_board_data = {
	.interface_type	= MUSB_INTERFACE_ULPI,
	/*
	 * mode[0:3] = USB0PORT's mode
	 * mode[4:7] = USB1PORT's mode
	 */
	.mode           = (MUSB_OTG << 4) | MUSB_HOST,
	.power		    = 500,
	.instances	    = 1,
};

#define VAR_SOM_TSC_CTW_IRQ_GPIO 	GPIO_TO_PIN(0, 3)

static struct i2c_board_info __initdata var_som_i2c1_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65910", TPS65910_I2C_ID1),
		.platform_data  = &am335x_tps65910_info,
	},
	{
		I2C_BOARD_INFO("tlv320aic3x", 0x1b),
	},
};

static struct i2c_board_info __initdata var_som_i2c2_boardinfo[] = {
};

static void i2c1_init(void)
{
    pr_info("TESLAMETER_3MH: I2C1 init\n");
    
	setup_pin_mux(i2c1_pin_mux);
	omap_register_i2c_bus(2, 100, var_som_i2c1_boardinfo,
			ARRAY_SIZE(var_som_i2c1_boardinfo));
}

static void i2c2_init(void)
{
    pr_info("TESLAMETER_3MH: I2C2 init\n");
    
    setup_pin_mux(i2c2_pin_mux);
    omap_register_i2c_bus(3, 100, var_som_i2c2_boardinfo,
        ARRAY_SIZE(var_som_i2c2_boardinfo));
}

/* Enable clkout2 */
static struct pinmux_config clkout2_pin_mux[] = {
	{"xdma_event_intr1.clkout2", OMAP_MUX_MODE3 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

static void __init clkout2_enable(void)
{
	struct clk *ck_32;
	void __iomem *base;
	
	pr_info("TESLAMETER_3MH: clockout2 init for WLAN\n");

	base = ioremap(AM33XX_RTC_BASE, SZ_4K);

	if (WARN_ON(!base)) {
		pr_err("TESLAMETER_3MH: Failed to ioremap RTC base addr\n");
		return;
	}

	/* Unlock the rtc's registers */
	writel(0x83e70b13, base + 0x6c);
	writel(0x95a4f1e0, base + 0x70);

	/* Enable the 32K OSc */
	writel(0x48, base + 0x54);

	iounmap(base);

	ck_32 = clk_get(NULL, "clkout2_ck");
	if (IS_ERR(ck_32)) {
		pr_err("TESLAMETER_3MH: 32k: Cannot Get Clock\n");
		return;
	}

	if (clk_enable(ck_32)) {
		pr_err("TESLAMETER_3MH: 32k: Clock Enable Failed\n");
		return;
	}

	setup_pin_mux(clkout2_pin_mux);
}

static struct resource am33xx_cpuidle_resources[] = {
	{
		.start		= AM33XX_EMIF0_BASE,
		.end		= AM33XX_EMIF0_BASE + SZ_32K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

/* AM33XX devices support DDR2 power down */
static struct am33xx_cpuidle_config am33xx_cpuidle_pdata = {
	.ddr2_pdown	= 1,
};

static struct platform_device am33xx_cpuidle_device = {
	.name			    = "cpuidle-am33xx",
	.num_resources		= ARRAY_SIZE(am33xx_cpuidle_resources),
	.resource		    = am33xx_cpuidle_resources,
	.dev = {
		.platform_data	= &am33xx_cpuidle_pdata,
	},
};

static void __init am33xx_cpuidle_init(void)
{
	extern void __iomem * __init am33xx_get_mem_ctlr(void);
	int ret;

    pr_info("TESLAMETER_3MH: AM33XX cpuidle registration\n");
    
	am33xx_cpuidle_pdata.emif_base = am33xx_get_mem_ctlr();

	ret = platform_device_register(&am33xx_cpuidle_device);

	if (ret)
		pr_warning("TESLAMETER_3MH: AM33XX cpuidle registration failed\n");
}

static void __init var_som_am335x_init(void)
{
	am33xx_cpuidle_init();
	am33xx_mux_init(board_mux);

    
	uart_init();
	printk ("TESLAMETER_3MH: Variscite VAR-SOM-AM33 revision %s detected\n",
			get_var_som_am33_rev_str());
	buzzer_init();
	backlight_init();
	digout_init();
	digin_init();
	clkout2_enable();                              /* Required by WLAN module */
	mmc1_wl12xx_init();
	mmc0_init();
	uart1_wl12xx_init();
	wl12xx_init();
	som_nand_init();
	lcdc_init();
	mcasp1_init();
	rmii1_init();
	ethernet_init();
	i2c1_init();
	i2c2_init();
	usb_musb_init(&musb_board_data);

	/* Create an alias for icss clock */
	if (clk_add_alias("pruss", NULL, "pruss_uart_gclk", NULL))
		pr_warn("TESLAMETER_3MH: Failed to create an alias: "
		    "icss_uart_gclk --> pruss\n");
		    
	/* Create an alias for gfx/sgx clock */
	if (clk_add_alias("sgx_ck", NULL, "gfx_fclk", NULL))
		pr_warn("TESLAMETER_3MH: Failed to create an alias: "
    		"gfx_fclk --> sgx_ck\n");
}

static void __init teslameter_am335x_som_map_io(void)
{
	omap2_set_globals_am33xx();
	omapam33xx_map_common_io();
}

MACHINE_START(TESLAMETER_3MH, "TESLAMETER_3MH")
	.atag_offset	= 0x100,
	.map_io		    = teslameter_am335x_som_map_io,
	.init_early	    = am33xx_init_early,
	.init_irq	    = ti81xx_init_irq,
	.handle_irq     = omap3_intc_handle_irq,
	.timer		    = &omap3_am33xx_timer,
	.init_machine	= var_som_am335x_init,
MACHINE_END
