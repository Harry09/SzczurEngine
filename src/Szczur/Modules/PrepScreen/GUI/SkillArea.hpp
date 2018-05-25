#pragma once

#include "BaseBar.hpp"

#include "SkillBar.hpp"
#include "../Skill/SortedSkillsContainer.hpp"
#include "../GlyphTypes.hpp"

#include <unordered_map>
#include <set>
#include <vector>
#include <memory>

#include "InfoBar.hpp"

namespace rat
{
    class GUI; class ScrollAreaWidget;
    class SkillCodex; class GrayPPArea;
    class ChosenSkillArea;

    class SkillArea : public BaseBar
    {
        using SkillBars_t = std::vector<std::unique_ptr<SkillBar>>;
    public:
        SkillArea(GrayPPArea& sourceBar);

        void initAssetsViaGUI(GUI& gui);
        void initViaSkillCodex(SkillCodex& skillCodex);

        void activate();
        void activate(const std::string& profession, const std::set<GlyphID>& colors);
        void deactivate();
        void setGlyphs(const std::set<GlyphID>& colors);
        void addColor(GlyphID color);
        void removeColor(GlyphID color);
        void setProfession(const std::string& profession);
        void initChosenSkillArea(ChosenSkillArea& chosenSkillArea);
        ChosenSkillArea& getChosenSkillArea() const;
        void recalculate();

        GrayPPArea& getSourceArea();

        void setSkillInfo(Skill* skill, const sf::Vector2f& pos = {});
        bool isSkillInInfo(Skill* enemy);
        void deactivateInfo();

    private:
        GrayPPArea& _sourceBar;
        ChosenSkillArea* _chosenSkillArea{nullptr};
        SortedSkillsContainer _skills;
        SkillBars_t _skillBars;
        size_t _activeBarsAmount{0};

        sf::Texture* _textureBar{nullptr};
        sf::Texture* _textureLocked{nullptr};
        sf::Font* _font{nullptr};

        WindowWidget* _border{nullptr};
        ScrollAreaWidget* _skillsScroller{nullptr};

        InfoBar _infoBar;
        Skill* _chosenSkill{nullptr};


        void _initNewSkillBarsAmount(size_t newAmount);

        float _barHeight{80.f};

        std::set<GlyphID> _chosenColors;
        std::string _curentProfession;
    };
}