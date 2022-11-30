#define KILLME (millis() >= _GlobalKillTime_)
struct RunTable {
  unsigned long ScheduleInterval;
  unsigned long LastRunTime=0;
  unsigned long MaxRunTime;
  char TaskDescription[100];
  void (*Fptr)();
  struct RunTable *next;
};
struct RunTable *_DispatchList_=NULL;
unsigned long _GlobalKillTime_;
void AddTask(char *, void (*Fptr)(), unsigned long , unsigned long ) ;
void Dispatch();
