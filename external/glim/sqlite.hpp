#ifndef GLIM_SQLITE_HPP_
#define GLIM_SQLITE_HPP_

/**
 * A threaded interface to <a href="http://sqlite.org/">SQLite</a>.
 * This file is a header-only library,
 * whose sole dependencies should be standard STL and posix threading libraries.
 * You can extract this file out of the "glim" library to include it separately in your project.
 * @code
Copyright 2006-2012 Kozarezov Artem Aleksandrovich

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 * @endcode
 * @file
 */

#include <stdexcept>
#include <string>
#include <sqlite3.h>
#include <pthread.h>
#include <string.h> // strerror
#include <sys/types.h> // stat
#include <sys/stat.h> // stat
#include <unistd.h> // stat
#include <errno.h> // stat
#include <stdio.h> // snprintf
#include <stdint.h>

namespace glim {

class SqliteSession;
class SqliteQuery;

struct SqliteEx: public std::runtime_error {
  SqliteEx (const std::string& what): std::runtime_error (what) {}
};

/**
 * The database.
 * According to sqlite3_open <a href="http://sqlite.org/capi3ref.html#sqlite3_open">documentation</a>,
 * only the thread that opened the database can safely access it. This restriction was
 * relaxed, as described in the <a href="http://www.sqlite.org/faq.html#q8">FAQ</a>
 * (the "Is SQLite threadsafe?" question), so we can use the library from multiple
 * threads, but only if no more than one thread at a time accesses the database.
 * This restriction is, in fact, beneficial if the database is used from a single application:
 * by restricting access to a sigle thread at a time, we effectively avoid all deadlock issues.\n
 * This library goals are:\n
 * \li to ensure that SQLite is used in a thread-safe way,
 * \li to provide additional threaded quirks, such as delayed updates (not implemented).
 * 
 * The library is targeted at SQLite setup which is \b not \c -DTHREADSAFE,
 * since this is the default setup on UNIX architectures.\n
 * \n
 * This file is a header-only library,
 * whose sole dependencies should be standard STL and posix threading libraries.
 * You can extract this file out of the "glim" library to include it separately in your project.\n
 * \n
 * This library is targeted at UTF-8 API. There is no plans to support the UTF-16 API.\n
 * \n
 * See also:\n
 * \li http://www.sqlite.org/cvstrac/fileview?f=sqlite/src/server.c\n
 *     for another way of handling multithreading with SQLite.
 */
class Sqlite {
  /// No copying allowed.
  Sqlite& operator = (const Sqlite& other) {return *this;}
  /// No copying allowed.
  Sqlite (const Sqlite& other) = delete;
  friend class SqliteSession;
  protected:
  /// Filename the database was opened with; we need it to reopen the database on fork()s.
  /// std::string is used to avoid memory allocation issues.
  std::string filename;
  ::sqlite3* handler;
  ::pthread_mutex_t mutex;
  public:
  /// Flags for the Sqlite constructor.
  enum Flags {
    /**
     * The file will be checked for existence.
     * SqliteEx is thrown if the file is not accessible;
     * format of the error description is "$filename: $strerror".\n
     * Usage example: \code Sqlite db ("filename", Sqlite::existing); \endcode
     */
    existing = 1
  };
  /**
   * Opens the database.
   * @param filename Database filename (UTF-8).
   * @param flags Optional. Currently there is the #existing flag.
   * @throws SqliteEx Thrown if we can't open the database.
   */
  Sqlite (std::string filename, int flags = 0) {
    if (flags & existing) {
      // Check if the file exists already.
      struct stat st; if (stat (filename.c_str(), &st))
        throw SqliteEx (filename + ": " + ::strerror(errno));
    }
    ::pthread_mutex_init (&mutex, NULL);
    this->filename = filename;
    if (::sqlite3_open(filename.c_str(), &handler) != SQLITE_OK)
      throw SqliteEx (std::string("sqlite3_open(") + filename + "): " + ::sqlite3_errmsg(handler));
  }
  /**
   * Closes the database.
   * @throws SqliteEx Thrown if we can't close the database.
   */
  ~Sqlite () {
    ::pthread_mutex_destroy (&mutex);
    if (::sqlite3_close(handler) != SQLITE_OK)
      throw SqliteEx (std::string ("sqlite3_close(): ") + ::sqlite3_errmsg(handler));
  }

  Sqlite& exec (const char* query);
  /**
   * Invokes `exec` on `query.c_str()`.
   * Example:\code
   *   glim::Sqlite sqlite (":memory:");
   *   for (std::string pv: {"page_size = 4096", "secure_delete = 1"}) sqlite->exec2 ("PRAGMA " + pv); \endcode
   */
  template <typename StringLike> Sqlite& exec2 (StringLike query) {return exec (query.c_str());}
};

/**
 * A single thread session with Sqlite.
 * Only a sigle thread at a time can have an SqliteSession,
 * all other threads will wait, in the SqliteSession constructor,
 * till the active session is either closed or destructed.
 */
class SqliteSession {
  /// No copying allowed.
  SqliteSession& operator = (const SqliteSession& other) {return *this;}
  /// No copying allowed.
  SqliteSession(SqliteSession& other): db (NULL) {}
  protected:
  Sqlite* db;
  public:
  /**
   * Locks the database.
   * @throws SqliteEx if a mutex error occurs.
   */
  SqliteSession (Sqlite* sqlite): db (sqlite) {
    int err = ::pthread_mutex_lock (&(db->mutex));
    if (err != 0) throw SqliteEx (std::string ("error locking the mutex: ") + ::strerror(err));
  }
  /**
   * A shorter way to construct query from the session.
   * Usage example: \code ses.query(S("create table test (i integer)")).step() \endcode
   * @see SqliteQuery#qstep
   */
  template <typename T>
  SqliteQuery query (T t);
  /// Automatically unlocks the database.
  /// @see close
  ~SqliteSession () {close();}
  /**
   * Unlock the database.
   * It is safe to call this method multiple times.\n
   * You must not use the session after it was closed.\n
   * All resources allocated within this session must be released before the session is closed.
   * @throws SqliteEx if a mutex error occurs.
   */
  void close () {
    if (db == NULL) return;
    int err = ::pthread_mutex_unlock (&(db->mutex));
    db = NULL;
    if (err != 0) throw SqliteEx (std::string ("error unlocking the mutex: ") + ::strerror(err));
  }
  /// True if the \c close method has been already called on this SqliteSession.
  bool isClosed () const {
    return db == NULL;
  }
  /**
   * This class can be used in place of the SQLite handler.
   * Make sure you've released any resources thus manually acquired before this SqliteSession is closed.
   * Usage example:
   * @code
   * glim::Sqlite db (":memory:");
   * glim::SqliteSession ses (&db);
   * sqlite3_exec (ses, "PRAGMA page_size = 4096;", NULL, NULL, NULL);
   * @endcode
   */
  operator ::sqlite3* () const {return db->handler;}
};

/**
 * Execute the given query, throwing SqliteEx on failure.\n
 * Example:\code
 *   glim::Sqlite sqlite (":memory:");
 *   sqlite.exec ("PRAGMA page_size = 4096") .exec ("PRAGMA secure_delete = 1"); \endcode
 */
inline Sqlite& Sqlite::exec (const char* query) {
  SqliteSession ses (this); // Maintains the locks.
  char* errmsg = NULL; ::sqlite3_exec (handler, query, NULL, NULL, &errmsg);
  if (errmsg) throw SqliteEx (std::string ("Sqlite::exec, error in query (") + query + "): " + errmsg);
  return *this;
}

/**
 * Wraps the sqlite3_stmt; will prepare it, bind values, query and finalize.
 */
class SqliteQuery {
 protected:
  ::sqlite3_stmt* statement;
  SqliteSession* session;
  int bindCounter;
  /// -1 if statement isn't DONE.
  int mChanges;
  void prepare (SqliteSession* session, char const* query, int queryLength) {
    ::sqlite3* handler = *session;
    if (::sqlite3_prepare_v2 (handler, query, queryLength, &statement, NULL) != SQLITE_OK)
      throw SqliteEx (std::string(query, queryLength) + ": " + ::sqlite3_errmsg(handler));
  }
  /** Shan't copy. */
  SqliteQuery (const SqliteQuery& other) = delete;
 public:
  SqliteQuery (SqliteQuery&& rvalue) {
    statement = rvalue.statement;
    session = rvalue.session;
    bindCounter = rvalue.bindCounter;
    mChanges = rvalue.mChanges;
    rvalue.statement = nullptr;
  }
  /**
   * Prepares the query.
   * @throws SqliteEx if sqlite3_prepare fails; format of the error message is "$query: $errmsg".
   */
  SqliteQuery (SqliteSession* session, char const* query, int queryLength)
    : statement (NULL), session (session), bindCounter (0), mChanges (-1) {
    prepare (session, query, queryLength);
  }
  /**
   * Prepares the query.
   * @throws SqliteEx if sqlite3_prepare fails; format of the error message is "$query: $errmsg".
   */
  SqliteQuery (SqliteSession* session, std::pair<char const*, int> query)
    : statement (NULL), session (session), bindCounter (0), mChanges (-1) {
    prepare (session, query.first, query.second);
  }
  /**
   * Prepares the query.
   * @throws SqliteEx if sqlite3_prepare fails; format of the error message is "$query: $errmsg".
   */
  SqliteQuery (SqliteSession* session, std::string query)
    : statement (NULL), session (session), bindCounter (0), mChanges (-1) {
    prepare (session, query.c_str(), query.length());
  }
  /**
   * Release resources.
   * @see http://sqlite.org/capi3ref.html#sqlite3_finalize
   */
  ~SqliteQuery () {
    if (statement) ::sqlite3_finalize (statement);
  }
  
  /// Call this (followed by the #step) if you need the query to be re-executed.
  /// @see http://sqlite.org/capi3ref.html#sqlite3_reset
  SqliteQuery& reset () {
    bindCounter = 0;
    mChanges = -1;
    ::sqlite3_reset (statement);
    return *this;
  }
  
  /// Synonym for #step.
  bool next () {return step();}
  /**
   * Invokes sqlite3_step.
   * @return \c true if there was a row fetched successfully, \c false if there is no more rows.
   * @see http://sqlite.org/capi3ref.html#sqlite3_step
   */
  bool step () {
    if (mChanges >= 0) {mChanges = 0; return false;}
    int ret = ::sqlite3_step (statement);
    if (ret == SQLITE_ROW) return true;
    if (ret == SQLITE_DONE) {
      mChanges = ::sqlite3_changes (*session);
      return false;
    }
    throw SqliteEx (std::string(::sqlite3_errmsg(*session)));
  }
  /**
   * Perform #step and throw an exception if #step has returned \c false.
   * Usage example:
   * \code (ses.query(S("select count(*) from test where idx = ?")) << 12345).qstep().intAt(1) \endcode
   */
  SqliteQuery& qstep () {
    if (!step())
      throw SqliteEx (std::string("qstep: no rows returned / affected"));
    return *this;
  }
  /**
   * Invokes a DML query and returns the number of rows affected.
   * Example: \code
   * int affected = (ses.query(S("update test set count = count + ? where id = ?")) << 1 << 9).ustep();
   * \endcode
   * @see http://sqlite.org/capi3ref.html#sqlite3_step
   */
  int ustep () {
    int ret = ::sqlite3_step (statement);
    if (ret == SQLITE_DONE) {
      mChanges = ::sqlite3_changes (*session);
      return mChanges;
    }
    if (ret == SQLITE_ROW) return 0;
    throw SqliteEx (std::string(::sqlite3_errmsg(*session)));
  }
  
  /**
   * The number of rows changed by the query. 
   * Providing the query was a DML (Data Modification Language),
   * returns the number of rows updated.\n
   * If the query wasn't a DML, returned value is undefined.\n
   * -1 is returned if the query wasn't executed, or after #reset.\n
   * Example: \code
   * SqliteQuery query (&ses, S("update test set count = count + ? where id = ?"));
   * query.bind (1, 1);
   * query.bind (2, 9);
   * query.step ();
   * int affected = query.changes ();
   * \endcode
   * @see #ustep
   */
  int changes () {return mChanges;}
  
  /**
   * The integer value of the given column.
   * @param column 1-based.
   * @see http://sqlite.org/capi3ref.html#sqlite3_column_text
   */
  int intAt (int column) {
    return ::sqlite3_column_int (statement, --column);
  }
  
  /**
   * The integer value of the given column.
   * @param column 1-based.
   * @see http://sqlite.org/capi3ref.html#sqlite3_column_text
   */
  sqlite3_int64 int64at (int column) {
    return ::sqlite3_column_int64 (statement, --column);
  }
  
  /**
   * The floating point number from the given column.
   * @param column 1-based.
   * @see http://sqlite.org/capi3ref.html#sqlite3_column_text
   */
  double doubleAt (int column) {
    return ::sqlite3_column_double (statement, --column);
  }
  
  /**
   * Return the column as UTF-8 characters, which can be used until the next #step.
   * @param column 1-based.
   * @see http://sqlite.org/capi3ref.html#sqlite3_column_text
   */
  std::pair<char const*, int> charsAt (int column) {
    return std::pair<char const*, int> ((char const*) ::sqlite3_column_text (statement, column-1),
                                        ::sqlite3_column_bytes (statement, column-1));
  }
  
  /**
   * Return the column as C++ string (UTF-8).
   * @param column 1-based.
   */
  std::string stringAt (int column) {
    return std::string ((char const*) ::sqlite3_column_text (statement, column-1),
                        ::sqlite3_column_bytes (statement, column-1));
  }
  
  /**
   * The type of the column.
   * SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB or SQLITE_NULL.
   * @param column 1-based.
   * @see http://sqlite.org/capi3ref.html#sqlite3_column_text
   */
  int typeAt (int column) {
    return ::sqlite3_column_type (statement, --column);
  }
  
  /**
   * Binds a value using one of the bind methods.
   */
  template<typename T>
  SqliteQuery& operator << (T value) {
    return bind (++bindCounter, value);
  }
  /**
   * Binds a value using the named parameter and one of the bind methods.
   * @throws SqliteEx if the name could not be found.
   * @see http://sqlite.org/capi3ref.html#sqlite3_bind_parameter_index
   */
  template<typename T>
  SqliteQuery& bind (char const* name, T value) {
    int index = ::sqlite3_bind_parameter_index (statement, name);
    if (index == 0)
      throw SqliteEx (std::string ("No such parameter in the query: ") + name);
    return bind (index, value);
  }
  
  /**
   * Bind a string to the query.
   * @param transient must be true, if lifetime of the string might be shorter than that of the query.
   */
  SqliteQuery& bind (int index, const char* text, int length, bool transient = false) {
    if (::sqlite3_bind_text (statement, index, text, length,
                             transient ? SQLITE_TRANSIENT : SQLITE_STATIC) != SQLITE_OK)
      throw SqliteEx (std::string (::sqlite3_errmsg (*session)));
    return *this;
  }
  /**
   * Bind a string to the query.
   * @param transient must be true, if lifetime of the string might be shorter than that of the query.
   */
  SqliteQuery& bind (int index, std::pair<const char*, int> text, bool transient = false) {
    if (::sqlite3_bind_text (statement, index, text.first, text.second,
                             transient ? SQLITE_TRANSIENT : SQLITE_STATIC) != SQLITE_OK)
      throw SqliteEx (std::string (::sqlite3_errmsg (*session)));
    return *this;
  }
  /**
   * Bind a string to the query.
   * @param transient must be true, if lifetime of the string might be shorter than that of the query.
   */
  SqliteQuery& bind (int index, const std::string& text, bool transient = true) {
    if (::sqlite3_bind_text (statement, index, text.data(), text.length(),
                             transient ? SQLITE_TRANSIENT : SQLITE_STATIC) != SQLITE_OK)
      throw SqliteEx (std::string (::sqlite3_errmsg (*session)));
    return *this;
  }
  /**
   * Bind an integer to the query.
   */
  SqliteQuery& bind (int index, int value) {
    if (::sqlite3_bind_int (statement, index, value) != SQLITE_OK)
      throw SqliteEx (std::string (::sqlite3_errmsg (*session)));
    return *this;
  }
  /**
   * Bind an 64-bit integer to the query.
   */
  SqliteQuery& bind (int index, sqlite3_int64 value) {
    if (::sqlite3_bind_int64 (statement, index, value) != SQLITE_OK)
      throw SqliteEx (std::string (::sqlite3_errmsg (*session)));
    return *this;
  }
};

/**
 * Version of SqliteQuery suitable for using SQLite in parallel with other processes.
 * Will automatically handle the SQLITE_SCHEMA error
 * and will automatically repeat attempts after SQLITE_BUSY,
 * but it requires that the query string supplied
 * is constant and available during the SqliteParQuery lifetime.
 * Error messages, contained in exceptions, may differ from SqliteQuery by containing the query
 * (for example, the #step method will throw "$query: $errmsg" instead of just "$errmsg").
 */
class SqliteParQuery: public SqliteQuery {
  protected:
  char const* query;
  int queryLength;
  int repeat;
  int wait;
  public:
  /**
   * Prepares the query.
   * @param repeat the number of times we try to repeat the query when SQLITE_BUSY is returned.
   * @param wait how long, in milliseconds (1/1000 of a second) we are to wait before repeating.
   * @throws SqliteEx if sqlite3_prepare fails; format of the error message is "$query: $errmsg".
   */
  SqliteParQuery (SqliteSession* session, char const* query, int queryLength, int repeat = 90, int wait = 20)
    : SqliteQuery (session, query, queryLength) {
    this->query = query;
    this->queryLength = queryLength;
    this->repeat = repeat;
    this->wait = wait;
  }
  /**
   * Prepares the query.
   * @param query the SQL query together with its length.
   * @param repeat the number of times we try to repeat the query when SQLITE_BUSY is returned.
   * @param wait how long, in milliseconds (1/1000 of a second) we are to wait before repeating.
   * @throws SqliteEx if sqlite3_prepare fails; format of the error message is "$query: $errmsg".
   */
  SqliteParQuery (SqliteSession* session, std::pair<char const*, int> query, int repeat = 90, int wait = 20)
    : SqliteQuery (session, query) {
    this->query = query.first;
    this->queryLength = query.second;
    this->repeat = repeat;
    this->wait = wait;
  }
  
  bool next () {return step();}
  bool step () {
    if (mChanges >= 0) {mChanges = 0; return false;}
    repeat:
    int ret = ::sqlite3_step (statement);
    if (ret == SQLITE_ROW) return true;
    if (ret == SQLITE_DONE) {
      mChanges = ::sqlite3_changes (*session);
      return false;
    }
    if (ret == SQLITE_SCHEMA) {
      ::sqlite3_stmt* old = statement;
      prepare (session, query, queryLength);
      ::sqlite3_transfer_bindings(old, statement);
      ::sqlite3_finalize (old);
      goto repeat;
    }
    if (ret == SQLITE_BUSY) for (int repeat = this->repeat; ret == SQLITE_BUSY && repeat >= 0; --repeat) {
      //struct timespec ts; ts.tv_sec = 0; ts.tv_nsec = wait * 1000000; // nan is 10^-9 of sec.
      //while (::nanosleep (&ts, &ts) == EINTR);
      ::sqlite3_sleep (wait);
      ret = ::sqlite3_step (statement);
    }
    throw SqliteEx (std::string(query, queryLength) + ::sqlite3_errmsg(*session));
  }
};

template <typename T>
SqliteQuery SqliteSession::query (T t) {
  return SqliteQuery (this, t);
}

}; // namespace glim

#endif // GLIM_SQLITE_HPP_
