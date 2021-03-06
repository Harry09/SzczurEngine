#include "Playlist.hpp"

#include <utility> // move
#include <memory> // make_shared
#include <random> // mt19937, uniform_int_distribution
#include <chrono> // high_resolution_clock
#include <algorithm>

namespace rat 
{

	Playlist::Playlist(AudioEffects& effects)
		: _effects(effects)
	{

	}

	void Playlist::update(float deltaTime) 
	{
		if (!_hasBeenEverPlayed || _status == Status::Stopped) 
			return;
		
		if (_playlist[_currentID]->isEnding() && !_playlist[0]->getLoop()) {
			_endingFile = _playlist[_currentID];
			_isFileEnding =	true;
			_status = Status::Stopped;
			playNext();
		}

		if (_isFileEnding) {
			_playlist[_currentID]->start(deltaTime, _endingFile->getFadeTime());
			if (_endingFile->finish(deltaTime)) {
				_endingFile->getSource().cleanAllEffects();
				_isFileEnding = false;
			}
		}
		else
			_playlist[_currentID]->update(deltaTime);
	}

	void Playlist::stopUpdates()
	{
		_status = Status::Stopped;
	}
	
	void Playlist::add(MusicBase&& base)
	{	
		if (!includes(base.getName())) {
			_playlist.push_back(std::make_shared<MusicBase>(std::move(base)));
		}	

		if (_playlist.size() == 1) _playlist[0]->setLoop(true);
		if (_playlist.size() == 2) _playlist[0]->setLoop(false);
	}

	void Playlist::remove(const std::string& name) 
	{
		if (_playlist[_currentID]->getName() == name) {
			_status = Status::Stopped;
			_playlist[_currentID]->stop();
		}
		_playlist.erase(_playlist.begin() + getID(name));

		if (_playlist.size() == 1)
			_playlist[0]->setLoop(true);
	}

	Playlist::BasePointer_t Playlist::getCurrentPlaying() const
	{
		return _playlist[_currentID];
	}

	void Playlist::play(unsigned int id, float timeLeft)
	{
		setPlaylistToPlaying(id);
		_playlist[_currentID]->setTimeLeft(timeLeft);
	}

	void Playlist::play(unsigned int id)
	{
		if (id >= _playlist.size()) {
			_currentID = _playlist.size();
			return;
		}
			
		if (_status == Status::Playing && !_isFileEnding)
			_playlist[_currentID]->stop();

		setPlaylistToPlaying(id);
		_playlist[_currentID]->play();
	}

	void Playlist::play(const std::string& name) 
	{
		if (_status == Status::Paused) 				  unPause();
		else if (!name.empty())					  	  play(getID(name));
		else if (_playingMode == PlayingMode::Random) play(getRandomId());
		else										  play(0);
	}

	void Playlist::play(Playlist::BasePointer_t prevMusicFile, const std::string& name)
	{
		std::string prevName = prevMusicFile->getName();
		std::string toPlay;

		if(includes(prevName) && (name.empty() || prevName == name)) {
			toPlay = prevName;
		} 
		else {
			toPlay = name;
			if ((name != "" && _status == Status::Playing) || (_status != Status::Playing || _playlist[_currentID]->getName() != name)) {
				_isFileEnding = true;
				_endingFile = prevMusicFile;
			}
		}

		play(toPlay);
	}

	void Playlist::clear() 
	{
		_status = Status::Stopped;

		for (auto it : _playlist)
			it->stop();

		_currentID = 0;
		_playlist.clear();
	}

	void Playlist::setPlayingMode(PlayingMode mode) 
	{
		_playingMode = mode;
	}

	Playlist::Status Playlist::getStatus() const 
	{
		return _status;
	}

	void Playlist::pause() 
	{
		if (_status == Status::Playing) {
			_playlist[_currentID]->pause();
			_status = Status::Paused;
		}
	}

	void Playlist::stop() 
	{
		_playlist[_currentID]->stop();
		_playlist[_currentID]->reset();
		_status = Status::Stopped;
	}

	void Playlist::setVolume(float volume, const std::string& name) 
	{
		if (!name.empty()) {
			_playlist[getID(name)]->setVolume(volume);
		}
		else {
			for (auto it : _playlist)
				it->setVolume(it->getVolume() * (volume / 100));
		}
	}

	bool Playlist::includes(const std::string& name) const 
	{
		return (getID(name) != _playlist.size());
	}

	unsigned int Playlist::getRandomId() const 
	{
		unsigned int nextID = 0;

		if (_playlist.size() > 1) {
			auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
			std::mt19937 mt(seed);
			std::uniform_int_distribution<int> random(0, _playlist.size() - 1);
			
			do nextID = random(mt);
			while (nextID == _currentID);
		}

		return nextID;
	}

	void Playlist::playNext() 
	{
		if (_playingMode == PlayingMode::Orderly) {
			if (_currentID != _playlist.size() - 1)
				++_currentID;
			else
				_currentID = 0;
			play(_currentID);
		}
		else if (_playingMode == PlayingMode::Random) {
			play(getRandomId());
		}
	}

	void Playlist::unPause() 
	{
		if (_status == Status::Paused) {
			_playlist[_currentID]->play();
			_status = Status::Playing;
		}
	}

	unsigned int Playlist::getID(const std::string& name) const
	{
		for (unsigned int i = 0; i < _playlist.size(); ++i) {
			if (_playlist[i]->getName() == name)
				return i;
		}
		return _playlist.size();
	}

	const Playlist::Container_t& Playlist::getContainerRef() const
	{
		return _playlist;
	}

	void Playlist::setPlaylistToPlaying(int id)
	{
		_currentID = id;

		if (!_hasBeenEverPlayed)
			_hasBeenEverPlayed = true;

		setGlobalEffects();
		
		_status = Status::Playing;
	}

	void Playlist::setGlobalEffects()
	{
		if (_effects.globalEffects()) {
			_effects.getGlobalEffects().template sendAuxiliaryEffectsTo<RatMusic>(_playlist[_currentID]->getSource());
			LOG_INFO("Global effects loaded into ", _playlist[_currentID]->getName());
		}
	}
		
}