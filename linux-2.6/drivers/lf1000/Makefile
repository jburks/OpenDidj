#
# Makefile for the kernel LF1000 device drivers.
#

lf1000-dma-objs := dma.o
lf1000-ga3d-objs := ga3d.o

obj-$(CONFIG_LF1000_GA3D)	+= lf1000-ga3d.o
obj-$(CONFIG_LF1000_DMA)	+= lf1000-dma.o
obj-$(CONFIG_LF1000_ADC)	+= adc/
obj-$(CONFIG_LF1000_OSS)	+= i2s/
obj-$(CONFIG_LF1000_MLC)	+= mlc/
obj-$(CONFIG_LF1000_DPC)	+= dpc/
obj-$(CONFIG_LF1000_IDCT)	+= idct.o
obj-$(CONFIG_LFCART)		+= lfcart.o
