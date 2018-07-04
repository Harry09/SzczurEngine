#include "RatMusic.hpp"

#include "Szczur/Utility/Logger.hpp"

#include <nlohmann/json.hpp>

namespace rat
{

	RatMusic::RatMusic()
		: AudioEffect(m_source)
	{
		LOG_INFO("RatMusic created");	
	}

	void RatMusic::init(const std::string& name)
	{
		_name = name;
		getJsonData();
	}

	void RatMusic::incrementCounter()
	{
		++_counter;
	}

	void RatMusic::decrementCounter()
	{
		--_counter;
	}

	unsigned int RatMusic::getCounterValue() const
	{
		return _counter;
	}

	const std::string& RatMusic::getName() const
	{
		return _name;
	}

	float RatMusic::getBPM() const
	{
		return _bpm;
	}	

	void RatMusic::setBPM(float bpm)
	{
		_bpm = bpm;
	}

	float RatMusic::getFadeTime() const
	{
		return _fadeTime;
	}

	void RatMusic::setFadeTime(float fadeTime)
	{
		_fadeTime = fadeTime;
	}

	//Only for editor
	void RatMusic::saveToJson()
	{
		std::array<std::vector<float>, 3> effects;
		for (unsigned int i = 0; i < MAX_AUX_FOR_SOURCE; ++i) {
            if(effectsTypes[i] != AudioEffect::EffectType::None) {
				switch(effectsTypes[i]) {
					case AudioEffect::EffectType::Reverb:
						effects[0] = {
							reverbData.density, reverbData.diffusion, reverbData.gain, reverbData.gainHf, 
							reverbData.decayTime, reverbData.decayHfRatio, reverbData.reflectionsGain, 
							reverbData.reflectionsDelay, reverbData.lateReverbGain, reverbData.lateReverbDelay, 
							reverbData.airAbsorptionGainHf, reverbData.roomRolloffFactor, (float)reverbData.decayHfLimit
						};
						break;
					case AudioEffect::EffectType::Echo:
						effects[1] = {
							echoData.delay, echoData.lrDelay, echoData.damping,
							echoData.feedback, echoData.spread
						}; 
						break;
					case AudioEffect::EffectType::Equalizer: 
						effects[2] = {
							eqData.lowCutoff, eqData.lowGain, eqData.highCutoff, eqData.highGain, 
							eqData.lowMidCenter, eqData.lowMidWidth, eqData.lowMidGain, eqData.highMidCenter,
							eqData.highMidWidth, eqData.highMidGain
						};
						break;
					default: 
						break;
				}
			}
		}

		nlohmann::json j;

		j[_name]["BPM"] = _bpm;
		j[_name]["FadeTime"] = _fadeTime;
		j[_name]["Volume"] = getVolume();
		j[_name]["Effects"] = effects;

		std::string path = MUSIC_DEFAULT_PATH;
		std::ofstream file(path + "Data/" + _name + ".json", std::ios::trunc);
        if (file.is_open()) {
            file << j;
        }
        file.close();
	}
	
	void RatMusic::getJsonData()
	{
		nlohmann::json j;
		std::string path = MUSIC_DEFAULT_PATH;
		std::ifstream file(path + "Data/" + _name + ".json");

		float numberOfBars;
		
		if (file.is_open()) {
			file >> j;
			file.close();
			_bpm = j[_name]["BPM"];
			numberOfBars = j[_name]["FadeTime"];
			setVolume(j[_name]["Volume"]);
			if(j[_name]["Effects"][0].size() > 0) {
				reverbData.density = j[_name]["Effects"][0][0];
				reverbData.diffusion = j[_name]["Effects"][0][1];
				reverbData.gain = j[_name]["Effects"][0][2];
				reverbData.gainHf = j[_name]["Effects"][0][3];
				reverbData.decayTime = j[_name]["Effects"][0][4];
				reverbData.decayHfRatio = j[_name]["Effects"][0][5];
				reverbData.reflectionsGain = j[_name]["Effects"][0][6];
				reverbData.reflectionsDelay = j[_name]["Effects"][0][7];
				reverbData.lateReverbGain = j[_name]["Effects"][0][8];
				reverbData.lateReverbDelay = j[_name]["Effects"][0][9];
				reverbData.airAbsorptionGainHf = j[_name]["Effects"][0][10];
				reverbData.roomRolloffFactor = j[_name]["Effects"][0][11];
				reverbData.decayHfLimit = (bool)j[_name]["Effects"][0][12];

				effectsTypes[lastFreeSlot()] = AudioEffect::EffectType::Reverb;
                getEffect<Reverb>().density(reverbData.density);
				getEffect<Reverb>().diffusion(reverbData.diffusion);
				getEffect<Reverb>().gain(reverbData.gain);
				getEffect<Reverb>().gainHf(reverbData.gainHf);
				getEffect<Reverb>().decayTime(reverbData.decayTime);
				getEffect<Reverb>().decayHfRatio(reverbData.decayHfRatio);
				getEffect<Reverb>().reflectionsGain(reverbData.lateReverbGain);
				getEffect<Reverb>().reflectionsDelay(reverbData.reflectionsDelay);
				getEffect<Reverb>().lateReverbGain(reverbData.lateReverbGain);
				getEffect<Reverb>().lateReverbDelay(reverbData.lateReverbDelay);
				getEffect<Reverb>().airAbsorptionGainHf(reverbData.airAbsorptionGainHf);
				getEffect<Reverb>().roomRolloffFactor(reverbData.roomRolloffFactor);
				getEffect<Reverb>().decayHfLimit(reverbData.decayHfLimit);
			}
			if(j[_name]["Effects"][1].size() > 0) {
				echoData.delay = j[_name]["Effects"][1][0];
				echoData.lrDelay = j[_name]["Effects"][1][1]; 
				echoData.damping = j[_name]["Effects"][1][2];
				echoData.feedback = j[_name]["Effects"][1][3]; 
				echoData.spread = j[_name]["Effects"][1][4];

				effectsTypes[lastFreeSlot()] = AudioEffect::EffectType::Echo;
                getEffect<Echo>().delay(echoData.delay);
				getEffect<Echo>().lrDelay(echoData.lrDelay);
			    getEffect<Echo>().damping(echoData.damping);
				getEffect<Echo>().feedback(echoData.feedback);
				getEffect<Echo>().spread(echoData.spread);
			}
			if(j[_name]["Effects"][2].size() > 0) {
				eqData.lowCutoff = j[_name]["Effects"][2][0];
				eqData.lowGain = j[_name]["Effects"][2][1];
				eqData.highCutoff = j[_name]["Effects"][2][2]; 
				eqData.highGain = j[_name]["Effects"][2][3]; 
				eqData.lowMidCenter = j[_name]["Effects"][2][4]; 
				eqData.lowMidWidth = j[_name]["Effects"][2][5]; 
				eqData.lowMidGain = j[_name]["Effects"][2][6]; 
				eqData.highMidCenter = j[_name]["Effects"][2][7]; 
				eqData.highMidWidth = j[_name]["Effects"][2][8]; 
				eqData.highMidGain = j[_name]["Effects"][2][9];

				effectsTypes[lastFreeSlot()] = AudioEffect::EffectType::Equalizer;
                getEffect<Equalizer>().lowGain(eqData.lowGain);
				getEffect<Equalizer>().lowCutoff(eqData.lowCutoff);
				getEffect<Equalizer>().lowMidGain(eqData.lowMidGain);
				getEffect<Equalizer>().lowMidCenter(eqData.lowMidCenter);
				getEffect<Equalizer>().lowMidWidth(eqData.lowMidWidth);
				getEffect<Equalizer>().highMidGain(eqData.highMidGain);
				getEffect<Equalizer>().highMidCenter(eqData.highMidCenter);
				getEffect<Equalizer>().highMidWidth(eqData.highMidWidth);
				getEffect<Equalizer>().highGain(eqData.highGain);
				getEffect<Equalizer>().highCutoff(eqData.highCutoff);
			}
		}
		else {
			_bpm = 60;
			numberOfBars = 0;
			setVolume(100);
		}

		if(numberOfBars > 0) {
			float barTime = 240 / _bpm;
			_fadeTime = barTime * numberOfBars;
		}
	}

}