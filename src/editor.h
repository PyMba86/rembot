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
            BUTTON_INIT,
            BUTTON_DATA_ON,
            BUTTON_DATA_OFF
        };

        void setEventCallback(Event e, std::function<void()> && callback);

    private:
        void createGridLines();

        bool _showGridLines;
        bool _windowHasFocus;

        sf::RenderWindow* _window;
        std::shared_ptr<rb::detail::Graphics>  _graphics;
        std::vector<std::array<sf::Vertex, 2>> _gridLines;
        detail::Level _level;
        detail::WindowTypes _currentWindowType;

        struct Data;
        std::unique_ptr<Data> _data;

    };
}