#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <string>
#include <functional>
#include "detail.h"

namespace rb {

    struct StateData;
    struct StateInput;

    class Editor {
    public:
        explicit Editor(sf::RenderWindow* window);
        ~Editor();
        void render();
        void update(sf::Time elapsedTime);
        void exit();

        void processEvent(sf::Event &event);

        void setStateData(std::weak_ptr<StateData> stateData);
        std::weak_ptr<StateInput> getStateInput() const;

        enum Event {
            WINDOW_CLOSE,
            BUTTON_CONNECT,
            BUTTON_DISCONNECT,
            BUTTON_PLAY,
            BUTTON_STOP
        };

        void setEventCallback(Event e, std::function<void()> && callback);

    private:
        void createGridLines();

        bool _showGridLines;
        bool _windowHasFocus;
        bool _hideShapes;
        bool _showEntityList;
        bool _attachPoint;

        int _menuClicks;

        sf::RenderWindow* _window;
        std::shared_ptr<rb::detail::Graphics>  _graphics;
        std::vector<std::array<sf::Vertex, 2>> _gridLines;
        detail::Level _level;
        detail::WindowTypes _currentWindowType;
        detail::MapEditorMode _currentMapEditorMode;
        detail::DrawShapes _currentDrawShape;
        sf::Event _currentEvent;

        struct Data;
        std::unique_ptr<Data> _data;

    };
}