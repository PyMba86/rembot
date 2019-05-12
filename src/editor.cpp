#include "editor.h"

#include <regex>

#include "../libext/imgui.h"
#include "../libext/imgui-SFML.h"
#include "../libext/imgui_internal.h"

#include "data.h"

namespace rb {

    struct Editor::Data {
        Data() : stateInput(new StateInput()) {}
        std::shared_ptr<StateInput> stateInput;
        std::weak_ptr<StateData> stateData;
        std::shared_ptr<StateData> stateDataLocked;

        std::map<Event, std::function<void()>> callbacks;

        };

    Editor::Editor(sf::RenderWindow *window) :
            _showGridLines(true),
            _windowHasFocus(true),
            _graphics(new detail::Graphics(window)),
            _level(this->_graphics),
            _currentWindowType(detail::WindowTypes::NewMapWindow),
            _data(new Data){

        ImGui::SFML::Init(*window);
        this->_window = window;
        this->createGridLines();
        if (auto & c = _data->callbacks[BUTTON_INIT]) c();
    }

    void Editor::render() {

        if (!(_data->stateDataLocked = _data->stateData.lock())) {
           // CG_WARN(0, "Missing state data\n");
        }

        const auto & data = _data->stateDataLocked;

        //Return the current mouse position
        static auto getMousePos = [&]() -> sf::Vector2f {
            return this->_window->mapPixelToCoords(sf::Vector2i(
                    sf::Mouse::getPosition(*this->_window).x +
                    static_cast<int>(this->_graphics->getView().getViewport().left),
                    sf::Mouse::getPosition(*this->_window).y +
                    static_cast<int>(this->_graphics->getView().getViewport().top)));
        };

        this->_window->clear(sf::Color(30, 30, 30, 255));

        if (this->_showGridLines) {
            for (auto &t : this->_gridLines) {
                this->_graphics->draw(t.data(), 2, sf::Lines);
            }
        }


        sf::Vector2f mousePos = getMousePos();


        auto mousePosBoxX = (this->_level.getSize().x * this->_level.getTileSize().x *
         std::stof(
                 detail::utils::getConfigValue("tile_scale_x")));

        auto mousePosBoxY = (this->_level.getSize().y * this->_level.getTileSize().y *
                             std::stof(
                                     detail::utils::getConfigValue("tile_scale_y")));


        if (mousePos.x >= 0 && (mousePos.x <= mousePosBoxX) &&
            mousePos.y >= 0 && mousePos.y <= mousePosBoxY) {

            auto diffTileMouseX = (int)mousePos.x % (int) (this->_level.getTileSize().x *
                                                          std::stof(
                                                                  detail::utils::getConfigValue(
                                                                          "tile_scale_x")));

            auto diffTileMouseY = (int)mousePos.y % (int) (this->_level.getTileSize().y *
                                                           std::stof(
                                                                   detail::utils::getConfigValue(
                                                                           "tile_scale_y")));

            auto sizeBoxSelected = (this->_level.getTileSize().x *
                                    std::stof(
                                            detail::utils::getConfigValue(
                                                    "tile_scale_x"))) / 10;

            if (( std::abs(diffTileMouseX) <= sizeBoxSelected ) &&
                    (std::abs(diffTileMouseY) <= sizeBoxSelected)) {


                sf::RectangleShape rectangleBox;
                rectangleBox.setSize(sf::Vector2f(sizeBoxSelected * 2,
                                                  sizeBoxSelected * 2));
                rectangleBox.setOutlineColor(sf::Color::Blue);
                rectangleBox.setOutlineThickness(2);


                rectangleBox.setPosition(
                        std::floor(mousePos.x - ((int) mousePos.x % (int) (this->_level.getTileSize().x *
                                                                           std::stof(
                                                                                   detail::utils::getConfigValue(
                                                                                           "tile_scale_x")))) - sizeBoxSelected),
                        std::floor(mousePos.y - ((int) mousePos.y % (int) (this->_level.getTileSize().y *
                                                                           std::stof(
                                                                                   detail::utils::getConfigValue(
                                                                                           "tile_scale_y"))))) - sizeBoxSelected);
                rectangleBox.setFillColor(sf::Color::Transparent);
                this->_window->draw(rectangleBox);
            }
        }

        static bool newMapBoxVisible = true;
        static bool configureBoxVisible = false;

        static std::regex macRegex("^[a-fA-F0-9:]{17}|[a-fA-F0-9]{12}$");


        if (configureBoxVisible) {
            this->_currentWindowType = detail::WindowTypes::ConfigWindow;
            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(360, 160));
            static std::string configureErrorText;
            ImGui::Begin("Configure", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Text("MAC address: %d", data->number);
            static char macAddress[18] = "";
            ImGui::PushItemWidth(300);
            ImGui::InputText("", macAddress, 18);
            ImGui::PopItemWidth();
            ImGui::Separator();

            ImGui::PushItemWidth(100);

            ImGui::PushID("Chanel");
            ImGui::Text("Chanel");
            static int chanel = 0;
            ImGui::InputInt("ch", &chanel, 1, 0);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PopItemWidth();

            if (ImGui::Button("Create")) {
                if (!std::regex_match(macAddress, macRegex)) {
                    configureErrorText = "Invalid MAC address!";
                }
                else if (chanel <= 0) {
                    configureErrorText = "Chanel must be greater than 0!";
                }else {
                    this->_currentWindowType = detail::WindowTypes::None;
                    configureBoxVisible = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                this->_currentWindowType = detail::WindowTypes::None;
                configureBoxVisible = false;
            }
            ImGui::Text("%s", configureErrorText.c_str());
            ImGui::End();

        }

        //New map box
        if (newMapBoxVisible) {
            this->_currentWindowType = detail::WindowTypes::NewMapWindow;
            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(460, 185));
            static std::string newMapErrorText;
            ImGui::Begin("New map properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Text("Size world");
            static int mapSizeX = 10;
            static int mapSizeY = 10;
            ImGui::InputInt("x", &mapSizeX, 1, 0);
            ImGui::InputInt("y", &mapSizeY, 1, 0);
            ImGui::Separator();

            ImGui::PushItemWidth(100);

            ImGui::PushID("NewMapTileSize");
            ImGui::Text("Tile size");
            static int mapTileSize = 10;
            ImGui::InputInt("sm", &mapTileSize, 1, 0);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PopItemWidth();
            if (ImGui::Button("Create")) {
                if (mapSizeX < 1 || mapSizeY < 1) {
                    newMapErrorText = "The map's height and width must be at least 1!";
                } else if (mapTileSize < 1) {
                    newMapErrorText = "The map's tile height and tile width must be at least 1!";
                } else if (mapSizeX * mapTileSize > 255 || mapSizeY * mapTileSize > 255) {
                    newMapErrorText = "Maximum card size exceeded (255 sm)!";
                }
                else {
                    this->_level.createMap(sf::Vector2i(mapSizeX, mapSizeY),
                                           sf::Vector2i(mapTileSize, mapTileSize));
                    createGridLines();
                    newMapErrorText = "";
                    this->_currentWindowType = detail::WindowTypes::None;
                    newMapBoxVisible = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                newMapErrorText = "";
                this->_currentWindowType = detail::WindowTypes::None;
                newMapBoxVisible = false;
            }
            ImGui::Text("%s", newMapErrorText.c_str());
            ImGui::End();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {

                if (ImGui::MenuItem("Configure")) {
                    configureBoxVisible = true;
                }
                if (ImGui::MenuItem("Exit")) {

                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Control")) {
                if (ImGui::MenuItem("Play")) {
                    if (auto & c = _data->callbacks[BUTTON_INIT]) c();
                }
                if (ImGui::MenuItem("Stop")) {
                    if (auto & c = _data->callbacks[BUTTON_CLOSE]) c();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Map")) {
                if (ImGui::MenuItem("New map")) {
                    newMapBoxVisible = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::Render();

        //Draw the status bar
        sf::RectangleShape rectangle;

        rectangle.setSize(
                sf::Vector2f(this->_window->getSize().x, 30) / (this->_graphics->getZoomPercentage() / 100.0f));
        rectangle.setFillColor(sf::Color::Black);

        sf::Vector2f cameraOffset = (this->_graphics->getView().getCenter() -
                                     (this->_graphics->getView().getSize() / 2.0f));

        rectangle.setPosition(0 + cameraOffset.x,
                              (this->_window->getSize().y / (this->_graphics->getZoomPercentage() / 100.0f)) -
                              (30 / (this->_graphics->getZoomPercentage() / 100.0f)) +
                              cameraOffset.y);

        this->_window->draw(rectangle);
    }

    void Editor::update(sf::Time t) {
        ImGui::SFML::Update(t);
        //Updating internal classes
        this->_level.update(t.asSeconds());
        this->_graphics->update(t.asSeconds(), sf::Vector2f(this->_level.getTileSize()), (this->_windowHasFocus));
    }

    void Editor::exit() {

    }

    void Editor::processEvent(sf::Event &event) {
        ImGui::SFML::ProcessEvent(event);
        this->_currentEvent = event;
        switch (event.type) {
            case sf::Event::GainedFocus:
                this->_windowHasFocus = true;
                break;
            case sf::Event::LostFocus:
                this->_windowHasFocus = false;
                break;
            case sf::Event::MouseWheelScrolled:
                    if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                        this->_graphics.get()->zoom(event.mouseWheelScroll.delta,
                                                    {event.mouseWheelScroll.x, event.mouseWheelScroll.y});
                    }
                break;
            case sf::Event::Closed:
                if (auto & c = _data->callbacks[WINDOW_CLOSE]) c();
                break;
            default:
                break;
        }
    }

    void Editor::createGridLines() {
        this->_gridLines.clear();
        std::array<sf::Vertex, 2> line;

        //Horizontal lines
        for (int i = 0; i < this->_level.getSize().y + 1; ++i) {
            line = {
                    sf::Vertex(sf::Vector2f(0, i * (this->_level.getTileSize().y * std::stof(
                            detail::utils::getConfigValue("tile_scale_y"))))),
                    sf::Vertex(sf::Vector2f(this->_level.getSize().x * this->_level.getTileSize().x *
                                            std::stof(detail::utils::getConfigValue("tile_scale_x")),
                                            i * (this->_level.getTileSize().y *
                                                 std::stof(detail::utils::getConfigValue("tile_scale_y")))))
            };
            this->_gridLines.push_back(line);
        }

        //Vertical lines
        for (int i = 0; i < this->_level.getSize().x + 1; ++i) {
            line = {
                    sf::Vertex(sf::Vector2f(i * (this->_level.getTileSize().x * std::stof(
                            detail::utils::getConfigValue("tile_scale_x"))), 0)),
                    sf::Vertex(sf::Vector2f(i * this->_level.getTileSize().x *
                                            std::stof(detail::utils::getConfigValue("tile_scale_x")),
                                            this->_level.getSize().y * (this->_level.getTileSize().y *
                                                                        std::stof(detail::utils::getConfigValue(
                                                                                "tile_scale_y")))))
            };
            this->_gridLines.push_back(line);
        }
    }

    void Editor::setStateData(std::weak_ptr<StateData> stateData) {
        _data->stateData = stateData;
    }

    std::weak_ptr<StateInput> Editor::getStateInput() const {
        return _data->stateInput;
    }

    void Editor::setEventCallback(Editor::Event event, std::function<void()> &&callback) {
        _data->callbacks[event] = std::move(callback);
    }

    Editor::~Editor() {}

}




