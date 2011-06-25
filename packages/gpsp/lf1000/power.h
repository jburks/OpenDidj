#ifndef Power
#define Power

int open_input_device(char *input_name);
int GrabPowerButton(void);
int MonitorPower(int fd);
extern char cPower;

#endif
