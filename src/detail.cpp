#include "detail.h"
#include <experimental/filesystem>
#include <sstream>
#include <fstream>
#include <tuple>
#include <iostream>
#include <cmath>
#include <memory>
#include <functional>

namespace rb {
    namespace detail {

        Graphics::Graphics(sf::RenderWindow *window) {
            this->_window = window;
            this->_view.reset(sf::FloatRect(-1.0f, -20.0f, this->_window->getSize().x, this->_window->getSize().y));
            this->_zoomPercentage = 100;
        }

        void Graphics::draw(const sf::Vertex *vertices, unsigned int vertexCount, sf::PrimitiveType type,
                            const sf::RenderStates &states) {
            this->_window->setView(this->_view);
            this->_window->draw(vertices, vertexCount, type, states);
        }

        void Graphics::setViewPosition(sf::Vector2f pos) {
            this->_view.reset(sf::FloatRect(pos.x, pos.y, this->_window->getSize().x, this->_window->getSize().y));
        }

        void Graphics::zoom(float n, sf::Vector2i pixel) {

            const sf::Vector2f before {this->_window->mapPixelToCoords(pixel) };
            if (n > 0) {
                this->_view.zoom(1.f / 1.06f);
                this->_zoomPercentage *= 1.06f;
            }
            else if (n < 0) {
                this->_view.zoom(1.06f);
                this->_zoomPercentage /= 1.06f;
            }
            this->_window->setView(this->_view);
            const sf::Vector2f after { this->_window->mapPixelToCoords(pixel) };
            const sf::Vector2f offset { before - after };
            this->_view.move(offset);
            this->_window->setView(this->_view);
        }

        void Graphics::update(float elapsedTime, sf::Vector2f tileSize, bool windowHasFocus) {
            float amountToMoveX = (
                    tileSize.x * std::stof(utils::getConfigValue("tile_scale_x")))
                            / std::stof(utils::getConfigValue("camera_pan_factor"));

            float amountToMoveY = (
                    tileSize.y * std::stof(utils::getConfigValue("tile_scale_y")))
                            / std::stof(utils::getConfigValue("camera_pan_factor"));

            if (windowHasFocus) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                    this->_view.move(0, amountToMoveY);
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
                    this->_view.move(0, -amountToMoveY);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                    this->_view.move(-amountToMoveX, 0);
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                    this->_view.move(amountToMoveX, 0);
                }
                this->_window->setView(this->_view);
            }
        }

        sf::View Graphics::getView() const {
            return this->_view;
        }

        float Graphics::getZoomPercentage() const {
            return this->_zoomPercentage;
        }

        void Graphics::setZoomPercentage(float zoomPercentage) {
            this->_zoomPercentage = zoomPercentage;
        }


        std::vector<std::string> utils::split(std::string str, char c) {
            std::vector<std::string> list;
            std::stringstream ss(str);
            std::string item;
            while (std::getline(ss, item, c)) {
                list.push_back(item);
            }
            return list;
        }

        std::vector<std::string> utils::split(const std::string &str, const std::string &delim, int count) {
            std::vector<std::string> tokens;
            size_t prev = 0, pos = 0;
            do {
                pos = str.find(delim, prev);
                if (pos == std::string::npos) {
                    pos = str.length();
                }
                std::string token = str.substr(prev, pos - prev);
                if (!token.empty()) {
                    tokens.push_back(token);
                }
                prev = pos + delim.length();

                if (count == 1) {
                    tokens.push_back(str.substr(prev, str.length()));
                    return tokens;
                }
                //end horrible hack
            } while (pos < str.length() && prev < str.length());
            return tokens;
        }

        std::vector<std::string>
        utils::splitVector(const std::vector<std::string> &v, const std::string &delim, int index) {
            std::vector<std::string> returnVector;
            for (const auto &i : v) {
                auto t = utils::split(i, delim);
                returnVector.push_back(t[index]);
            }
            return returnVector;
        }

        std::vector<const char *> utils::getFilesInDirectory(std::string directory) {
            std::vector<std::string> mapFiles;
            for (auto &p : std::experimental::filesystem::directory_iterator(directory)) {
                mapFiles.push_back(p.path().string());
            }
            std::vector<const char*> ret(mapFiles.size());
            std::transform(mapFiles.begin(), mapFiles.end(), ret.begin(), std::mem_fun_ref(&std::string::c_str));
            return ret;
        }

        std::string utils::getConfigValue(std::string key) {
            std::ifstream in("rembot.config");
            std::map<std::string, std::string> configMap;
            if (!in.fail()) {
                for (std::string line; std::getline(in, line); ) {
                    configMap.insert(std::pair<std::string, std::string>(utils::split(line, '=')[0], utils::split(line, '=')[1]));
                }
            }
            return configMap.size() <= 0 ? "" : configMap[key];
        }

        sf::Color utils::getColor(const ImVec4 &c) {
            return sf::Color(c.x * 255, c.y * 255, c.z * 255, c.w * 255);
        }

        sf::Color utils::getColor(const std::string &intValue) {
            return sf::Color(static_cast<sf::Uint32>(std::stoll(intValue)));
        }

        sf::Color utils::getColor(const long long &intValue) {
            return getColor(std::to_string(intValue));
        }

        void Level::createMap(sf::Vector2i size, sf::Vector2i tileSize) {
            this->_shapeList.clear();
            this->_size = size;
            this->_tileSize = tileSize;
        }

        Level::Level(std::shared_ptr<Graphics> graphics) {
            this->_graphics = graphics;
        }

        void Level::update(float elapsedTime) {
            (void)elapsedTime;
        }

        sf::Vector2i Level::getSize() const {
            return this->_size;
        }

        void Level::setSize(sf::Vector2i size) {
            this->_size = size;
        }

        void Level::addShape(std::shared_ptr<detail::Shape> shape) {
            this->_shapeList.push_back(shape);
        }

        std::vector<std::shared_ptr<detail::Shape>> Level::getShapeList() {
            return this->_shapeList;
        }

        void Level::updateShape(std::shared_ptr<detail::Shape> oldShape, std::shared_ptr<detail::Shape> newShape) {
            for (unsigned int i = 0; i < this->_shapeList.size(); ++i) {
                if (oldShape->equals(this->_shapeList[i])) {
                    this->_shapeList[i] = newShape;
                    return;
                }
            }
        }

        void Level::removeShape(std::shared_ptr<detail::Shape> shape) {
            this->_shapeList.erase(std::remove(this->_shapeList.begin(), this->_shapeList.end(), shape), this->_shapeList.end());
        }

        sf::Vector2i Level::globalToLocalCoordinates(sf::Vector2f coords) const {
            return sf::Vector2i(static_cast<int>(coords.x) / this->_tileSize.x / static_cast<int>(std::stof(detail::utils::getConfigValue("tile_scale_x"))) + 1,
                                static_cast<int>(coords.y) / this->_tileSize.y / static_cast<int>(std::stof(detail::utils::getConfigValue("tile_scale_y"))) + 1);
        }

        sf::Vector2i Level::getTileSize() const {
            return this->_tileSize;
        }

        Shape::Shape(std::string name, sf::Color color) {
            this->_name = name;
            this->_color = color;
            this->_selected = false;
        }

        std::string Shape::getName() {
            return this->_name;
        }

        sf::Color Shape::getColor() const {
            return this->_color;
        }

        void Shape::setName(std::string name) {
            this->_name = name;
        }

        void Shape::setColor(sf::Color color) {
            this->_color = color;
        }

        Point::Point(std::string name, sf::Color color, sf::CircleShape dot) :
                Shape(name, color)
        {
            this->_dot = dot;
        }

        sf::CircleShape Point::getCircle() {
            return this->_dot;
        }

        sf::Color Point::getColor() const {
            return this->_color;
        }

        void Point::setColor(sf::Color color) {
            this->_color = color;
            this->_dot.setFillColor(sf::Color(color.r, color.g, color.b, 80));
            this->_dot.setOutlineColor(sf::Color(color.r, color.g, color.b, 160));
        }

        void Point::fixPosition(sf::Vector2i levelSize, sf::Vector2i tileSize, sf::Vector2f tileScale) {
            this->setPosition(sf::Vector2f(
                    std::min(this->getCircle().getPosition().x, levelSize.x * tileSize.x * tileScale.x -
                                                                (this->getCircle().getRadius() * 2)),
                    std::min(this->getCircle().getPosition().y, levelSize.y * tileSize.y * tileScale.y - (this->getCircle().getRadius() * 2))));
        }

        bool Point::isPointInside(sf::Vector2f point) {
            sf::Vector2f center = sf::Vector2f(this->_dot.getGlobalBounds().left + (this->_dot.getGlobalBounds().width / 2),
                                               this->_dot.getGlobalBounds().top + (this->_dot.getGlobalBounds().height / 2));
            auto x = pow(point.x - center.x, 2);
            auto y = pow(point.y - center.y, 2);
            return pow(this->_dot.getRadius(), 2) > (x + y);
        }

        void Point::select() {
            if (!this->_selected) {
                this->_selected = true;
                this->_dot.setOutlineThickness(this->_dot.getOutlineThickness() + 3);
                this->_dot.setFillColor(sf::Color(static_cast<sf::Uint8>(std::min(155, this->_dot.getFillColor().r + 16)),
                                                  static_cast<sf::Uint8>(std::min(155, this->_dot.getFillColor().g + 16)),
                                                  static_cast<sf::Uint8>(std::min(155, this->_dot.getFillColor().b + 16)),
                                                  static_cast<sf::Uint8>(std::min(155, this->_dot.getFillColor().a + 16))
                ));
            }
        }

        void Point::unselect() {
            if (this->_selected) {
                this->_selected = false;
                this->_dot.setOutlineThickness(this->_dot.getOutlineThickness() - 3);
                this->_dot.setFillColor(sf::Color(static_cast<sf::Uint8>(std::max(0, this->_dot.getFillColor().r - 16)),
                                                  static_cast<sf::Uint8>(std::max(0, this->_dot.getFillColor().g - 16)),
                                                  static_cast<sf::Uint8>(std::max(0, this->_dot.getFillColor().b - 16)),
                                                  static_cast<sf::Uint8>(std::max(0, this->_dot.getFillColor().a - 16))
                ));
            }
        }

        void Point::setPosition(sf::Vector2f pos) {
            this->_dot.setPosition(pos);
        }

        void Point::setSize(sf::Vector2f size) {
            this->_dot.setRadius(size.x);
        }

        void Point::draw(sf::RenderWindow *window) {
            window->draw(this->_dot);
        }

        bool Point::equals(std::shared_ptr<Shape> other) {
            std::shared_ptr<detail::Point> p = std::dynamic_pointer_cast<detail::Point>(other);
            if (p == nullptr) return false;
            return this->_name == p->getName() &&
                   this->_color == p->getColor() &&
                   this->_dot.getPosition() == p->getCircle().getPosition();
        }

        Line::Line(std::string name, sf::Color color, std::vector<std::shared_ptr<Point>> points) :
                Shape(name, color)
        {
            this->_points = points;
        }

        std::vector<std::shared_ptr<Point>> Line::getPoints() {
            return this->_points;
        }

        std::shared_ptr<Point> Line::getSelectedPoint(sf::Vector2f mousePos) {
            for (auto &p : this->_points) {
                if (p->isPointInside(mousePos)) {
                    return p;
                }
            }
            return nullptr;
        }

        void Line::deletePoint(std::shared_ptr<Point> p) {
            this->_points.erase(std::remove(this->_points.begin(), this->_points.end(), p), this->_points.end());
        }

        sf::Color Line::getColor() const {
            return this->_color;
        }

        void Line::setColor(sf::Color color) {
            this->_color = color;
        }

        void Line::fixPosition(sf::Vector2i levelSize, sf::Vector2i tileSize, sf::Vector2f tileScale) {
            for (auto &p : this->_points) {
                p->fixPosition(levelSize, tileSize, tileScale);
            }
        }

        bool Line::isPointInside(sf::Vector2f point) {
            //Points
            for (auto &p : this->_points) {
                if (p->isPointInside(point)) {
                    return true;
                }
            }
            return false;
        }

        void Line::select() {
            for (auto &p : this->_points) {
                p->select();
            }
        }

        void Line::unselect() {
            for (auto &p : this->_points) {
                p->unselect();
            }
        }

        void Line::setPosition(sf::Vector2f pos) {
            throw utils::NotImplementedException("setPosition");
        }

        void Line::setSize(sf::Vector2f size) {
            throw utils::NotImplementedException("setSize");
        }

        void Line::draw(sf::RenderWindow *window) {
            for (auto &p : this->_points) {
                p->draw(window);
            }
            if (this->_points.size() >= 2) {
                for (unsigned int i = 0; i < this->_points.size() - 1; ++i) {
                    sf::Vertex v[4];
                    sf::Vector2f point1 = sf::Vector2f(this->_points[i]->getCircle().getPosition().x + this->_points[i]->getCircle().getRadius(),
                                                       this->_points[i]->getCircle().getPosition().y + this->_points[i]->getCircle().getRadius());
                    sf::Vector2f point2 = sf::Vector2f(this->_points[i + 1]->getCircle().getPosition().x + this->_points[i + 1]->getCircle().getRadius(),
                                                       this->_points[i + 1]->getCircle().getPosition().y + this->_points[i + 1]->getCircle().getRadius());

                    sf::Vector2f direction = point2 - point1;

                    sf::Vector2f unitDirection = direction/std::sqrt(direction.x*direction.x+direction.y*direction.y);
                    sf::Vector2f unitPerpendicular(-unitDirection.y,unitDirection.x);

                    sf::Vector2f offset = (3.0f / 2.0f) * unitPerpendicular;

                    v[0].position = point1 + offset;
                    v[1].position = point2 + offset;
                    v[2].position = point2 - offset;
                    v[3].position = point1 - offset;
                    for (int i = 0; i < 4; ++i) {
                        v[i].color = this->_color;
                    }
                    window->draw(v, 4, sf::Quads);
                }
            }
        }

        bool Line::equals(std::shared_ptr<Shape> other) {
            std::shared_ptr<Line> l = std::dynamic_pointer_cast<Line>(other);
            if (l == nullptr) return false;
            return this->_name == l->getName() &&
                   this->_color == l->getColor() &&
                   [&]()->bool {
                       if (this->_points.size() != l->getPoints().size()) return false;
                       for (unsigned int i = 0; i < this->_points.size(); ++i) {
                           if (!this->_points[i]->equals(l->getPoints()[i])) {
                               return false;
                           }
                       }
                       return true;
                   }();
        }


    }
}