/**
  * Copyright (C) 2018 flerovium^-^ (Frederic Meyer)
  *
  * This file is part of NanoboyAdvance.
  *
  * NanoboyAdvance is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * NanoboyAdvance is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with NanoboyAdvance. If not, see <http://www.gnu.org/licenses/>.
  */

#pragma once

#include "gpio.hpp"

namespace Core {

    class RTC : public GPIO {
    private:
        enum RTCPort {
            PORT_SCK = 0,
            PORT_SIO = 1,
            PORT_CS  = 2
        };

        enum RTCState {
            WAIT_CMD,
            SENDING,
            RECEIVING,
            COMPLETE
        };

        enum RTCRegister {
            FORCE_RESET = 0,
            DATETIME    = 0x2,
            CONTROL     = 0x4
        };

        int idx_bit  {0};
        int idx_byte {0};

        uint8_t byte_reg {0};

        struct RTCPortData {
            int sck;
            int sio;
            int cs;
        } port;

        RTCState state { WAIT_CMD };

        int cmd;
        std::uint8_t data[8];

        struct RTCCommandRegister {
            bool unknown;
            bool minute_irq;
            bool mode_24h;
            bool power_off;
        } control;

        auto readSIO() -> bool;
        void processCommandBit();
        void readRTC(RTCRegister reg);
        void writeRTC(RTCRegister reg);

    protected:
        auto readPort() -> std::uint8_t final;

        void writePort(std::uint8_t data) final;

    public:
        using GPIO::GPIO;

        void reset() final {
            GPIO::reset();

            // Reset cached port state.
            this->port.sck = 0;
            this->port.sio = 0;
            this->port.cs  = 0;

            // Reset control register.
            this->control.unknown    = false;
            this->control.minute_irq = false;
            this->control.mode_24h   = false;
            this->control.power_off  = false;
        }
    };
}
