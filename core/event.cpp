/*
 * event.cpp
 *
 *  Created on: Oct 24, 2008
 *      Author: patrick
 */

#include "event.h"

#include "base.h"

namespace dss {

  //================================================== EventInterpreter

  EventInterpreter::EventInterpreter(EventQueue* _queue) {
  } // ctor

  EventInterpreter::~EventInterpreter() {
    ScrubVector(m_Plugins);
    ScrubVector(m_Subscriptions);
  } // dtor

  void EventInterpreter::AddPlugin(EventInterpreterPlugin* _plugin) {
    m_Plugins.push_back(_plugin);
  } // AddPlugin

  void EventInterpreter::Execute() {
    Event* toProcess = NULL;
    while(!m_Terminated && (toProcess = m_Queue->PopEvent()) != NULL) {

    }
  } // Execute

  void EventInterpreter::Subscribe(EventSubscription* _subscription) {
    m_Subscriptions.push_back(_subscription);
  } // Subscribe

  void EventInterpreter::Unsubscribe(const string& _subscriptionID) {
    for(vector<EventSubscription*>::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
        ipSubscription != e; ++ipSubscription)
    {
      if((*ipSubscription)->GetID() == _subscriptionID) {
        m_Subscriptions.erase(ipSubscription);
        break;
      }
    }
  } // Unsubscribe


  //================================================== EventQueue

  void EventQueue::PushEvent(Event* _event) {
    // TODO: lock queue
    m_EventQueue.push(_event);
  } // PushEvent

  Event* EventQueue::PopEvent() {
    // TODO: lock queue
    Event* result = m_EventQueue.front();
    m_EventQueue.pop();
    return result;
  } // PopEvent


} // namespace dss
