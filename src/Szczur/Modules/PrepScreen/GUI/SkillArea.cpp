#include "SkillArea.hpp"

#include <algorithm>

#include "Szczur/Modules/GUI/GUI.hpp"
#include "Szczur/Modules/GUI/ScrollAreaWidget.hpp"
#include "Szczur/Modules/GUI/WindowWidget.hpp"

#include "ChosenSkillArea.hpp"
#include "GrayPPArea.hpp"

#include "../Skill/SkillCodex.hpp"

#include "Szczur/Utility/Logger.hpp" 
#include <ctime>

namespace rat
{
    SkillArea::SkillArea(GrayPPArea& sourceBar)
    :
    _sourceBar(sourceBar),
    _chosenColors({}),
    _curentProfession("Mele")
    {
        _border = new WindowWidget;
        _addWidget(_border);
        _border->setPadding(11.f, 11.f);

        _skillsScroller = new ScrollAreaWidget;
        _skillsScroller->setSize(300.f, 400.f);
        _border->add(_skillsScroller);

        _addBar(_infoBar);
        _infoBar.deactivate();
    }

    void SkillArea::initAssetsViaGUI(GUI& gui)
    {
        _font = gui.getAsset<sf::Font>("Assets/fonts/NotoMono.ttf");

        _skillsScroller->setPathTexture(gui.getAsset<sf::Texture>("Assets/Test/ScrollerBar.png"));
        _skillsScroller->setScrollerTexture(gui.getAsset<sf::Texture>("Assets/Test/Scroller.png"));
        _skillsScroller->setBoundsTexture(gui.getAsset<sf::Texture>("Assets/Test/ScrollerBound.png"));

        _infoBar.initAssetsViaGUI(gui);

        _border->setTexture(gui.getAsset<sf::Texture>("Assets/Test/Window.png"), 200);

        for(auto& skillBar : _skillBars)
        {
            skillBar->loadAssetsFromGUI(gui);
        }
    }
    

    void SkillArea::initViaSkillCodex(SkillCodex& skillCodex)
    {
        _skills.initViaSkillCodex(skillCodex);           
        size_t maxSkillBars = _skills.getMaxAmountOfSkills();

        _skillBars.clear();
        for(size_t i = 0; i < maxSkillBars; i++)
        {
            auto skillBar = std::make_unique<SkillBar>(*this);
            skillBar->setParent(_skillsScroller);
            _skillBars.emplace_back(std::move(skillBar));
        }
        deactivate();
    }
    
    void SkillArea::activate(const std::string& profession, const std::set<GlyphID>& colors)
    {
        _curentProfession = profession;
        _chosenColors = colors;
        auto skills = _skills.getWholeGlyphs(profession, colors);
        size_t newBarsAmount = skills.size();
        size_t i = 0;
        for(auto* skill : skills)
        {
            auto& skillBar = _skillBars[i++];
            skillBar->setSkill(skill);
        }
        _initNewSkillBarsAmount(newBarsAmount);
    }
    void SkillArea::_initNewSkillBarsAmount(size_t newAmount)
    {
        for(size_t i = 0; i < newAmount; i++)
        {
            auto& skillBar = _skillBars[i];
            skillBar->activate();
            skillBar->setPosition(0.f, float(i) * 80.f);
        }
        for(size_t i = newAmount; i < _activeBarsAmount; i++)
        {
            auto& skillBar = _skillBars[i];
            skillBar->deactivate();
            skillBar->setPosition(0.f, 0.f);
        }
        _activeBarsAmount = newAmount;
        recalculate();
    }
    
    void SkillArea::activate()
    {
        for(size_t i = 0; i < _activeBarsAmount; i++)
        {
            auto& skillBar = _skillBars[i];
            skillBar->activate();
        }
        recalculate();
    }
    void SkillArea::deactivate()
    {
        for(auto& skillBar : _skillBars)
        {
            skillBar->deactivate();
        }
    }
    
    void SkillArea::setGlyphs(const std::set<GlyphID>& colors)
    {
        activate(_curentProfession, colors);
    }
    void SkillArea::addColor(GlyphID color)
    {
        if(_chosenColors.find(color) == _chosenColors.end())
        {
            _chosenColors.emplace(color);
            setGlyphs(_chosenColors);
        }
    }
    void SkillArea::setProfession(const std::string& profession)
    {
        activate(profession, _chosenColors);

    }

    GrayPPArea& SkillArea::getSourceArea()
    {
        return _sourceBar;
    }

    void SkillArea::initChosenSkillArea(ChosenSkillArea& chosenSkillArea)
    {
        _chosenSkillArea = &chosenSkillArea;
    }
    ChosenSkillArea& SkillArea::getChosenSkillArea() const
    {
        return *_chosenSkillArea;
    }

    void SkillArea::setSkillInfo(Skill* skill, const sf::Vector2f& pos)
    {
        _infoBar.setPosition(pos);
        _infoBar.setName(skill->getName());
        _infoBar.activate();
    }
    bool SkillArea::isSkillInInfo(Skill* skill)
    {
        return _chosenSkill == skill;
    }
    void SkillArea::deactivateInfo()
    {
        _chosenSkill = nullptr;
        _infoBar.deactivate();
    }

    
    
    
    void SkillArea::recalculate()
    {
        int moveDir = 0;
        size_t activeIndex = 0;
        for(size_t i = 0; i < _activeBarsAmount; i++)
        {
            auto& skillBar = _skillBars[i];
            bool isBought = skillBar->isBought();
            bool isActivate = skillBar->isActivate();
            if(isActivate && isBought)
            {
                moveDir--;
                skillBar->deactivate();
            }
            else
            {
                if(!isActivate && !isBought)
                {
                    moveDir++;
                    skillBar->activate();
                    skillBar->setPosition(0.f, float(activeIndex) * 80.f);
                }
                else
                {
                    if(moveDir != 0) skillBar->move(0.f, float(moveDir) * 80.f);
                }
                activeIndex++;
            }
        }
        _skillsScroller->calculateSize();
    }
}