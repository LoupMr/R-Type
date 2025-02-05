#ifndef GRAPHICAL_LIBRARY_AUDIO_HPP
#define GRAPHICAL_LIBRARY_AUDIO_HPP

#include "Core/Core.hpp"

namespace GraphicalLibrary {
    class Audio {
    public:
        static void InitializeAudioDevice();
        static void ShutdownAudioDevice();
        static Sound LoadAudioFromFile(const std::string& filePath);
        static void ReleaseAudio(Sound sound);
        static void PlayAudio(Sound sound);
    };
}

#endif // GRAPHICAL_LIBRARY_AUDIO_HPP