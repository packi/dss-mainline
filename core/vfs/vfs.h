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
    VFS();

    boost::shared_ptr<Reader> CreateReader(const string& _uri);
    boost::shared_ptr<Writer> CreateWriter(const string& _uri);
  }; // VFS
  
  class Processor {
  private:
    string m_Name;
  public:
    Processor(const string& _name);
    
    int Process(const char* _in, unsigned int _inLen, char* _out, int _outLen);
    
    bool IsPreprocessor();
  }; // Processor
  
  class Reader {
  private:
    string m_Name;
  public:
    Reader(const string& _name);
    
    int Read(char* _out, int _outLen);
  };
  
  class Writer {
  private:
    string m_Name;
  public:
    Writer(const string& _name);
    
    int Write(const char* _in, int _inLen);
  };

}

#endif /* VFS_H_ */
