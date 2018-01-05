#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

#include "Element.hpp"

namespace rat {
    class Elements : public sf::Drawable, public sf::Transformable {
    public:
        Elements(float radius, float gap);
        ~Elements();

        void create(float radius, float gap);

        Element* add(size_t id);

        void update(float deltaTime);
    
        
    protected:
        float _radius;
        float _gap;
        std::vector<Element*> _elements;
    private:
        virtual Element* _createElement(size_t id) const;

        virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    };
}