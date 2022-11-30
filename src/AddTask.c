void AddTask(char *Descr, void (*Fptr)(), unsigned long delay, unsigned long killtime) {
  static struct RunTable *_link_ = NULL;
  static struct RunTable *_lastnode_ = NULL;
  if (_DispatchList_ == NULL) {
    _DispatchList_ = (struct RunTable *)malloc(sizeof(struct RunTable));
    strncpy(_DispatchList_->TaskDescription, Descr, 100);
    _DispatchList_->ScheduleInterval = delay;
    _DispatchList_->MaxRunTime = killtime;
    _DispatchList_->Fptr = Fptr;
    _DispatchList_->next = NULL;
    Serial.printf("#AddTask: Inserted \"%s\", Interval=%lu, Runtime=%lu, Func=%p\n",
                  _DispatchList_->TaskDescription, _DispatchList_->ScheduleInterval, _DispatchList_->MaxRunTime, _DispatchList_->Fptr);
    _lastnode_ = _DispatchList_;
  }

  else {
    _link_ = (struct RunTable *)malloc(sizeof(struct RunTable));
    strncpy(_link_->TaskDescription, Descr, 100);
    _link_->ScheduleInterval = delay;
    _link_->MaxRunTime = killtime;
    _link_->Fptr = Fptr;
    _link_->next = NULL;
    Serial.printf("#AddTask: Inserted \"%s\", Interval=%lu, Runtime=%lu, Func=%p\n",
                  _link_->TaskDescription, _link_->ScheduleInterval, _link_->MaxRunTime, _link_->Fptr);
    _lastnode_->next = _link_;
    _lastnode_ = _link_;
  }
}
