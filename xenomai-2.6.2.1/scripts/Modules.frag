DRIVERS-$(CONFIG_XENO_OPT_NUCLEUS) += kernel/xenomai/nucleus/xeno_nucleus.o
DRIVERS-$(CONFIG_XENO_SKIN_NATIVE) += kernel/xenomai/skins/native/xeno_native.o
DRIVERS-$(CONFIG_XENO_SKIN_POSIX) += kernel/xenomai/skins/posix/xeno_posix.o
DRIVERS-$(CONFIG_XENO_SKIN_RTDM) += kernel/xenomai/skins/rtdm/xeno_rtdm.o
DRIVERS-$(CONFIG_XENO_SKIN_PSOS) += kernel/xenomai/skins/psos+/xeno_psos.o
DRIVERS-$(CONFIG_XENO_SKIN_VRTX) += kernel/xenomai/skins/vrtx/xeno_vrtx.o
DRIVERS-$(CONFIG_XENO_SKIN_VXWORKS) += kernel/xenomai/skins/vxworks/xeno_vxworks.o
DRIVERS-$(CONFIG_XENO_SKIN_UITRON) += kernel/xenomai/skins/uitron/xeno_uitron.o
DRIVERS-$(CONFIG_XENO_SKIN_RTAI) += kernel/xenomai/skins/rtai/xeno_rtai.o
DRIVERS-$(CONFIG_XENO_DRIVERS_16550A) += drivers/xenomai/serial/xeno_16550A.o
DRIVERS-$(CONFIG_XENO_DRIVERS_TIMERBENCH) += drivers/xenomai/testing/xeno_timerbench.o
DRIVERS-$(CONFIG_XENO_DRIVERS_IRQBENCH) += drivers/xenomai/testing/xeno_irqbench.o
DRIVERS-$(CONFIG_XENO_DRIVERS_SWITCHTEST) += drivers/xenomai/testing/xeno_switchtest.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN) += drivers/xenomai/can/xeno_can.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_VIRT) += drivers/xenomai/can/xeno_can_virt.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_MSCAN) += drivers/xenomai/can/mscan/xeno_can_mscan.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_SJA1000) += drivers/xenomai/can/sja1000/xeno_can_sja1000.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_SJA1000_ISA) += drivers/xenomai/can/sja1000/xeno_can_isa.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_SJA1000_MEM) += drivers/xenomai/can/sja1000/xeno_can_mem.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_SJA1000_PEAK_PCI) += drivers/xenomai/can/sja1000/xeno_can_peak_pci.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_SJA1000_PEAK_DNG) += drivers/xenomai/can/sja1000/xeno_can_peak_dng.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_SJA1000_IXXAT_PCI) += drivers/xenomai/can/sja1000/xeno_can_ixxat_pci.o
DRIVERS-$(CONFIG_XENO_DRIVERS_CAN_SJA1000_EMS_PCI) += drivers/xenomai/can/sja1000/xeno_can_ems_pci.o
