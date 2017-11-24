#include "GUI.hpp"

#include <iostream>

namespace rat {
    void GUI::init() {

        Widget* nickname = _root.add(new ImageWidget("data/button.png"));
        nickname->setPosition({100.f, 100.f});

        InputWidget *inputWidget = new InputWidget(
            TextWidget::createText(";", sf::Color::White, 25u),
            "data/consolab.ttf",
            15u
        );

        nickname->add(inputWidget);

        nickname->setCallback(Widget::CallbackType::onHoverIn, [inputWidget](Widget*){
            inputWidget->active();
        });

        nickname->setCallback(Widget::CallbackType::onHoverOut, [inputWidget](Widget*){
            inputWidget->deactive();
        });


        
        /*for(int i = 0; i<5; i++){
            std::string name;
            switch(i) {
                case 0:
                    name = "Start";
                break;
                case 1:
                    name = "Load";
                break;
                case 2:
                    name = "Options";
                break;
                case 3:
                    name = "Credits";
                break;
                case 4:
                    name = "Exit";
                break;
            }
            TextWidget* textWidget = new TextWidget(
                TextWidget::createText(name, sf::Color::Red, 70u),
                "data/consolab.ttf"
            );

            Widget* test = _root.add(textWidget);

            test->setCallback(Widget::CallbackType::onHoverIn, [textWidget](Widget* owner) {
                //std::cout << "Yellow\n";
                textWidget->setColor(sf::Color::Yellow);
            });
            test->setCallback(Widget::CallbackType::onHoverOut, [textWidget](Widget* owner) {
                //std::cout << "Red\n";
                textWidget->setColor(sf::Color::Red);
            });
            test->setCallback(Widget::CallbackType::onPress, [textWidget](Widget* owner) {
                //std::cout << "Blue\n";
                textWidget->setColor(sf::Color::Blue);
            });
            test->setCallback(Widget::CallbackType::onRelease, [textWidget](Widget* owner) {
                //std::cout << "Red\n";
                textWidget->setColor(sf::Color::Red);
            });
            test->setPosition({255.f, 120.f + (110.f*i)});
        }
        _root.setCallback(Widget::CallbackType::onHover, [](Widget*){
            //std::cout << "Im in _root\n";
        });*/
        
        

    }

    void GUI::input(const sf::Event& event) {
        if(
            event.type == sf::Event::MouseButtonPressed || 
            event.type == sf::Event::MouseButtonReleased || 
            event.type == sf::Event::MouseMoved ||
            event.type == sf::Event::TextEntered
        ) {
            _root.input(event);
        }
    }

    void GUI::update(float deltaTime) {
        _root.update(deltaTime);
    }

    void GUI::render() {
        getModule<Canvas>().draw(Canvas::LayerId::GUI, _root);
    }
}