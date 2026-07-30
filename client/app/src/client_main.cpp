#include <iostream>
#include <string>
#include <vector>

#include "client/app/include/client_app.hpp"

// https://github.com/hadasagavra/KungFuChess
//
// The client executable's entry point: a thin shim. Everything -- arg parsing,
// board loading, mode selection, the frame loop -- lives in the client/app layer,
// mirroring how server_main hands off to the RoomManager. main() only collects
// the arguments and starts the client.
int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    return kfc::app::runClient(args, std::cin, std::cout);
}
