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
            WAIT_INIT_1,
            WAIT_INIT_2,
            WAIT_CMD,
            SENDING,
            RECEIVING,
            COMPLETE
        };

        enum RTCCommand {
            TODO
        };

        int idx_bit  {0};
        int idx_byte {0};

        uint8_t byte_reg {0};

        int  sck;
        bool chip_select { false };

        RTCState state { WAIT_INIT_1 };

    protected:
        auto readPort() -> std::uint8_t final;

        void writePort(std::uint8_t data) final;

    public:
        using GPIO::GPIO;

        void reset() final {
            GPIO::reset();
            this->state    = WAIT_INIT_1;
            this->idx_bit  = 0;
            this->idx_byte = 0;
            this->chip_select = false;
        }
    };
}