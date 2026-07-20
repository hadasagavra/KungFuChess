#pragma once

#include "error.hpp"

namespace kfc::model {

// Base for any Board contract violation; catch it to handle them all at once.
class BoardError : public kfc::Error {
public:
    using kfc::Error::Error;
};

class OutOfBoundsError : public BoardError {
public:
    using BoardError::BoardError;
};

class CellOccupiedError : public BoardError {
public:
    using BoardError::BoardError;
};

class CellEmptyError : public BoardError {
public:
    using BoardError::BoardError;
};

}