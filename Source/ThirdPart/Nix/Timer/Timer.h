#ifndef TIMER_H
#define TIMER_H

#include <stdlib.h>
#include <stdint.h>

namespace Nix {

    class Timer
    {
        private:
            double _secondsPerCount;
            double _deltaTime;
            int64_t _baseTime;
            int64_t _pausedTime;
            int64_t _stopTime;
            int64_t _prevTime;
            int64_t _currTime;
            bool _stopped;

        public:
            Timer(/* args */);
            ~Timer();

            float totalTime() const;
            float deltaTime() const;

            void reset();
            void start();
            void stop();
            void tick();
    };

}




#endif