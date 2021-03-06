#include "SpriteComponent.hpp"

#include <experimental/filesystem>

#include "../Entity.hpp"
#include "../Scene.hpp"
#include "../ScenesManager.hpp"

#include "Szczur/Utility/ImGuiTweaks.hpp"
#include "Szczur/Utility/Convert/Windows1250.hpp"
#include "Szczur/Modules/Script/Script.hpp"
#include "Szczur/Modules/World/World.hpp"


namespace rat {
	SpriteComponent::SpriteComponent(Entity* parent)
	: Component { parent, fnv1a_64("SpriteComponent"), "SpriteComponent", Component::Drawable }
	{

	}
	///
	std::unique_ptr<Component> SpriteComponent::copy(Entity* newParent) const
	{
		auto ptr = std::make_unique<SpriteComponent>(*this);

		ptr->setEntity(newParent);

		return ptr;
	}

	///
	void SpriteComponent::setSpriteDisplayData(SpriteDisplayData* spriteDisplayData)
	{
		_spriteDisplayData = spriteDisplayData;
	}

	void SpriteComponent::setTexture(const std::string& texturePath)
	{
		auto* data = detail::globalPtr<World>->getScenes().getTextureDataHolder().getData(texturePath);
		setSpriteDisplayData(data);
	}

	///
	SpriteDisplayData* SpriteComponent::getSpriteDisplayData() const
	{
		return _spriteDisplayData;
	}

	///
	void* SpriteComponent::getFeature(Component::Feature_e feature)
	{
		if (feature == Feature_e::Drawable)	return static_cast<sf3d::Drawable*>(this);

		return nullptr;
	}

	///
	const void* SpriteComponent::getFeature(Component::Feature_e feature) const
	{
		if (feature == Feature_e::Drawable) return static_cast<const sf3d::Drawable*>(this);

		return nullptr;
	}

	///
	void SpriteComponent::loadFromConfig(Json& config)
	{
		Component::loadFromConfig(config);
		// auto& spriteDisplayDataHolder = getEntity()->getScene()->getSpriteDisplayDataHolder();
		auto name = mapUtf8ToWindows1250(config["spriteDisplayData"].get<std::string>());
		if(name != "") {
			// LOG_INFO("A")
			auto& textureDataHolder = getEntity()->getScene()->getScenes()->getTextureDataHolder();
			// LOG_INFO("B")
			auto* data = textureDataHolder.getData(name);
			// LOG_INFO("C")
			setSpriteDisplayData(data);
			// LOG_INFO("D")

			// bool found{false};
			// for(auto& it : spriteDisplayDataHolder) {
			// 	if(name == it.getName()) {
			// 		setSpriteDisplayData(&it);
			// 		found = true;
			// 	}
			// }
			// if(!found) {
			// 	try {
			// 		setSpriteDisplayData(&(spriteDisplayDataHolder.emplace_back(name)));
			// 	}
			// 	catch(const std::exception& exc) {

			// 	}
			// }
		}

		if (auto& val = config["parallax"]; !val.is_null()) _parallax = val;
		if (auto& val = config["parallaxValue"]; !val.is_null()) _parallaxValue = val;
	}

	///
	void SpriteComponent::saveToConfig(Json& config) const
	{
		Component::saveToConfig(config);
		config["spriteDisplayData"] = _spriteDisplayData ? mapWindows1250ToUtf8(_spriteDisplayData->getName()) : "";

		config["parallax"] = _parallax;
		config["parallaxValue"] = _parallaxValue;
	}

	void SpriteComponent::update(ScenesManager& scenes, float deltaTime)
	{
		if (_parallax) {
			auto camera = getEntity()->getScene()->getCamera();

			_parallexedPos = _parallaxValue * camera->getPosition().x;
		}
	}

	///
	void SpriteComponent::draw(sf3d::RenderTarget& target, sf3d::RenderStates states) const
	{
		// return;
		if(_spriteDisplayData) {
			states.transform *= getEntity()->getTransform();
			states.transform.translate(_parallexedPos, 0.f, 0.f);

			// @todo parallaxa, ale ustawiana przy `draw` przez `states` z X kamery.

			target.draw(*_spriteDisplayData, states);
		}
	}

	void SpriteComponent::renderHeader(ScenesManager& scenes, Entity* object) {
		if(ImGui::CollapsingHeader("Sprite##sprite_component")) {

			Component::drawOriginSetter<SpriteComponent>(&SpriteComponent::setOrigin);

			// Sprite data holder
			auto& sprites = object->getScene()->getSpriteDisplayDataHolder();

			// Load texture button
			if (ImGui::Button("Load texture...##sprite_component")) {
				
				// Path to .png file
			    std::string file = scenes.getRelativePathFromExplorer("Select texture", ".\\Assets", "Images (*.png, *.jpg, *.psd|*.png;*.jpg;*.psd");
			    
				// Load file to sprite data holder
				if (file != "") {
					auto* data = scenes.getTextureDataHolder().getData(file);
					setSpriteDisplayData(data);
					// try {
					// 	auto& it = sprites.emplace_back(file);
					// 	setSpriteDisplayData(&it);
					// }
					// catch(const std::exception& exc) {
					// 	setSpriteDisplayData(nullptr);

					// 	LOG_EXCEPTION(exc);
					// }
				}
			}

			// Change entity name
			if(getSpriteDisplayData()) {
				ImGui::SameLine();
				if(ImGui::Button("Change entity name")) {
					getEntity()->setName(std::experimental::filesystem::path(getSpriteDisplayData()->getName()).stem().string());
				}
			}

			// Show path to .png file
			ImGui::Text("Path:");
			ImGui::SameLine();
			ImGui::Text(getSpriteDisplayData() ? mapWindows1250ToUtf8(getSpriteDisplayData()->getName()).c_str() : "None");


			ImGui::Checkbox("Parallax", &_parallax);
			if (_parallax) {
				ImGui::DragFloat<ImGui::CopyPaste>("Value##parallax", _parallaxValue);
			}
		}
	}

	void SpriteComponent::initScript(ScriptClass<Entity>& entity, Script& script)
	{
		auto object = script.newClass<SpriteComponent>("SpriteComponent", "World");

		// Main
		object.set("setTexture", &SpriteComponent::setTexture);
		object.set("setTextureData", &SpriteComponent::setSpriteDisplayData);
		object.set("getTextureData", &SpriteComponent::getSpriteDisplayData);
		object.set("getTextureSize", [](SpriteComponent& comp){return glm::vec2(comp._spriteDisplayData->getTexture().getSize());});
		object.set("getEntity", sol::resolve<Entity*()>(&Component::getEntity));

		// Entity
		entity.set("addSpriteComponent", [&](Entity& e){return (SpriteComponent*)e.addComponent<SpriteComponent>();});
		entity.set("sprite", &Entity::getComponentAs<SpriteComponent>);

		object.init();
	}

	void SpriteComponent::setOrigin(int vertical, int horizontal)
	{
		if (_spriteDisplayData == nullptr)
			return;

		auto size = _spriteDisplayData->getTexture().getSize();

		glm::vec2 pos;

		switch (vertical)
		{
			case -1:
				pos.x = 0;
				break;
			case 0:
				pos.x = size.x / 2;
				break;
			case 1:
				pos.x = size.x;
				break;
		}

		switch (horizontal)
		{
			case -1:
				pos.y = 0;
				break;
			case 0:
				pos.y = size.y / 2;
				break;
			case 1:
				pos.y = size.y;
				break;
		}

		getEntity()->setOrigin(glm::vec3(pos, getEntity()->getOrigin().z));
	}
}
