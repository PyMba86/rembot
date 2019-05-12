#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <stack>
#include <sstream>
#include <cstring>
#include "../libext/imgui.h"

namespace rb {
    namespace detail {

        /*
         * Forward declares
         */
        class Shape;

        enum class Features {
            None, Map
        };

        enum class WindowTypes {
            None, TilesetWindow, NewMapWindow, ConfigWindow, MapSelectWindow, AboutWindow, LightEditorWindow,
            NewAnimatedSpriteWindow, NewAnimationWindow, RemoveAnimationWindow, EntityListWindow, EntityPropertiesWindow, ShapeColorWindow,
            ConfigureMapWindow, ConfigureBackgroundColorWindow, ConsoleWindow, BackgroundWindow, TileTypeWindow,
            TileTypeColorSelectionWindow
        };

        namespace utils {
            template<class C, class T>
            inline bool contains(const C &v, const T &x) {
                return std::end(v) != std::find(std::begin(v), std::end(v), x);
            };

            std::vector<std::string> split(std::string str, char c);

            std::vector<std::string> split(const std::string &str, const std::string &delim, int count = -1);

            std::vector<std::string>
            splitVector(const std::vector<std::string> &v, const std::string &delim, int index = 0);

            std::vector<const char *> getFilesInDirectory(std::string directory);

            std::string getConfigValue(std::string key);

            sf::Color getColor(const ImVec4 &c);

            sf::Color getColor(const std::string &intValue);

            sf::Color getColor(const long long &intValue);

            class NotImplementedException : public std::logic_error {
            public:
                explicit NotImplementedException(std::string method = "") : std::logic_error(
                        "Function not yet implemented." + (method.length() > 0 ? " Method: " + method : "")) {};
            };
        }

        class Graphics {
        public:
            explicit Graphics(sf::RenderWindow *window);

            void draw(const sf::Vertex *vertices, unsigned int vertexCount, sf::PrimitiveType type,
                      const sf::RenderStates &states = sf::RenderStates::Default);

            void setViewPosition(sf::Vector2f pos);

            void zoom(float n, sf::Vector2i pixel);

            void update(float elapsedTime, sf::Vector2f tileSize, bool windowHasFocus);

            sf::View getView() const;

            float getZoomPercentage() const;

            void setZoomPercentage(float zoomPercentage);

        private:
            sf::RenderWindow *_window;
            sf::View _view;
            float _zoomPercentage;
        };


        class Level {
        public:
            Level(std::shared_ptr<Graphics> graphics);

            ~Level() = default;

            void createMap(sf::Vector2i size, sf::Vector2i tileSize);

            void update(float elapsedTime);

            sf::Vector2i getSize() const;

            void setSize(sf::Vector2i size);

            sf::Vector2i getTileSize() const;

            void addShape(std::shared_ptr<detail::Shape> shape);

            std::vector<std::shared_ptr<detail::Shape>> getShapeList();

            void updateShape(std::shared_ptr<detail::Shape> oldShape, std::shared_ptr<detail::Shape> newShape);

            void removeShape(std::shared_ptr<detail::Shape> shape);

            sf::Vector2i globalToLocalCoordinates(sf::Vector2f coords) const;

        private:
            sf::Vector2i _size;
            std::vector<std::shared_ptr<detail::Shape>> _shapeList;
            std::shared_ptr<Graphics> _graphics;
            sf::Vector2i _tileSize;
        };

        class Shape {
        public:
            Shape(std::string name, sf::Color color);

            std::string getName();

            virtual sf::Color getColor() const;

            void setName(std::string name);

            virtual void setColor(sf::Color color);

            virtual void fixPosition(sf::Vector2i levelSize, sf::Vector2i tileSize, sf::Vector2f tileScale) = 0;

            virtual bool isPointInside(sf::Vector2f point) = 0;

            virtual void select() = 0;

            virtual void unselect() = 0;

            virtual void setPosition(sf::Vector2f pos) = 0;

            virtual void setSize(sf::Vector2f size) = 0;

            virtual void draw(sf::RenderWindow *window) = 0;

            virtual bool equals(std::shared_ptr<Shape> other) = 0;

        protected:
            std::string _name;
            sf::Color _color = sf::Color::White;
            bool _selected;
        };
    }
}