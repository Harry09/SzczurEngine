#pragma once

#include <vector>
#include <SFML/Graphics.hpp>
#include "UsableItem.hpp"

namespace rat {	
	class EquipmentSlot; class UsebleItem; class Widget; class ImageWidget; class Equipment; class ReplaceItem;
	typedef std::multimap<std::string, EquipmentSlot*> itemMap_t;

	class NormalSlots			//part of equipment for normal items looking like a grid
	{
		friend class Equipment;
	public:
		NormalSlots(unsigned int slotNumber, sf::Texture* frameText, sf::Texture* highlightText, sf::Vector2i frameSize, Equipment* equipment);

		bool addItem(EquipmentObject* item);
		bool removeItem(sf::String itemName);
		bool removeItem(int index);
		void resizeSlots(size_t newSize);
		void setParent(Widget* newBase);
		itemMap_t getItemMap();
		int getFreeSlotsAmount();
		int getSlotsAmount();

		void setPropPosition(sf::Vector2f);
		sf::Vector2f getPosition();

		void update(float deltaTime);

	private:
		unsigned int _slotAmount;
		sf::Vector2i _frameSize;

		Widget* _base;

		void _removeSlotDropped(std::shared_ptr<EquipmentSlot>);

		sf::Texture* _frameText;

		itemMap_t _occupiedSlots;		//slots with items
		std::vector<EquipmentSlot*> _freeSlots;
		std::vector<EquipmentSlot*> _allSlots;

		//std::pair<bool, EquipmentSlot*> isMouseOverSlot(sf::Vector2i position, bool freeSlot);
		std::shared_ptr<EquipmentSlot> _slotHeld;
		std::shared_ptr<EquipmentSlot> _slotDropped;
		EquipmentObject* _itemHeld;
		ImageWidget* _itemHeldWidget;

		EquipmentObject* _itemForReplacing;

		sf::Vector2i _originalMousePosition;

		void _onMouseButtonPressed(std::shared_ptr<EquipmentSlot> clickedObj);
		void _onMouseButtonReleased();

		Equipment* _equipment;

		void _checkForDoubleClick(float deltaTime);
		bool _isLeftMouseButtonPressed;
		bool _isCountingToDoubleClickEnabled;	//used in detection of doubleclick
		float _timeFromLastClick;

		void _stopReplacing();
	};
}
