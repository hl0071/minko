/*
Copyright (c) 2015 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "minko/Minko.hpp"
#include "AbstractMagnetometer.hpp"

#include "minko/Signal.hpp"

namespace minko
{
    namespace sensors
    {
        class Magnetometer : AbstractMagnetometer
        {
        public:
            typedef std::shared_ptr<Magnetometer> Ptr;
            typedef std::shared_ptr<AbstractMagnetometer> AbstractMagnetometerPtr;

            static
            Ptr
            getInstance()
            {
                if (_instance == nullptr)
                    _instance = Ptr(new Magnetometer());

                return _instance;
            }

            void
            initialize() override;

            void
            startTracking() override;

            void
            stopTracking() override;

            const math::vec3&
            getSensorValue() override;

            Signal<float, float, float>::Ptr
            onSensorChanged() override;

            bool
            isSupported() override;

        private:
            Magnetometer();

            std::shared_ptr<AbstractMagnetometer> _magnetometerManager;

            static Ptr _instance;
        };
    }
}

