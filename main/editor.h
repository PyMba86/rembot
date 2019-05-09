#pragma once

#include <memory>
#include <functional>

namespace CG {
    class Window2D;
}

class Editor {
public:
    Editor();

    ~Editor();

    bool init(std::weak_ptr<CG::Window2D> window);

    void update();

    void render() const;

    void terminate();

private:

    void createGridLines(bool always = false);

    bool _showGridLines;

};