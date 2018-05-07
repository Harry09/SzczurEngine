#pragma once

#include "Widget.hpp"

#include "Szczur/Modules/GUITest/NinePatch.hpp"

namespace rat
{
    class WindowWidget : public Widget
    {
    public:
        WindowWidget();
        void setTexture(sf::Texture* texture, int padding);
        void setScale(const sf::Vector2f& scale);
        void setScale(float x, float y);

        void setPatchAmount(const sf::Vector2u& amount);
        void setPatchAmount(unsigned int horizontalAmount, unsigned int verticalAmount);

        virtual void setPadding(const sf::Vector2f& padding) override;
        virtual void setPadding(float x, float y) override;
    protected:
        virtual void _draw(sf::RenderTarget& target, sf::RenderStates states) const override;
		virtual sf::Vector2u _getSize() const override;
		virtual void _calculateSize() override;
    private:
        NinePatch _ninePatch;
        bool _isPaddingSet{false};
        sf::Vector2f _scale{1.f, 1.f};

        sf::Vector2u _minWinSize;
        sf::Vector2u _patchesAmount;
        bool _isPathesAmountSet{false};

        void _calcPadding();
        void _calcPatchesAmount();
    };
}