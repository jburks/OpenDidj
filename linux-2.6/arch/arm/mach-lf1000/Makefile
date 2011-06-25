#
# Makefile for the linux kernel.
#

obj-y					:= core.o clock.o clock_dev.o
obj-$(CONFIG_LF1000_GPIO)		+= gpio_main.o gpio.o gpio_l2p.o
obj-$(CONFIG_ARCH_LF1000)		+= lf1000_core_func.o
obj-$(CONFIG_ARCH_LF1000)		+= lf1000.o
obj-$(CONFIG_ARCH_LF1000)		+= pwm.o
obj-$(CONFIG_LF1000_SCREEN)		+= screen.o
