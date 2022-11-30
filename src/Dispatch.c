void Dispatch(int debugLevel) {
  struct RunTable *ptr = _DispatchList_;
  struct RunTable *Candidate;
  unsigned long LapseTime;
  unsigned long Overrun = 0;
  unsigned long StoredOverrun = 0;
  Candidate = NULL;
  while (ptr != NULL) {
    LapseTime = millis() - ptr->LastRunTime;
    if (LapseTime >= ptr->ScheduleInterval) {
      Overrun = LapseTime - ptr->ScheduleInterval;
      if (Overrun > 0) {
        if (debugLevel == 1 || debugLevel == 3) Serial.printf("#Dispatch: \"%s\" overdue (%luMs)\n", ptr->TaskDescription, Overrun);
        if (Overrun > StoredOverrun) {
          StoredOverrun = Overrun;
          Candidate = ptr;
        }
      }
    }
    ptr = ptr->next;
  }
  if (Candidate != NULL) {
    _GlobalKillTime_ = Candidate->MaxRunTime + millis();
    Candidate->LastRunTime = millis();
    Candidate->Fptr();
    if (debugLevel >= 2) Serial.printf("#Dispatch: \"%s\" took %luMs\n", Candidate->TaskDescription, (millis() - Candidate->LastRunTime));
  }
}
