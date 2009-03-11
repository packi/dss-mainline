/*
 * setbuilder.h
 *
 *  Created on: Oct 27, 2008
 *      Author: patrick
 */

#ifndef SETBUILDER_H_
#define SETBUILDER_H_

#include "model.h"

#include <string>

using std::string;

namespace dss {
  class SetBuilder {
  protected:
	  Set RestrictBy(const string& _identifier, const Set& _set, const Zone& _zone);
	  Set RestrictByFunction(const string& _identifier, const Set& _set, const Zone& _zone);
	public:
	  SetBuilder();

	  Set BuildSet(const string& _setDescription, const Zone* _context);
  }; // SetBuilder

} // namespace dss

#endif /* SETBUILDER_H_ */
