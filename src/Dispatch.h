#ifndef Dispatch_h
#define Dispatch_h

#include <Arduino.h>
class Dispatch {
public:
  void run();
  int add(void (*Fptr)(), unsigned long, unsigned long);
  void remove(void (*Fptr)());
  bool expired();
  unsigned long runtime();
  unsigned long delaytime();
private:
  struct RunTable {
    unsigned long ScheduleInterval = 0;
    unsigned long LastRunTime = 0;
    unsigned long MaxRunTime = 0;
    void (*Fptr)();
    struct RunTable *next=NULL;
  };
  struct RunTable *_DispatchList_ = NULL;
  unsigned long _GlobalKillTime_=0;
  unsigned long _GlobalStartTime_=0;
  unsigned long _GlobalOverrunTime_=0;
};
#endif
