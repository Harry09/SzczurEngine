#pragma once

#include <glm/glm.hpp>
#include <boost/container/flat_map.hpp>

#include "RatSound.hpp"
#include "SoundAssets.hpp"

#include "Szczur/Modules/Script/Script.hpp"

namespace rat
{
    class SoundBase
    {
        using Second_t = float;

        enum class CallbackType 
        {
            onStart,
            onFinish
        };

    public:
    
		using Status = sf::SoundSource::Status;
        using SolFunction_t = sol::function;
        using CallbacksContainer_t = boost::container::flat_map<CallbackType, SolFunction_t>;
    
    private:

        inline static float SoundVolume {100};
        
        SoundAssets& _assets;
            
        struct
        {
            Second_t beginTime {0};
            Second_t endTime   {0};
        } offset;

        Second_t _length      {0};
        Second_t _playingTime {0};

        std::string _name     {""};
        std::string _fileName {""};

        float _volume {100};
        float _pitch  {1};
            
        SoundBuffer* _buffer {nullptr};
        RatSound _sound;

        CallbacksContainer_t _callbacks;

    public:

        SoundBase() = delete;
        
        SoundBase(SoundAssets& assets);
        SoundBase(SoundAssets& assets, const std::string& name);
        
        ~SoundBase();

        void init();
        bool load();
        
        bool setBuffer(SoundBuffer* buffer);

        static void initScript(Script& script);

        void update();

        void play();
        void stop();
        void pause();

        void setCallback(CallbackType type, SolFunction_t callback);

        float getVolume() const;
        void setVolume(float volume);

        static float GetSoundVolume();
        static void SetSoundVolume(float volume);

        float getPitch() const;
        void setPitch(float pitch);

        bool isRelativeToListener() const;
        void setRelativeToListener(bool relative);

        float getAttenuation() const; 
        void setAttenuation(float attenuation);

        float getMinDistance() const; 
        void setMinDistance(float minDistance);

        void setPosition(float x, float y, float z);
        glm::vec3 getPosition() const;

        bool getLoop() const;
        void setLoop(bool loop);

        Status getStatus() const;

        float getPlayingOffset();
        void setOffset(Second_t beginT, Second_t endT);
        Second_t getLength() const;

        Second_t getBeginTime() const;
        Second_t getEndTime() const;  

        void setName(const std::string& name);
        const std::string getName() const;
        void setFileName(const std::string& fileName);
        std::string getFileName() const;

        template <typename T>
		T& getEffect() {
            return _sound.getEffect<T>();
        }

        template <typename T>
    	void cleanEffect() {
            _sound.cleanEffect<T>();
        }
        void cleanEffects();

    private:

        bool loadBuffer();
        std::string getPath() const;

        void callback(CallbackType type);

    };
}
