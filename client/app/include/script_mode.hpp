#pragma once

#include <istream>
#include <ostream>

namespace kfc::app {

// The text harness: read a board + a script from `in`, replay the commands
// through the public command path, and print board dumps to `out`. It drives a
// local engine directly through a LocalGameAccess -- a test tool, not networked
// play, so it does not go over the server protocol. Returns a process exit code.
int runScript(std::istream& in, std::ostream& out);

}  // namespace kfc::app
