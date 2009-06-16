/*
 * vfs.h
 *
 *  Created on: Mar 19, 2009
 *      Author: patrick
 */

#ifndef VFS_H_
#define VFS_H_

#include <boost/shared_ptr.hpp>

namespace dss {

  class Reader;
  class Writer;
  class Processor;

  /** System that returns a Reader or Writer according to an uri.
   * The URI may be prefixed by one (or many) processors. If you'd
   * like to get an Reader to a local encrypted file the uri would
   * be: decrypt+file://data/file.dat
   *
   * To write an encrypted stream you'd write to:
   * encrypt+file://data/file.dat.
   *
   */

  class VFS {
  public:
    vFS();

    boost::shared_ptr<Reader> createReader(const string& _uri);
    boost::shared_ptr<Writer> createWriter(const string& _uri);
  }; // VFS
  
  class Processor {
  private:
    string m_Name;
  public:
    processor(const string& _name);
    
    int process(const char* _in, unsigned int _inLen, char* _out, int _outLen);
    
    bool isPreprocessor();
  }; // Processor
  
  class Reader {
  private:
    string m_Name;
  public:
    reader(const string& _name);
    
    int read(char* _out, int _outLen);
  };
  
  class Writer {
  private:
    string m_Name;
  public:
    writer(const string& _name);
    
    int write(const char* _in, int _inLen);
  };

}

#endif /* VFS_H_ */
