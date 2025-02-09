#ifndef ENGINE_AUDIO_HPP
#define ENGINE_AUDIO_HPP

#include <raylib.h>
#include <string>

namespace Engine {
    class Audio {
    public:
        static void InitializeAudioDevice();
        static void ShutdownAudioDevice();
        static Sound LoadAudioFromFile(const std::string& filePath);
        static void ReleaseAudio(Sound sound);
        static void PlayAudio(Sound sound);
    };
}

#endif // ENGINE_AUDIO_HPP
