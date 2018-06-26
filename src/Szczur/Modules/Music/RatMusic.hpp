#pragma once

#include <SFML/Audio/Music.hpp>

#include "Szczur/Modules/AudioEffects/AudioEffect.hpp"

#define MUSIC_DEFAULT_PATH "Assets/Music/"

namespace rat
{
	class RatMusic : public sf::Music, public AudioEffect
	{

	private:

		unsigned int _counter = 0;

	public:
	
		std::string _name;
		float _bpm = 60;
		float _fadeTime = 0;

		RatMusic();

		void init(const std::string& name);

		void incrementCounter();
		void decrementCounter();

		unsigned int getCounterValue() const;

		void saveToJson(); //Only for editor

	private:

		void getJsonData();

	};
}