#include "eventlist.h"
//#include <iostream>

EventList::EventList()
    : _endtime(0),
      _lasteventtime(0)
{}

void
EventList::setEndtime(simtime_picosec endtime)
{
    _endtime = endtime;
}

bool
EventList::doNextEvent()
{
    if (_pendingsources.empty())
        return false;
    simtime_picosec nexteventtime = _pendingsources.begin()->first;
    EventSource* nextsource = _pendingsources.begin()->second;
    _pendingsources.erase(_pendingsources.begin());
    assert(nexteventtime >= _lasteventtime);
    _lasteventtime = nexteventtime; // set this before calling doNextEvent, so that this::now() is accurate
    nextsource->doNextEvent();
    return true;
}


void
EventList::sourceIsPending(EventSource &src, simtime_picosec when)
{
    assert(when>=now());
    if (_endtime==0 || when<_endtime)
        _pendingsources.insert(make_pair(when,&src));
}
