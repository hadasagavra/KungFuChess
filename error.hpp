#pragma once

#include <stdexcept>

namespace kfc {

class Error : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

}