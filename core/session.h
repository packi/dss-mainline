
#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "datetools.h"
#include "model.h"

namespace dss {

  typedef map<const int, Set> SetsByID;

  class Session {
  private:
    int m_Token;
    uint32_t m_OriginatorIP;

    int m_LastSetNr;
    DateTime m_LastTouched;
    SetsByID m_SetsByID;
  public:
    Session(const int _tokenID);

    bool IsStillValid();
    bool HasSetWithID(const int _id);
    Set& GetSetByID(const int _id);
    Set& AllocateSet(int& _id);
    void FreeSet(const int id);
    Set& AddSet(Set _set, int& id);

    Session& operator=(const Session& _other);
  }; // Session


}

#endif // defined SESSION_H_INCLUDED
