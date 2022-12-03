#include "Dispatch.h"
//----------------------------------------------------------------------------
// Dispatch add
//
// Purpose: Add a function pointer to the link list of tasks to be invoked
// 
// Parameters:
// Fptr		a pointer to the function that should be invoked
// delay: 	Time (in milliseconds) to delay before invoking the function.
// killtime: 	How long the function can run for before being elligible to 
//           	break out and return
//
// Author: WickedRainman
// ---------------------------------------------------------------------------
 
void Dispatch::add(void (*Fptr)(), unsigned long delay, unsigned long killtime) {
static struct RunTable *_link_ = NULL;
static struct RunTable *_lastnode_ = NULL;
  if (_DispatchList_ == NULL) {
    _DispatchList_ = (struct RunTable *)malloc(sizeof(struct RunTable));
    _DispatchList_->ScheduleInterval = delay;
    _DispatchList_->MaxRunTime = killtime;
    _DispatchList_->Fptr = Fptr;
    _DispatchList_->next = NULL;
    _lastnode_ = _DispatchList_;
  }

  else {
    _link_ = (struct RunTable *)malloc(sizeof(struct RunTable));
    _link_->ScheduleInterval = delay;
    _link_->MaxRunTime = killtime;
    _link_->Fptr = Fptr;
    _link_->next = NULL;
    _lastnode_->next = _link_;
    _lastnode_ = _link_;
  }
}

// ---------------------------------------------------------------------------
// Dispatch remove
//
// Purpose: Remove a function from the linklist of functions to be executed.
//
// Parameters:
//
// Jobptr: A pointer to the function that is to be removed
//
// ---------------------------------------------------------------------------

void Dispatch::remove(void (*Jobptr)()) {
  static struct RunTable *current = NULL;
  static struct RunTable *previous = NULL;
  current = _DispatchList_;
  while (current != NULL) {
    if (Jobptr == current->Fptr) {
      if (previous == NULL) {
        _DispatchList_ = current->next;
        free(current);
        return;
      } 
      else {
        previous->next = current->next;
        free(current);
        return;
      }
    } 
    else {
      previous = current;
      current = current->next;
    }
  }
}

// ---------------------------------------------------------------------------
// Dispatch run
//
// Purpose: 	To cycle through the linklist of functions and to invoke each
// 		function if the defined time lapse has occurred. The function
// 		that has been delayed the longest will be invoked first
//
// Parameters: None 
// ---------------------------------------------------------------------------

void Dispatch::run() {
  static int FirstRun=0;
  struct RunTable *ptr;
  struct RunTable *Candidate = NULL;
  unsigned long LapseTime;
  unsigned long Overrun = 0;
  unsigned long StoredOverrun = 0;

  //If this is the first time Dispatch->run has been executed
  //All the LastRunTIme entries need to be set to the current
  //millisecond value - else all function running will be
  //executed as over-delayed

  if(FirstRun == 0) {
    FirstRun = 1;
    ptr = _DispatchList_;
    while(ptr !=NULL) {
        ptr->LastRunTime=millis();
        ptr = ptr->next;
    }
  }
// Not the first run, so look for candidates that
// need to be run by going through the link list
// looking for the most out of date expected
// runtime.  
  else {
    ptr = _DispatchList_;
    Candidate = NULL;
    while (ptr != NULL) {
      LapseTime = millis() - ptr->LastRunTime;
      if (LapseTime >= ptr->ScheduleInterval) {
        Overrun = LapseTime - ptr->ScheduleInterval;
        if (Overrun > StoredOverrun) {
          StoredOverrun = Overrun;
          Candidate = ptr;
        }
      }
      ptr = ptr->next;
    }
  }  
  if (Candidate != NULL) {
    _GlobalKillTime_ = Candidate->MaxRunTime + millis();
    _GlobalStartTime_=millis();
    _GlobalOverrunTime_=StoredOverrun; 
    Candidate->LastRunTime = millis();
    Candidate->Fptr();
  }
}

// ---------------------------------------------------------------------------
//
// Dispatch expired
//
// Purpose:	The test function to decide if the currently function has
// 		exhausted it's execution time.
//
// Return value: True of False
//
// ---------------------------------------------------------------------------

bool Dispatch::expired() {
  if (millis() >= _GlobalKillTime_) return true; 
  return false;
}

unsigned long  Dispatch::runtime() {
	return(millis()-_GlobalStartTime_);
}	

unsigned long Dispatch::delaytime() {
	return(_GlobalOverrunTime_);
}
