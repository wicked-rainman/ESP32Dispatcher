#include "Dispatch.h"
//----------------------------------------------------------------------------
// Dispatch add
//
// Purpose: Add a function pointer to the link list of tasks to be invoked
// 
// Parameters:
// Fptr		a pointer to the function that should be invoked
// delay: 	Time (in milliseconds) to delay before invoking the function.
// killtime: 	How long the function should run for before being elligible to 
//           	break out and return
//
// ---------------------------------------------------------------------------
int Dispatch::add(void (*Fptr)(), unsigned long Mdelay, unsigned long killtime) {
static struct RunTable *_link_ = NULL;
static struct RunTable *_lastnode_ = NULL;

  //----------------------------------------
  // Create a new linklist node.
  // ---------------------------------------
  _link_ = (struct RunTable *) malloc(sizeof(struct RunTable));
  if(_link_==NULL) return 0; //because malloc failed
  _link_->ScheduleInterval = Mdelay;
  _link_->MaxRunTime = killtime;
  _link_->LastRunTime = millis();
  _link_->Fptr = Fptr;
  _link_->next = NULL;
  //---------------------------------
  // If a link list head hasn't been 
  // created then make this it. 
  // --------------------------------
  if(_DispatchList_==NULL) _DispatchList_ =_link_;
  //-----------------------------------
  // Else, find the end of the existing
  // link list.
  // ----------------------------------
  else {
    _lastnode_ = _DispatchList_;
    while(_lastnode_->next !=NULL) _lastnode_ = _lastnode_->next;
    //----------------------------
    // Add the new node onto the
    // end of the existing linklist
    // ----------------------------
    _lastnode_->next = _link_;
  }
  return 1;
}

//---------------------------------------------------------------------------
// Dispatch remove
//
// Purpose: Remove a function from the linklist of functions to be executed.
//
// Parameters:
//
// Jobptr: A pointer to the function that is to be removed
//
//---------------------------------------------------------------------------

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
  struct RunTable *ptr = NULL;
  struct RunTable *Candidate = NULL;
  unsigned long WhenTime = 0;
  unsigned long Overrun = 0;
  unsigned long StoredOverrun = 0;
  static unsigned long Ctime=0;
  //----------------------------------------
  // Check for millis() rollover (which
  // will occur if this has been executing 
  // for roughly 49 days). This will show
  // up if the previous millis value (Ctime) 
  // is greater than current millis value. 
  // If this is the case, then cycle through 
  // the task link list, and reset LastRunTime 
  // to the current millis time value.
  //----------------------------------------
  if(Ctime > millis()) {
    ptr = _DispatchList_;
    while(ptr !=NULL) {
      ptr->LastRunTime=millis();
      ptr = ptr->next;
    }
  }
  //----------------------------------------
  // Store the current time in millis.
  // Set up a pointer to cycle through the
  // link list. 
  //----------------------------------------
  Ctime=millis();
  ptr = _DispatchList_;
  //----------------------------------------
  // Cycle through the link list looking for
  // the task that is most overdue (it could
  // be that none of them are late).
  //----------------------------------------
  while (ptr != NULL) {
    //---------------------------------------
    //A task is elligible to run when you add
    //together the wanted delay and the time
    //when the task was last run (All in Ms)
    //---------------------------------------
    WhenTime = ptr->LastRunTime+ptr->ScheduleInterval;
    //------------------------------------
    // If the current time is greater than
    // the expected run time for this task
    // -----------------------------------
    if(WhenTime <= Ctime) {
      //------------------------------
      // Calculate the overrun time for
      // this task
      //-------------------------------
      Overrun = Ctime-WhenTime;
      //--------------------------------------
      // If task has the longest overrun time.
      //-------------------------------------
      if (Overrun > StoredOverrun) {
	//-------------------------------------
	// Store how big this overrun is
	//------------------------------------
        StoredOverrun = Overrun;
	//------------------------------------
        // Make the task a possible candidate.
	//-----------------------------------
        Candidate = ptr;
      }
    }
    ptr = ptr->next;
  }
  //----------------------------------------
  // Run the most delayed task (If any). 
  //----------------------------------------
  if (Candidate != NULL) {
    //--------------------------------------------------
    // Store the time when  the task will be eligible for 
    // killing, how long it was delayed by and how long  
    // it ran for.
    //--------------------------------------------------
    _GlobalKillTime_ = Candidate->MaxRunTime + Ctime ;
    _GlobalStartTime_=Ctime;
    _GlobalOverrunTime_=StoredOverrun; 
    Candidate->LastRunTime = Ctime ;
    //-------------------
    // Execute the task
    //-------------------
    Candidate->Fptr();
  }
}

// ----------------------------------------------------------
//
// Dispatch expired
//
// Purpose:	The test function to decide if the currently 
// 		function has exhausted it's execution time.
//
// Return value: Boolean True of False
//
// ----------------------------------------------------------
bool Dispatch::expired() {
  if (millis() >= _GlobalKillTime_) return true; 
  return false;
}

// -------------------------------------------------------------
//
// Dispatch runtime
//
// Purpose: To return how long a function has been running
//
// Return value: Run time in milliseconds (Unsigned long int)
//
// ------------------------------------------------------------
unsigned long  Dispatch::runtime() {
  return(millis()-_GlobalStartTime_);
}	

// ----------------------------------------------------------------
//
// Dispatch delaytime
//
// Purpose: To return how long a function's execution was delayed
//
// Return value: Delay time in milliseconds (Unsigned long int)
//
// ----------------------------------------------------------------
unsigned long Dispatch::delaytime() {
  return(_GlobalOverrunTime_);
}
