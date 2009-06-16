
#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "datetools.h"
#include "model.h"

#include <map>

namespace dss {

  typedef std::map<const int, Set> SetsByID;

  class Session {
  private:
    int m_Token;

    int m_LastSetNr;
    DateTime m_LastTouched;
    SetsByID m_SetsByID;
  public:
    Session() {}
    Session(const int _tokenID);

    bool isStillValid();
    bool hasSetWithID(const int _id);
    Set& getSetByID(const int _id);
    Set& allocateSet(int& _id);
    void freeSet(const int id);
    Set& addSet(Set _set, int& id);

    Session& operator=(const Session& _other);
  }; // Session


}

#endif // defined SESSION_H_INCLUDED
