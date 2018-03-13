#include "Options.hpp"
#include <iostream>
namespace rat {
    Options::Options() {

    }

    Options::~Options() {
        for(auto it : _options)
            delete it;
    }

    void Options::initScript(Script& script) {
        auto object = script.newClass<Options>("Options", "Dialog");
        
        object.setProperty(
            "add",
            []() {},
            [](Options& owner, sol::table tab) {
                if(tab["target"].valid()) {
                    auto condValid = tab["condition"].valid();
                    auto actiValid = tab["action"].valid();
                    if(condValid && actiValid) {
                        owner.addOption(
                            tab.get<sol::function>("condition"),
                            tab["target"],
                            tab.get<sol::function>("action")
                        );
                    }
                    else if(condValid) {
                        owner.addOption(
                            tab.get<sol::function>("condition"),
                            tab["target"],
                            nullptr
                        );
                    }
                    else if(actiValid) {
                        owner.addOption(
                            nullptr,
                            tab["target"],
                            tab.get<sol::function>("action")
                        );
                    }
                    else {
                        owner.addOption(
                            nullptr,
                            tab["target"],
                            nullptr
                        );
                    }
                }
            }
        );
        

        object.init();
    }

    void Options::addOption(Condition_t condition, Key_t target, AfterAction_t afterAction) {
        _options.push_back(new Option{condition, target, afterAction});
    }

    bool Options::checkIfRunsWith(Key_t id) const {
        return _runners.find(id) != _runners.end();
    }

    void Options::forEach(std::function<void(Option*)> func) {
        for(auto it : _options)
            std::invoke(func, it);
    }
}
