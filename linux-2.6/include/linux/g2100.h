#ifndef __G2100_H__
#define __G2100_H__

void g2100_reset(void);
int g2100_request(unsigned int irq, irq_handler_t handler,
	               unsigned long flags, const char *name, void *dev);
void g2100_free(unsigned int, void *);
void g2100_enable(unsigned int irq);
irqreturn_t g2100_disable(unsigned int irq);

#endif /* __G2100_H__ */
