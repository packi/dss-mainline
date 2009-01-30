/*
 * seriespersistence.cpp
 *
 *  Created on: Jan 30, 2009
 *      Author: patrick
 */

#include "seriespersistence.h"

#include "core/xmlwrapper.h"
#include "core/logger.h"

namespace dss {

  const int SeriesXMLFileVersion = 1;

  //================================================== SeriesReader

  template<class T>
  Series<T>* SeriesReader<T>::ReadFromXML(const string& _fileName) {
    XMLDocumentFileReader reader(_fileName);

    XMLNode rootNode = reader.GetDocument().GetRootNode();
    if(rootNode.GetName() == "values") {
      if(rootNode.GetAttributes()["version"] != IntToString(SeriesXMLFileVersion)) {
        Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: Version mismatch, expected ") + IntToString(SeriesXMLFileVersion) + " got " + rootNode.GetAttributes()["version"],
                                   lsError);
        return NULL;
      }
      int numberOfValues = StrToIntDef(rootNode.GetAttributes()["numberOfValues"], -1);
      if(numberOfValues == -1) {
        Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: numberOfValues attribute missing or invalid: ") + rootNode.GetAttributes()["numberOfValues"],
                                   lsError);
        return NULL;
      }
      int resolution = StrToIntDef(rootNode.GetAttributes()["resolution"], -1);
      if(resolution == -1) {
        Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: resolution attribute missing or invalid: ") + rootNode.GetAttributes()["numberOfValues"],
                                   lsError);
        return NULL;
      }

      Series<T>* result = new Series<T>(resolution, numberOfValues);
      XMLNodeList& values = rootNode.GetChildren();
      for(XMLNodeList::iterator iNode = values.begin(), e = values.end();
          iNode != e; ++iNode)
      {
        if(iNode->GetName() == "value") {
          try {
            T value(0);
            value.LoadFromXMLNode(*iNode);
            result.AddValue(value);
          } catch(runtime_error& e) {
            Logger::GetInstance()->Log(string("SeriesReader::ReadFromXML: Error while reading value: ") + e.what());
          }
        }
      }
      return result;
    }
    return NULL;
  }

  //================================================== Explicit instantiations
  template class SeriesReader<AdderValue>;

} // namespace dss
