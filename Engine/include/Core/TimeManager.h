#pragma once

namespace GEM{
    class TimeManager{
    public:
        void Update() {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> delta = now - mLastTime;

            mDeltaTime = delta.count();
            mElapsedTime += mDeltaTime;
            mLastTime = now;
        }

        float GetDeltaTime() const {
            return mDeltaTime;
        }

        float GetElapsedTime() const {
            return mElapsedTime;
        }


        void Pause() {
            paused = true;
        }

        void Resume() {
            paused = false;
        }

        const bool GetPausedState() const {
            return paused;
        }

    private:
        std::chrono::steady_clock::time_point mLastTime = std::chrono::steady_clock::now();
        float mDeltaTime = 0.f;
        float mElapsedTime = 0.f;
        bool paused = false;
    };
}