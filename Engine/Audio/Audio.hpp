#ifndef GRAPHICAL_LIBRARY_AUDIO_HPP
#define GRAPHICAL_LIBRARY_AUDIO_HPP

#include "Core/Core.hpp"

namespace GraphicalLibrary {
    class Audio {
    public:
        static void InitializeAudioDevice(); // Renamed from InitDevice
        static void ShutdownAudioDevice();   // Renamed from CloseDevice
        static Sound LoadAudioFromFile(const std::string& filePath); // Renamed from LoadSoundFromFile
        static void ReleaseAudio(Sound sound); // Renamed from UnloadSound
        static void PlayAudio(Sound sound);    // Renamed from PlaySound
    };
}

#endif // GRAPHICAL_LIBRARY_AUDIO_HPP