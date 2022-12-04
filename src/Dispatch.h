#ifndef Dispatch_h
#define Dispatch_h

#define KILLME (millis() >= _GlobalKillTime_)
#include <Arduino.h>
class Dispatch {
public:
  void run();
  void add(void (*Fptr)(), unsigned long, unsigned long);
  void remove(void (*Fptr)());
  bool expired();
  unsigned long runtime();
  unsigned long delaytime();
private:
  struct RunTable {
    unsigned long ScheduleInterval;
    unsigned long LastRunTime = 0;
    unsigned long MaxRunTime;
    void (*Fptr)();
    struct RunTable *next;
  };
  struct RunTable *_DispatchList_ = NULL;
  unsigned long _GlobalKillTime_;
  unsigned long _GlobalStartTime_;
  unsigned long _GlobalOverrunTime_;
};
#endif
