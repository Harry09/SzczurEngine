#include "Szczur/Modules/Cinematics/LoopText.hpp"

LoopText::LoopText(callme callback,const char * text,int jump,unsigned int size)
{  
    func = callback;
    m_text.setCharacterSize(size);
    m_text.setString(text);
    m_set = false;
    m_jump = jump;
}

bool LoopText::isSet()
{
    return m_set;
}

void LoopText::set(bool val)
{
    m_set=val;
}

void LoopText::setFont(sf::Font &font)
{
    m_text.setFont(font);
}

void LoopText::setCharacterSize(const unsigned int &size)
{
    m_text.setCharacterSize(size);
}

void LoopText::setSize(const sf::Vector2f &vec)
{
    m_text.setScale(vec);
}

void LoopText::setPosition(const sf::Vector2i &vec)
{
    m_text.setPosition(vec.x,vec.y);
}

void LoopText::draw()
{
    //getModule<rat::Window>().getWindow().draw(m_text);
    auto& w = getModule<rat::Window>();
    w.pushGLStates();
    w.draw(m_text);
    w.popGLStates();
}

void LoopText::setColor(const sf::Color &color)
{
    m_text.setColor(color);
}

void LoopText::callEvent()
{
    func();
}

int LoopText::getJump()
{
    return m_jump;
}