#include <utility>

#include "editor.h"

#include <regex>
#include <iomanip>

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
            _hideShapes(false),
            _graphics(new detail::Graphics(window)),
            _level(this->_graphics),
            _menuClicks(0),
            _attachPoint(false),
            _showEntityList(false),
            _currentDrawShape(detail::DrawShapes::None),
            _currentWindowType(detail::WindowTypes::NewMapWindow),
            _currentMapEditorMode(detail::MapEditorMode::None),
            _data(new Data) {

        ImGui::SFML::Init(*window);
        this->_window = window;
        this->createGridLines();
    }

    void Editor::render() {

        _data->stateDataLocked = _data->stateData.lock();

        const auto &inp = _data->stateInput;
        const auto &data = _data->stateDataLocked;

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

        //Draw shapes
        if (!this->_hideShapes) {
            for (std::shared_ptr<detail::Shape> shape : this->_level.getShapeList()) {
                shape.get()->draw(this->_window);
            }
        }

        auto sizeBoxSelected = (this->_level.getTileSize().x *
                                std::stof(
                                        detail::utils::getConfigValue(
                                                "tile_scale_x"))) / 10;


        //Shape creation
        if (!this->_hideShapes) {
            //Points
            if (this->_currentDrawShape == detail::DrawShapes::Point) {
                static sf::Vector2f mousePos = getMousePos();
                static sf::CircleShape dot;
                static const float DOT_RADIUS = 6.0f;
                if (this->_currentEvent.type == sf::Event::MouseButtonReleased &&
                    this->_currentEvent.mouseButton.button == sf::Mouse::Left) {
                    if (++this->_menuClicks > 1) {
                        mousePos = getMousePos();
                        mousePos.x = std::max(0.0f, mousePos.x);
                        mousePos.x = std::min(this->_level.getSize().x *
                                              std::stof(detail::utils::getConfigValue("tile_scale_x")) *
                                              this->_level.getTileSize().x, mousePos.x);
                        mousePos.y = std::max(0.0f, mousePos.y);
                        mousePos.y = std::min(this->_level.getSize().y *
                                              std::stof(detail::utils::getConfigValue("tile_scale_y")) *
                                              this->_level.getTileSize().y, mousePos.y);
                        dot.setRadius(DOT_RADIUS);
                        dot.setPosition(
                                std::floor(mousePos.x - ((int) mousePos.x % (int) (this->_level.getTileSize().x *
                                                                                   std::stof(
                                                                                           detail::utils::getConfigValue(
                                                                                                   "tile_scale_x")))) -
                                           DOT_RADIUS),
                                std::floor(mousePos.y - ((int) mousePos.y % (int) (this->_level.getTileSize().y *
                                                                                   std::stof(
                                                                                           detail::utils::getConfigValue(
                                                                                                   "tile_scale_y"))))) -
                                DOT_RADIUS);
                        dot.setFillColor(sf::Color(0, 0, 255, 80));
                        dot.setOutlineColor(sf::Color(0, 0, 255, 160));
                        dot.setOutlineThickness(2.0f);
                        this->_level.addShape(std::make_shared<detail::Point>("Point", sf::Color::Blue, dot));
                        this->_currentEvent = sf::Event();
                        this->_currentDrawShape = detail::DrawShapes::None;
                        this->_menuClicks = 0;
                    } else {
                        this->_currentEvent = sf::Event();
                    }
                }
            }

            //Lines
            if (this->_currentDrawShape == detail::DrawShapes::Line &&
                this->_currentMapEditorMode == detail::MapEditorMode::Object) {
                static sf::Vector2f mousePos = getMousePos();
                static std::vector<std::shared_ptr<detail::Point>> points;

                //Draw temporary points / line
                for (auto &p : points) {
                    p->draw(this->_window);
                }
                //Draw connecting lines between the points
                if (points.size() >= 2) {
                    for (unsigned int i = 0; i < points.size() - 1; ++i) {
                        sf::Vertex v[4];
                        sf::Vector2f point1 = sf::Vector2f(
                                points[i]->getCircle().getPosition().x + points[i]->getCircle().getRadius(),
                                points[i]->getCircle().getPosition().y + points[i]->getCircle().getRadius());
                        sf::Vector2f point2 = sf::Vector2f(
                                points[i + 1]->getCircle().getPosition().x + points[i + 1]->getCircle().getRadius(),
                                points[i + 1]->getCircle().getPosition().y + points[i + 1]->getCircle().getRadius());

                        sf::Vector2f direction = point2 - point1;

                        sf::Vector2f unitDirection =
                                direction / std::sqrt(direction.x * direction.x + direction.y * direction.y);
                        sf::Vector2f unitPerpendicular(-unitDirection.y, unitDirection.x);

                        sf::Vector2f offset = (3.0f / 2.0f) * unitPerpendicular;

                        v[0].position = point1 + offset;
                        v[1].position = point2 + offset;
                        v[2].position = point2 - offset;
                        v[3].position = point1 - offset;

                        this->_graphics->draw(v, 4, sf::Quads);
                    }
                }
                static const float DOT_RADIUS = 6.0f;
                if (this->_currentEvent.type == sf::Event::MouseButtonReleased) {
                    if (this->_currentEvent.mouseButton.button == sf::Mouse::Left) {
                        if (++this->_menuClicks > 1) {
                            mousePos = getMousePos();
                            mousePos.x = std::max(0.0f, mousePos.x);
                            mousePos.x = std::min(this->_level.getSize().x *
                                                  std::stof(detail::utils::getConfigValue("tile_scale_x")) *
                                                  this->_level.getTileSize().x, mousePos.x);
                            mousePos.y = std::max(0.0f, mousePos.y);
                            mousePos.y = std::min(this->_level.getSize().y *
                                                  std::stof(detail::utils::getConfigValue("tile_scale_y")) *
                                                  this->_level.getTileSize().y, mousePos.y);


                            auto positionX = std::floor(
                                    mousePos.x - ((int) mousePos.x % (int) (this->_level.getTileSize().x *
                                                                            std::stof(
                                                                                    detail::utils::getConfigValue(
                                                                                            "tile_scale_x")))) -
                                    DOT_RADIUS);


                            auto positionY = std::floor(
                                    mousePos.y - ((int) mousePos.y % (int) (this->_level.getTileSize().y *
                                                                            std::stof(
                                                                                    detail::utils::getConfigValue(
                                                                                            "tile_scale_y")))) -
                                    DOT_RADIUS);

                            auto it = std::find_if(points.begin(), points.end(),
                                                   [positionX, positionY](std::shared_ptr<detail::Point> &point) {
                                                       return point->getCircle().getPosition() ==
                                                              sf::Vector2f(positionX, positionY);
                                                   });


                            if (this->_attachPoint || it == points.end()) {


                                if (!points.empty()) {
                                    auto positionLastPoint = points.back()->getCircle().getPosition();

                                    if (positionLastPoint.x == positionX || positionLastPoint.y == positionY) {

                                        sf::CircleShape c;
                                        c.setRadius(DOT_RADIUS);
                                        c.setPosition(positionX, positionY);
                                        c.setFillColor(sf::Color(0, 180, 0, 80));
                                        c.setOutlineColor(sf::Color(0, 180, 0, 160));
                                        c.setOutlineThickness(2.0f);
                                        points.push_back(
                                                std::make_shared<detail::Point>(
                                                        "p" + std::to_string((points.size() + 1)),
                                                        sf::Color(0, 255, 0), c));
                                        this->_currentEvent = sf::Event();
                                    }
                                } else {
                                    sf::CircleShape c;
                                    c.setRadius(DOT_RADIUS);
                                    c.setPosition(positionX, positionY);
                                    c.setFillColor(sf::Color(0, 180, 0, 80));
                                    c.setOutlineColor(sf::Color(0, 180, 0, 160));
                                    c.setOutlineThickness(2.0f);
                                    points.push_back(
                                            std::make_shared<detail::Point>("p" + std::to_string((points.size() + 1)),
                                                                            sf::Color(0, 255, 0), c));
                                    this->_currentEvent = sf::Event();

                                }
                            }
                        } else {
                            this->_currentEvent = sf::Event();
                        }
                    }
                    if (this->_currentEvent.mouseButton.button == sf::Mouse::Right) {
                        if (points.size() >= 2) {
                            //Save the line!
                            this->_level.addShape(std::make_shared<detail::Line>("Line", sf::Color::White, points));
                        }
                        points.clear();
                        this->_currentEvent = sf::Event();
                        this->_currentDrawShape = detail::DrawShapes::None;
                        this->_menuClicks = 0;
                    }
                }
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

            auto diffTileMouseX = (int) mousePos.x % (int) (this->_level.getTileSize().x *
                                                            std::stof(
                                                                    detail::utils::getConfigValue(
                                                                            "tile_scale_x")));

            auto diffTileMouseY = (int) mousePos.y % (int) (this->_level.getTileSize().y *
                                                            std::stof(
                                                                    detail::utils::getConfigValue(
                                                                            "tile_scale_y")));

            if ((std::abs(diffTileMouseX) <= sizeBoxSelected) &&
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
                                                                                           "tile_scale_x")))) -
                                   sizeBoxSelected),
                        std::floor(mousePos.y - ((int) mousePos.y % (int) (this->_level.getTileSize().y *
                                                                           std::stof(
                                                                                   detail::utils::getConfigValue(
                                                                                           "tile_scale_y"))))) -
                        sizeBoxSelected);
                rectangleBox.setFillColor(sf::Color::Transparent);
                this->_window->draw(rectangleBox);
            }
        }


        // UI

        static bool newMapBoxVisible = false;
        static bool configureBoxVisible = true;
        static bool playBoxVisible = false;
        static bool cbShowEntityList = false;
        static bool cbHideShapes = false;
        static bool cbAttachShapes = false;
        static bool entityPropertiesLoaded = false;
        static bool showEntityProperties = false;


        cbShowEntityList = this->_showEntityList;

        //Point
        static std::shared_ptr<detail::Point> selectedEntityPoint;
        static std::shared_ptr<detail::Point> originalSelectedEntityPoint;

        //Line
        static std::shared_ptr<detail::Line> selectedEntityLine;
        static std::shared_ptr<detail::Line> originalSelectedEntityLine;


        // Status bar
        static std::string currentStatus = "";
        static bool showCurrentStatus = false;
        static int currentStatusTimer = 0;

        static auto startStatusTimer = [&](std::string newStatus, int time) {
            currentStatus = newStatus;
            currentStatusTimer = time;
            showCurrentStatus = true;
        };

        static std::regex macRegex("^[a-fA-F0-9:]{17}|[a-fA-F0-9]{12}$");


        //Draw active point color custom
        /* if (selectedEntityLine) {
            auto points = selectedEntityLine->getPoints();
            if (data->statusControl == StatusControl::Play) {
                points.at(data->positionActive)->setColor(sf::Color::Red);
                if (data->positionActive > 0) {
                    points.at(data->positionActive - 1)->setColor(sf::Color(0, 255, 0));
                }
            } else if (data->statusControl == StatusControl::Stop) {
                if (points.size() >= data->positionActive+1) {
                    points.at(data->positionActive)->setColor(sf::Color(0, 255, 0));
                }
            }
        } */

        // Show messages core
        if (!data->message.empty()) {
            startStatusTimer(data->message, 200);
            data->message.clear();
        }

        if (configureBoxVisible) {
            this->_currentWindowType = detail::WindowTypes::ConfigWindow;
            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(360, 160));
            static std::string configureErrorText;
            ImGui::Begin("Configure", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Text("MAC address");
            ImGui::PushItemWidth(300);
            ImGui::InputText("", inp->macAddress, 18);
            ImGui::PopItemWidth();
            ImGui::Separator();

            ImGui::PushItemWidth(100);

            ImGui::PushID("Chanel");
            ImGui::Text("Chanel");
            ImGui::InputInt("ch", &inp->chanel, 1, 0);
            ImGui::Separator();
            ImGui::PopID();

            ImGui::PopItemWidth();

            if (ImGui::Button("Create")) {
                if (!std::regex_match(inp->macAddress, macRegex)) {
                    configureErrorText = "Invalid MAC address!";
                } else if (inp->chanel <= 0) {
                    configureErrorText = "Chanel must be greater than 0!";
                } else {
                    if (auto &c = _data->callbacks[BUTTON_CONNECT]) c();
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

        //Clear all of the selected entity objects
        static auto clearSelectedEntityObjects = [&]() {
            selectedEntityPoint = nullptr;
            originalSelectedEntityPoint = nullptr;
            selectedEntityLine = nullptr;
            originalSelectedEntityLine = nullptr;
        };

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
                } else if (mapTileSize >= 255) {
                    newMapErrorText = "The map's tile height and tile width must be at exceeded 255!";
                } else if (mapSizeX >= 255 || mapSizeY >= 255) {
                    newMapErrorText = "Maximum map size exceeded (255 sm)!";
                } else {
                    clearSelectedEntityObjects();
                    this->_level.createMap(sf::Vector2i(mapSizeX, mapSizeY),
                                           sf::Vector2i(mapTileSize, mapTileSize));
                    createGridLines();
                    newMapErrorText = "";
                    this->_currentWindowType = detail::WindowTypes::None;
                    this->_currentMapEditorMode = detail::MapEditorMode::Object;
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


        if (playBoxVisible) {
            this->_currentWindowType = detail::WindowTypes::ControlPlayWindow;
            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(280, 100));
            static std::string newMapErrorText;
            ImGui::Begin("Control properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);


            auto fillLineSection = [&]() -> void {
                int c = 0;
                for (std::shared_ptr<detail::Shape> shape : this->_level.getShapeList()) {
                    ++c;
                    //Line check
                    auto l = std::dynamic_pointer_cast<detail::Line>(shape);
                    if (l != nullptr) {
                        std::string strId = "Line" + std::to_string(c);
                        ImGui::PushID(strId.c_str());
                        bool isSelected = (selectedEntityLine == l);
                        shape->unselect();
                        if (ImGui::Selectable(l->getName().c_str(), isSelected)) {
                            selectedEntityLine = l;
                            shape->select();
                            originalSelectedEntityLine = std::make_shared<detail::Line>(l->getName(), l->getColor(),
                                                                                        l->getPoints());

                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                        ImGui::PopID();
                        continue;
                    }
                }
            };

            if (ImGui::BeginCombo("Path", selectedEntityLine ? selectedEntityLine->getName().c_str() : "Select path")) {
                fillLineSection();
                ImGui::EndCombo();
            }
            ImGui::Separator();
            ImGui::PushItemWidth(100);


            if (ImGui::Button("Play")) {
                if (selectedEntityLine == nullptr) {
                    newMapErrorText = "Select path!";
                } else {
                    inp->commands.clear();
                    const auto &points = selectedEntityLine->getPoints();
                    for (unsigned int i = 0; i < points.size() - 1; ++i) {

                        sf::Vector2f point1 = sf::Vector2f(
                                points[i]->getCircle().getPosition().x + points[i]->getCircle().getRadius(),
                                points[i]->getCircle().getPosition().y + points[i]->getCircle().getRadius());
                        sf::Vector2f point2 = sf::Vector2f(
                                points[i + 1]->getCircle().getPosition().x + points[i + 1]->getCircle().getRadius(),
                                points[i + 1]->getCircle().getPosition().y + points[i + 1]->getCircle().getRadius());

                        sf::Vector2f direction = point2 - point1;

                        sf::Vector2f unitDirection =
                                direction / std::sqrt(direction.x * direction.x + direction.y * direction.y);

                        sf::Vector2f unitPerpendicular(unitDirection.x, -unitDirection.y);

                        // движение
                        Command upCommand{};
                        upCommand.size = this->_level.getTileSize().x;
                        upCommand.direction = Direction::Up;
                        upCommand.view = Direction::Up;


                        // поворот робота
                        Command directCommand{};
                        directCommand.size = this->_level.getTileSize().x;
                        directCommand.length = static_cast<int>(std::abs(direction.y)) / std::stof(
                                detail::utils::getConfigValue(
                                        "tile_scale_x")) / this->_level.getTileSize().x;

                        if (unitPerpendicular.y > 0) {
                            // поворот на верх
                            directCommand.view = Direction::Up;
                            upCommand.view = Direction::Up;

                            upCommand.length = static_cast<int>(std::abs(direction.y)) / std::stof(
                                    detail::utils::getConfigValue(
                                            "tile_scale_x")) / this->_level.getTileSize().x;

                            if (!inp->commands.empty()) {
                                const auto &lastDirectCommand = inp->commands.back();

                                if ((lastDirectCommand.view == Direction::Right)) {
                                    // Поворот налево, в случае если смотрит на право
                                    directCommand.direction = Direction::Left;
                                    inp->commands.emplace_back(directCommand);
                                } else if ((lastDirectCommand.view == Direction::Left)) {
                                    // Поворот направо, в случае если смотрит на лево
                                    directCommand.direction = Direction::Right;
                                    inp->commands.emplace_back(directCommand);
                                }
                            }

                        } else if (unitPerpendicular.y < 0) {
                            // поворот на вниз
                            directCommand.view = Direction::Down;
                            upCommand.view = Direction::Down;

                            upCommand.length = static_cast<int>(std::abs(direction.y)) / std::stof(
                                    detail::utils::getConfigValue(
                                            "tile_scale_x")) / this->_level.getTileSize().x;


                            if (!inp->commands.empty()) {
                                const auto &lastDirectCommand = inp->commands.back();

                                if ((lastDirectCommand.view == Direction::Right)) {
                                    // Поворот направо, в случае если смотрит на право
                                    directCommand.direction = Direction::Right;
                                    inp->commands.emplace_back(directCommand);
                                } else if ((lastDirectCommand.view == Direction::Left)) {
                                    // Поворот налево, в случае если смотрит на лево
                                    directCommand.direction = Direction::Left;
                                    inp->commands.emplace_back(directCommand);
                                }
                            }

                        } else if (unitDirection.x > 0) {

                            // поворот на право
                            directCommand.view = Direction::Right;
                            upCommand.view = Direction::Right;

                            upCommand.length = static_cast<int>(std::abs(direction.x)) / std::stof(
                                    detail::utils::getConfigValue(
                                            "tile_scale_x")) / this->_level.getTileSize().x;


                            if (!inp->commands.empty()) {
                                const auto &lastDirectCommand = inp->commands.back();

                                if ((lastDirectCommand.view == Direction::Up)) {
                                    // Поворот направо, в случае если смотрит на верх
                                    directCommand.direction = Direction::Right;
                                    inp->commands.emplace_back(directCommand);
                                } else if ((lastDirectCommand.view == Direction::Down)) {
                                    // Поворот налево, в случае если смотрит вниз
                                    directCommand.direction = Direction::Left;
                                    inp->commands.emplace_back(directCommand);
                                }
                            }
                        } else if (unitDirection.x < 0) {

                            // поворот на право
                            directCommand.view = Direction::Left;
                            upCommand.view = Direction::Left;

                            upCommand.length = static_cast<int>(std::abs(direction.x)) / std::stof(
                                    detail::utils::getConfigValue(
                                            "tile_scale_x")) / this->_level.getTileSize().x;


                            if (!inp->commands.empty()) {
                                const auto &lastDirectCommand = inp->commands.back();

                                if ((lastDirectCommand.view == Direction::Up)) {
                                    // Поворот налево, в случае если смотрит на верх
                                    directCommand.direction = Direction::Left;
                                    inp->commands.emplace_back(directCommand);
                                } else if ((lastDirectCommand.view == Direction::Down)) {
                                    // Поворот направо, в случае если смотрит вниз
                                    directCommand.direction = Direction::Right;
                                    inp->commands.emplace_back(directCommand);
                                }
                            }
                        }
                        inp->commands.emplace_back(upCommand);
                    }

                    this->_currentWindowType = detail::WindowTypes::None;
                    if (auto &c = _data->callbacks[BUTTON_PLAY]) c();
                    playBoxVisible = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                newMapErrorText = "";
                this->_currentWindowType = detail::WindowTypes::None;
                playBoxVisible = false;
            }
            ImGui::Text("%s", newMapErrorText.c_str());
            ImGui::End();
        }


        /*
         * Entity list
         */
        if (!cbShowEntityList && this->_currentWindowType == detail::WindowTypes::EntityListWindow) {
            this->_currentWindowType = detail::WindowTypes::None;
        }
        if (cbShowEntityList && this->_currentMapEditorMode == detail::MapEditorMode::Object) {
            this->_currentWindowType = detail::WindowTypes::EntityListWindow;

            //FillPointSection function
            auto fillPointSection = [&]() -> void {
                int c = 0;
                for (std::shared_ptr<detail::Shape> shape : this->_level.getShapeList()) {
                    ++c;
                    //Point check
                    auto p = std::dynamic_pointer_cast<detail::Point>(shape);
                    if (p != nullptr) {
                        std::string strId = "Point" + std::to_string(c);
                        ImGui::PushID(strId.c_str());
                        if (ImGui::Selectable(p->getName().c_str())) {
                            entityPropertiesLoaded = false;
                            shape->select();
                            selectedEntityPoint = p;
                            originalSelectedEntityPoint = std::make_shared<detail::Point>(p->getName(), p->getColor(),
                                                                                          p->getCircle());

                            selectedEntityLine = nullptr;
                            originalSelectedEntityLine = nullptr;

                            //Show the properties window
                            showEntityProperties = true;
                        }
                        ImGui::PopID();
                        continue;
                    }
                }
            };
            //FillLineSection function
            auto fillLineSection = [&]() -> void {
                int c = 0;
                for (std::shared_ptr<detail::Shape> shape : this->_level.getShapeList()) {
                    ++c;
                    //Line check
                    auto l = std::dynamic_pointer_cast<detail::Line>(shape);
                    if (l != nullptr) {
                        std::string strId = "Line" + std::to_string(c);
                        ImGui::PushID(strId.c_str());
                        if (ImGui::Selectable(l->getName().c_str())) {
                            entityPropertiesLoaded = false;
                            shape->select();
                            selectedEntityLine = l;
                            originalSelectedEntityLine = std::make_shared<detail::Line>(l->getName(), l->getColor(),
                                                                                        l->getPoints());

                            selectedEntityPoint = nullptr;
                            originalSelectedEntityPoint = nullptr;

                            //Show the properties window
                            showEntityProperties = true;
                        }
                        ImGui::PopID();
                        continue;
                    }
                }
            };

            ImGui::SetNextWindowPosCenter();
            ImGui::SetNextWindowSize(ImVec2(500, 300));
            ImGui::Begin("Entity list", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);
            if (ImGui::TreeNode("Entities")) {
                if (ImGui::TreeNode("Objects")) {
                    if (ImGui::TreeNode("Lines")) {
                        fillLineSection();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Points")) {
                        fillPointSection();
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
            ImGui::End();
        };

        if (showEntityProperties) {

            ImGui::SetNextWindowSize(ImVec2(511, 234));
            this->_currentWindowType = detail::WindowTypes::EntityPropertiesWindow;
            ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
                                                ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushID("SelectedEntityName");
            static char name[500] = "";
            if (!entityPropertiesLoaded) {
                strcpy(name, selectedEntityPoint != nullptr ? selectedEntityPoint->getName().c_str() :
                             selectedEntityLine != nullptr ? selectedEntityLine->getName().c_str() :
                             "");
            }
            ImGui::PushItemWidth(200);
            ImGui::InputText("Name", name, sizeof(name));
            ImGui::PopItemWidth();
            ImGui::PopID();

            ImGui::Separator();


            //List all of the points (with an option to delete)
            if (selectedEntityLine != nullptr) {
                ImGui::Text("Points");
                for (auto &p : selectedEntityLine->getPoints()) {
                    ImGui::Text(p->getName().c_str());
                    if (selectedEntityLine->getPoints().size() > 2) {
                        ImGui::SameLine();
                        ImGui::PushID(("btn_" + p->getName()).c_str());
                        if (ImGui::Button("x", ImVec2(26, 20))) {
                            selectedEntityLine->deletePoint(p);
                        }
                        ImGui::PopID();
                    }
                }
            }

            if (ImGui::Button("Update")) {
                if (selectedEntityPoint != nullptr) {
                    selectedEntityPoint->setName(name);
                    selectedEntityPoint->unselect();
                    this->_level.updateShape(originalSelectedEntityPoint, selectedEntityPoint);
                    startStatusTimer("Point saved successfully!", 200);
                } else if (selectedEntityLine != nullptr) {
                    selectedEntityLine->setName(name);
                    selectedEntityLine->unselect();
                    this->_level.updateShape(originalSelectedEntityLine, selectedEntityLine);
                    startStatusTimer("Line saved successfully!", 200);
                }
                this->_currentWindowType = detail::WindowTypes::None;
                showEntityProperties = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                if (selectedEntityPoint != nullptr) {
                    selectedEntityPoint->unselect();
                } else if (selectedEntityLine != nullptr) {
                    selectedEntityLine->unselect();
                }
                this->_currentWindowType = detail::WindowTypes::None;
                showEntityProperties = false;
            }
            ImGui::SameLine();
            ImGui::Text("    ");
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                if (selectedEntityPoint != nullptr) {
                    this->_level.removeShape(selectedEntityPoint);
                } else if (selectedEntityLine != nullptr) {
                    this->_level.removeShape(selectedEntityLine);
                }
                clearSelectedEntityObjects();
                this->_currentWindowType = detail::WindowTypes::None;
                showEntityProperties = false;
            }
            entityPropertiesLoaded = true;
            ImGui::End();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Configure")) {

                if (ImGui::MenuItem("Connect", nullptr, false, data->statusConnection == StatusConnection::Closed)) {
                    configureBoxVisible = true;
                }
                if (ImGui::MenuItem("Disconnect", nullptr, false,
                                    data->statusConnection == StatusConnection::Connected)) {
                    if (auto &c = _data->callbacks[BUTTON_DISCONNECT]) c();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Control", data->statusConnection == StatusConnection::Connected)) {
                if (ImGui::MenuItem("Play")) {
                    playBoxVisible = true;
                }
                if (ImGui::MenuItem("Stop")) {
                    if (auto &c = _data->callbacks[BUTTON_STOP]) c();
                }
                ImGui::EndMenu();
            }


            if (ImGui::BeginMenu("Map", data->statusControl == StatusControl::Stop && this->_menuClicks <= 1)) {
                if (ImGui::MenuItem("New map")) {
                    newMapBoxVisible = true;
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("Add", this->_currentMapEditorMode == detail::MapEditorMode::Object)) {
                    if (ImGui::BeginMenu("Object", this->_currentMapEditorMode == detail::MapEditorMode::Object)) {
                        if (ImGui::MenuItem("Path") && this->_currentMapEditorMode == detail::MapEditorMode::Object) {
                            this->_currentDrawShape = detail::DrawShapes::Line;
                        }
                        if (ImGui::MenuItem("Point") && this->_currentMapEditorMode == detail::MapEditorMode::Object) {
                            this->_currentDrawShape = detail::DrawShapes::Point;
                        }
                        ImGui::EndMenu();

                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::Checkbox("Show entity list   E", &cbShowEntityList) &&
                    this->_currentMapEditorMode == detail::MapEditorMode::Object &&
                    data->statusControl == StatusControl::Stop) {
                    this->_showEntityList = cbShowEntityList;
                }
                if (ImGui::Checkbox("Hide shapes", &cbHideShapes)) {
                    this->_hideShapes = cbHideShapes;
                }
                if (ImGui::Checkbox("Attach shapes", &cbAttachShapes)) {
                    this->_attachPoint = cbAttachShapes;
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
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

        //Status bar
        ImGui::Begin("Background", nullptr, ImGui::GetIO().DisplaySize, 0.0f,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
        //Map zoom percentage
        if (this->_currentMapEditorMode == detail::MapEditorMode::Object) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << this->_graphics.get()->getZoomPercentage() << "%";
            ImGui::GetWindowDrawList()->AddText(
                    ImVec2(this->_window->getSize().x - 80, this->_window->getSize().y - 22),
                    ImColor(1.0f, 1.0f, 1.0f, 1.0f), ss.str().c_str());
        }
        if (showCurrentStatus) {
            ImGui::GetWindowDrawList()->AddText(ImVec2(10, this->_window->getSize().y - 22),
                                                ImColor(1.0f, 1.0f, 1.0f, 1.0f), currentStatus.c_str());

        }
        ImGui::End();

        //Status timer
        if (currentStatusTimer > 0) {
            currentStatusTimer -= 1;
        } else {
            showCurrentStatus = false;
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
        this->_currentEvent = event;
        switch (event.type) {
            case sf::Event::GainedFocus:
                this->_windowHasFocus = true;
                break;
            case sf::Event::LostFocus:
                this->_windowHasFocus = false;
                break;
            case sf::Event::KeyReleased:
                switch (event.key.code) {
                    case sf::Keyboard::E:
                        if (this->_currentMapEditorMode == detail::MapEditorMode::Object) {
                            this->_showEntityList = !this->_showEntityList;
                            this->_currentWindowType = this->_showEntityList
                                                       ? detail::WindowTypes::EntityListWindow
                                                       : detail::WindowTypes::None;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case sf::Event::MouseWheelScrolled:
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    this->_graphics.get()->zoom(event.mouseWheelScroll.delta,
                                                {event.mouseWheelScroll.x, event.mouseWheelScroll.y});
                }
                break;
            case sf::Event::Closed:
                if (auto &c = _data->callbacks[WINDOW_CLOSE]) c();
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




