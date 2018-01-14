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

#include "rtc.hpp"
#include "util/logger.hpp"

using namespace Util;

namespace Core {
    auto RTC::readPort() -> std::uint8_t {
        Logger::log<LOG_DEBUG>("RTC: read");
        return 0;
    }

    void RTC::writePort(std::uint8_t data) {
        Logger::log<LOG_DEBUG>("RTC: write 0x{0:x}", data);

        bool old_cs = this->chip_select;

        if (portDirection(PORT_CS) == GPIO::GPIO_DIR_IN) {
            this->chip_select = data & (1<<PORT_CS);
        }

        if (this->chip_select) {
            if (this->chip_select != old_cs) {
                this->idx_bit  = 0;
                this->idx_byte = 0;
                this->byte_reg = 0;
            }
            /*if (portDirection(PORT_SCK) == GPIO::GPIO_DIR_IN && ((data>>PORT_SCK)&1) == 0) {
                this->byte_reg |= ((data>>PORT_SIO)&1)<<this->idx_bit++;
                if (this->idx_bit == 8) {
                    Logger::log<LOG_DEBUG>("RTC: received byte=0x{0:X}", this->byte_reg);
                    
                    this->idx_bit  = 0;
                    this->idx_byte = this->idx_byte + 1;
                    this->byte_reg = 0;
                }
            }*/
        }
    }
}