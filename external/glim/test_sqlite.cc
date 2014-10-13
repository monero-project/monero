
#include "sqlite.hpp"
#define S(cstr) (std::pair<char const*, int> (cstr, sizeof (cstr) - 1))
#include <assert.h>
#include <iostream>
using namespace glim;

int main () {
  std::cout << "Testing sqlite.hpp ... " << std::flush;
  Sqlite sqlite (":memory:");
  SqliteSession sqs (&sqlite);
  assert (sqs.query ("CREATE TABLE test (t TEXT, i INTEGER)") .ustep() == 0);
  assert (sqs.query ("INSERT INTO test VALUES (?, ?)") .bind (1, S("foo")) .bind (2, 27) .ustep() == 1);
  assert (sqs.query ("SELECT t FROM test") .qstep() .stringAt (1) == "foo");
  assert (sqs.query ("SELECT i FROM test") .qstep() .intAt (1) == 27);
  std::cout << "pass." << std::endl;
  return 0;
}
