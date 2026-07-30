#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace kfc::app {

// The client composition root, as a function: parse the arguments, load the
// opening board, and run the chosen mode -- the text harness (--script), a local
// (loopback) game, or a networked game (prompting for credentials first). Kept
// out of main() so the executable's entry point is a thin shim. Returns a process
// exit code.
int runClient(const std::vector<std::string>& args, std::istream& in,
              std::ostream& out);

}  // namespace kfc::app
