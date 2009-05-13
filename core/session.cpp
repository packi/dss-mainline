
#include "session.h"

namespace dss {

  Session::Session(const int _tokenID)
  : m_Token(_tokenID)
  {
    m_LastTouched = DateTime();
  } // ctor

  bool Session::IsStillValid() {
    //const int TheSessionTimeout = 5;
    return true;//m_LastTouched.AddMinute(TheSessionTimeout).After(DateTime());
  } // IsStillValid

  bool Session::HasSetWithID(const int _id) {
    SetsByID::iterator iKey = m_SetsByID.find(_id);
    return iKey != m_SetsByID.end();
  } // HasSetWithID

  Set& Session::GetSetByID(const int _id) {
    SetsByID::iterator iEntry = m_SetsByID.find(_id);
    return iEntry->second;
  } // GetSetByID

  Set& Session::AllocateSet(int& id) {
    id = ++m_LastSetNr;
    m_SetsByID[id] = Set();
    return m_SetsByID[id];
  } // AllocateSet

  Set& Session::AddSet(Set _set, int& id) {
    id = ++m_LastSetNr;
    m_SetsByID[id] = _set;
    return m_SetsByID[id];
  } // AddSet

  void Session::FreeSet(const int _id) {
    m_SetsByID.erase(m_SetsByID.find(_id));
  } // FreeSet

  Session& Session::operator=(const Session& _other) {
    m_LastSetNr = _other.m_LastSetNr;
    m_Token = _other.m_Token;
    m_LastTouched = _other.m_LastTouched;

    for(SetsByID::const_iterator iEntry = m_SetsByID.begin(), e = m_SetsByID.end();
        iEntry != e; ++iEntry)
    {
      m_SetsByID[iEntry->first] = iEntry->second;
    }
    return *this;
  }

} // namespace dss
