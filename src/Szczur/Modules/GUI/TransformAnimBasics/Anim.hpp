#pragma once

#include <functional>
#include <array>

#include <SFML/Graphics/Color.hpp>

#include "AnimBase.hpp"

namespace rat
{
namespace gui
{
    template<typename W, typename T>
    class Anim : public AnimBase
    {
    using Setter_t =  std::function<void(W*, const T&)>;
    public:
        Anim(Setter_t setter, W* owner, AnimBase::Type type)
        :
        AnimBase(type),
        _owner(owner),
        _setter(setter)
        {
        }
        void setBounds(const T& fromValue, const T& toValue)
        {
            _start = fromValue;
            _end = toValue;
        }

    protected:
        virtual void _finish() override
        {
            std::invoke(_setter, _owner, _end);
        }
        virtual void _update() override
        {
            float prop = _getTimeProp();
            auto value = (_end - _start) * prop + _start;
            std::invoke(_setter, _owner, value);
        }

    
    private:
        W const * _owner{nullptr};
        const Setter_t _setter;

        T _start;
        T _end;
    };

    template<typename W>
    class Anim<W, sf::Color> : public AnimBase
    {
    using Setter_t =  std::function<void(W*, const sf::Color&)>;
    using ColorArray_t = std::array<sf::Uint8, 3>;
    public:
        Anim(Setter_t setter, W* owner, AnimBase::Type type)
        :
        AnimBase(type),
        _owner(owner),
        _setter(setter)
        {
        }
        void setBounds(const sf::Color& fromValue, const sf::Color& toValue)
        {
            _start = fromColorToArray(fromValue);
            _end = fromColorToArray(toValue);
        }

    protected:
        virtual void _finish() override
        {
            std::invoke(_setter, _owner,  fromArrayToColor(_end));
        }
        virtual void _update() override
        {
            std::array<float, 3u> diff;
            for(size_t i = 0; i < 3; i++)
            {
                diff[i] = float(_end[i]) - float(_start[i]);
            }
            ColorArray_t c;
            float prop = _getTimeProp();
            for(size_t i = 0; i < 3; i++)
            {
                auto addon = sf::Uint8(prop * float(diff[i]));
                c[i] = sf::Uint8(float(_start[i]) + addon);
            }

            std::invoke(_setter, _owner, fromArrayToColor(c));
        }

    
    private:
        W const * _owner{nullptr};
        const Setter_t _setter;

        ColorArray_t _start;
        ColorArray_t _end;

        ColorArray_t fromColorToArray(const sf::Color color) const
        {
            std::array<sf::Uint8, 3> ar;
            ar[0] = color.r; 
            ar[1] = color.g;
            ar[2] = color.b;
            return std::move(ar);
        }
        sf::Color fromArrayToColor(const ColorArray_t& color) const
        {
            sf::Color c;
            c.r = color[0];
            c.g = color[1];
            c.b = color[2];
            return c;
        }
    };
}
}