#pragma once

namespace kfc::model {


class GameState {
public:
    bool isOver() const { return over_; }
    void markOver() { over_ = true; }

private:
    bool over_ = false;
};

}