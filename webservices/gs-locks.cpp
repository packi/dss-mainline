/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org> 

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "webservices/soapH.h"
#include "webservices/gs-locks.h"

#include <openssl/crypto.h>

#include <unistd.h>
#include <pthread.h>

#include "src/logger.h"

using namespace dss;

struct CRYPTO_dynlock_value { 
    pthread_mutex_t mutex;
};

static pthread_mutex_t *mutex_buffer;
static int number_of_locks = -1;

static struct CRYPTO_dynlock_value *lock_create_callback(
                                                    const char *file, int line)
{
  struct CRYPTO_dynlock_value *val;
  val = (struct CRYPTO_dynlock_value*)malloc(sizeof(struct CRYPTO_dynlock_value));
  if (val) {
    if (pthread_mutex_init(&(val->mutex), NULL)) {
        Logger::getInstance()->log(
                "lock_create_callback: could not initialize mutex!", lsFatal);
    }
  }

  return val;
}

static void lock_destroy_callback(struct CRYPTO_dynlock_value *l, const char *file, int line)
{
  if (!l) {
    Logger::getInstance()->log("lock_destroy_callback: inalid mutex!", lsFatal);
    return;
  }

  if (pthread_mutex_destroy(&(l->mutex))) {
    Logger::getInstance()->log("lock_destroy_callback: could not unlock mutex!",
                               lsFatal);
  }
  free(l);
}

static void locking_callback(int mode, int n, const char *file, int line)
{
  if ((n < 0) || (n >= number_of_locks)) {
      Logger::getInstance()->log("locking callback: inalid mutex index!", 
                                 lsFatal);
    return;
  }
          
  if (mode & CRYPTO_LOCK) {
    if (pthread_mutex_lock(&(mutex_buffer[n]))) {
      Logger::getInstance()->log("locking_callback: could not lock mutex!", 
                                 lsFatal);
    }
  } else {
    if (pthread_mutex_unlock(&(mutex_buffer[n]))) {
      Logger::getInstance()->log("locking_callback: could not unlock mutex!", 
                                 lsFatal);
    }
  }
}


static void lock_callback(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line)
{
  if (!l) {
    Logger::getInstance()->log("lock_callback: invalid mutex!", 
                               lsFatal);
    return;
  }

  if (mode & CRYPTO_LOCK) {
    if (pthread_mutex_lock(&(l->mutex))) {
      Logger::getInstance()->log("lock_callback: could not lock mutex!", 
                                 lsFatal);
    }
  } else {
    if (pthread_mutex_unlock(&(l->mutex))) {
      Logger::getInstance()->log("lock_callback: could not unlock mutex!", 
                                 lsFatal);
    }
  }
}

static unsigned long thread_id_callback()
{ 
  return (unsigned long)pthread_self();
}

int CRYPTO_setup_locks()
{ 
  number_of_locks = CRYPTO_num_locks();
  mutex_buffer = (pthread_mutex_t *)malloc(number_of_locks * sizeof(pthread_mutex_t));
  if (!mutex_buffer) {
    return SOAP_EOM;
  }

  for (int i = 0; i < number_of_locks; i++) {
    if (pthread_mutex_init(&(mutex_buffer[i]), NULL)) {
        number_of_locks = i;
        CRYPTO_cleanup_locks();
        Logger::getInstance()->log(
                "CRYPTO_setup_locks: failed to initialize mutexes!", lsFatal);
        return SOAP_FATAL_ERROR;
    }
  }

  CRYPTO_set_dynlock_create_callback(lock_create_callback);
  CRYPTO_set_dynlock_destroy_callback(lock_destroy_callback);
  CRYPTO_set_locking_callback(locking_callback);
  CRYPTO_set_dynlock_lock_callback(lock_callback);
  CRYPTO_set_id_callback(thread_id_callback);
  return SOAP_OK;
}

void CRYPTO_cleanup_locks()
{ 
  if (!mutex_buffer) {
    return;
  }

  CRYPTO_set_dynlock_create_callback(NULL);
  CRYPTO_set_dynlock_destroy_callback(NULL);
  CRYPTO_set_locking_callback(NULL);
  CRYPTO_set_dynlock_lock_callback(NULL);
  CRYPTO_set_id_callback(NULL);
  for (int i = 0; i < number_of_locks; i++) {
    pthread_mutex_destroy(&(mutex_buffer[i]));
  }

  free(mutex_buffer);
  mutex_buffer = NULL;
  number_of_locks = -1;
}


