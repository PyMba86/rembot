
#include <iostream>
#include <SFML/Graphics.hpp>
#include "editor.h"
#include "core.h"

int main() {

    sf::RenderWindow window(sf::VideoMode(800, 600), "rembot", sf::Style::Titlebar | sf::Style::Close);

    window.setVerticalSyncEnabled(true);

    rb::Editor editor(&window);
    auto core = std::make_shared<rb::Core>();
    sf::Clock timer;

    editor.setStateData(core->getStateData());
    core->setStateInput(editor.getStateInput());

    editor.setEventCallback(rb::Editor::BUTTON_INIT, [core]() { core->notifyEvent(rb::Core::Init); });
    editor.setEventCallback(rb::Editor::BUTTON_DATA_ON, [core]() { core->notifyEvent(rb::Core::DataOn); });
    editor.setEventCallback(rb::Editor::BUTTON_DATA_OFF, [core]() { core->notifyEvent(rb::Core::DataOff); });

    core->init();

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            editor.processEvent(event);
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        editor.update(timer.restart());
        core->update();

        window.clear();

        editor.render();

        window.display();
    }
    core->exit();
    editor.exit();
    return 0;
}