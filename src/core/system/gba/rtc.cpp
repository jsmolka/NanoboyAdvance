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
        //Logger::log<LOG_DEBUG>("RTC: read");
        return 0;
    }

    void RTC::writePort(std::uint8_t data) {
        // Verify port directions.
        if (portDirection(PORT_CS) != GPIO::GPIO_DIR_OUT) {
            Logger::log<LOG_WARN>("RTC: wrong CS port direction.");
        }
        if (portDirection(PORT_SCK) != GPIO::GPIO_DIR_OUT) {
            Logger::log<LOG_WARN>("RTC: wrong SCK port direction.");
        }

        int old_sck = this->sck;

        int sck = (data>>PORT_SCK)&1;
        int sio = (data>>PORT_SIO)&1;
        int cs  = (data>>PORT_CS )&1;

        this->sck = sck;

        Logger::log<LOG_DEBUG>("RTC: sck={0} sio={1} cs={2}", sck, sio, cs);

        bool old_cs = this->chip_select;

        this->chip_select = cs;

        if (!old_cs &&  cs) Logger::log<LOG_DEBUG>("RTC: enabled.");
        if ( old_cs && !cs) Logger::log<LOG_DEBUG>("RTC: disbled.");

        if (!this->chip_select) return;

        if (sck && !old_sck) {
            this->byte_reg |= (sio<<this->idx_bit);
            if (++this->idx_bit == 8) {
                Logger::log<LOG_DEBUG>("RTC: byte_reg=0x{0:X}", this->byte_reg);
                this->byte_reg = 0;
                this->idx_bit  = 0;
            }
        }

        /*switch (this->state) {
            case WAIT_INIT_1: {
                if (sck == 1 && cs == 0) {
                    Logger::log<LOG_DEBUG>("RTC: init_1 accepted.");
                    this->state = WAIT_INIT_2;
                }
                else {
                    Logger::log<LOG_DEBUG>("RTC: init_1 rejected.");
                }
                break;
            }
            case WAIT_INIT_2: {
                if (sck == 1 && cs == 1) {
                    Logger::log<LOG_DEBUG>("RTC: init_2 accepted.");
                    this->state = WAIT_CMD;
                }
                else {
                    Logger::log<LOG_DEBUG>("RTC: init_2 rejected.");
                }
                break;
            }
        }*/
    }
}