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


        sf::Vector2f mousePos = getMousePos();


        if (mousePos.x >= 0 && (mousePos.x <= (this->_level.getSize().x * this->_level.getTileSize().x *
                                              std::stof(
                                                      detail::utils::getConfigValue("tile_scale_x")))) &&
            mousePos.y >= 0 && mousePos.y <= (this->_level.getSize().y * this->_level.getTileSize().y *
                                              std::stof(
                                                      detail::utils::getConfigValue("tile_scale_y")))) {

            auto diffTileMouseX = (int)mousePos.x % (int) (this->_level.getTileSize().x *
                                                          std::stof(
                                                                  detail::utils::getConfigValue(
                                                                          "tile_scale_x")));

            auto diffTileMouseY = (int)mousePos.y % (int) (this->_level.getTileSize().y *
                                                           std::stof(
                                                                   detail::utils::getConfigValue(
                                                                           "tile_scale_y")));

            if (( diffTileMouseX <= 5 ) &&
                    (diffTileMouseY <= 5)) {


                sf::RectangleShape rectangleBox;
                rectangleBox.setSize(sf::Vector2f(10,
                                               10));
                rectangleBox.setOutlineColor(sf::Color::Blue);
                rectangleBox.setOutlineThickness(2);
                rectangleBox.setPosition(
                        std::floor(mousePos.x - ((int) mousePos.x % (int) (this->_level.getTileSize().x *
                                                                           std::stof(
                                                                                   detail::utils::getConfigValue(
                                                                                           "tile_scale_x")))) - 5),
                        std::floor(mousePos.y - ((int) mousePos.y % (int) (this->_level.getTileSize().y *
                                                                           std::stof(
                                                                                   detail::utils::getConfigValue(
                                                                                           "tile_scale_y"))))) - 5);
                rectangleBox.setFillColor(sf::Color::Transparent);
                this->_window->draw(rectangleBox);
            }
        }

        static bool newMapBoxVisible = true;
        static std::regex macRegex("^[a-fA-F0-9:]{17}|[a-fA-F0-9]{12}$");

        //New map box
        if (newMapBoxVisible) {
            this->_currentWindowType = detail::WindowTypes::NewMapWindow;
            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(460, 320));
            static std::string newMapErrorText = "";
            ImGui::Begin("New map properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Text("MAC address: %d", data->number);
            static char macAddress[18] = "";
            ImGui::PushItemWidth(300);
            ImGui::InputText("", macAddress, 18);
            ImGui::PopItemWidth();
            ImGui::Separator();

            ImGui::PushItemWidth(100);

            ImGui::PushID("RadiusWheel");
            ImGui::Text("Radius wheel");
            ImGui::PushItemWidth(300);
            static float radiusWheel = 0;
            ImGui::InputFloat("mm", &radiusWheel);
            ImGui::PopItemWidth();
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PushID("DistanceWheel");
            ImGui::Text("Distance wheel");
            ImGui::PushItemWidth(300);
            static float distanceWheel = 0;
            ImGui::InputFloat("mm", &distanceWheel);
            ImGui::PopItemWidth();
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PushID("NewMapSize");
            ImGui::Text("Size world");
            static int mapSizeX = 0;
            static int mapSizeY = 0;
            ImGui::InputInt("x", &mapSizeX, 1, 0);
            ImGui::InputInt("y", &mapSizeY, 1, 0);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PushID("NewMapTileSize");
            ImGui::Text("Tile size");
            static int mapTileSizeX = 8;
            static int mapTileSizeY = 8;
            ImGui::InputInt("x", &mapTileSizeX, 1, 0);
            ImGui::InputInt("y", &mapTileSizeY, 1, 0);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PopItemWidth();
            if (ImGui::Button("Create")) {
                if (std::regex_match(macAddress, macRegex)) {
                    newMapErrorText = "Invalid MAC address!";
                }
                else if (radiusWheel <= 0) {
                    newMapErrorText = "Wheel radius must be greater than 0!";
                }
                else if (mapSizeX < 1 || mapSizeY < 1) {
                    newMapErrorText = "The map's height and width must be at least 1!";
                } else if (mapTileSizeX < 1 || mapTileSizeY < 1) {
                    newMapErrorText = "The map's tile height and tile width must be at least 1!";
                } else {
                    this->_level.createMap(sf::Vector2i(mapSizeX, mapSizeY),
                                           sf::Vector2i(mapTileSizeX, mapTileSizeY));
                    createGridLines();
                    mapSizeX = 0;
                    mapSizeY = 0;
                    mapTileSizeX = 8;
                    mapTileSizeY = 8;
                    newMapErrorText = "";
                    this->_currentWindowType = detail::WindowTypes::None;
                    newMapBoxVisible = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                mapSizeX = 0;
                mapSizeY = 0;
                mapTileSizeX = 8;
                mapTileSizeY = 8;
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
                    if (auto & c = _data->callbacks[BUTTON_INIT]) c();
                }
                if (ImGui::MenuItem("Exit")) {

                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Control")) {
                if (ImGui::MenuItem("Play")) {

                }
                if (ImGui::MenuItem("Stop")) {

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




