#include "Audio.hpp"
#include <raylib.h>

namespace Engine {
    void Audio::InitializeAudioDevice() {
        InitAudioDevice();
    }

    void Audio::ShutdownAudioDevice() {
        CloseAudioDevice();
    }

    Sound Audio::LoadAudioFromFile(const std::string& filePath) {
        return LoadSound(filePath.c_str());
    }

    void Audio::ReleaseAudio(Sound sound) {
        UnloadSound(sound);
    }

    void Audio::PlayAudio(Sound sound) {
        PlaySound(sound);
    }
}
