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
    auto RTC::readSIO() -> bool {
        this->byte_reg &= ~(1 << this->idx_bit);
        this->byte_reg |=  (this->port.sio << this->idx_bit);

        if (++this->idx_bit == 8) {
            Logger::log<LOG_DEBUG>("RTC: byte_reg=0x{0:X}", this->byte_reg);
            this->idx_bit  = 0;
            return true;
        }

        return false;
    }

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

        int old_sck = this->port.sck;
        int old_cs  = this->port.cs;

        int sck = this->port.sck = (data>>PORT_SCK)&1;
        int sio = this->port.sio = (data>>PORT_SIO)&1;
        int cs  = this->port.cs  = (data>>PORT_CS )&1;

        Logger::log<LOG_DEBUG>("RTC: sck={0} sio={1} cs={2}", sck, sio, cs);

        if (!old_cs &&  cs) {
            Logger::log<LOG_DEBUG>("RTC: enabled.");

            // Not tested but probably needed.
            this->state    = WAIT_CMD;
            this->idx_bit  = 0;
            this->idx_byte = 0;
        }
        if ( old_cs && !cs) Logger::log<LOG_DEBUG>("RTC: disbled.");

        if (!cs) return;

        switch (this->state) {
            case WAIT_CMD: {
                // CHECKME: apparently data is accepted on rising clock edge?
                if (old_sck && !sck) {
                    bool completed = readSIO();

                    // Wait until the complete CMD byte arrived.
                    if (!completed) return;

                    uint8_t cmd = 0;

                    // Check for FWD/REV format specifier
                    if ((this->byte_reg & 15) == 6) {
                        cmd = this->byte_reg;
                    }
                    else if ((this->byte_reg >> 4) == 6) {
                        Logger::log<LOG_DEBUG>("RTC: game uses REV format.");
                        // Reverse bit pattern.
                        for (int i = 0; i < 7; i++)
                            cmd |= ((this->byte_reg>>(i^7))&1)<<i;
                    }
                    else {
                        Logger::log<LOG_WARN>("RTC: undefined state: unknown command format.");
                        return;
                    }

                    this->cmd = (cmd>>4)&7;
                    this->idx_byte = 0;
                    this->idx_bit  = 0; // actually not needed?

                    if (cmd & 0x80) {
                        Logger::log<LOG_DEBUG>("RTC: cmd={0} (read)", this->cmd);
                        this->state = SENDING;
                    }
                    else {
                        Logger::log<LOG_DEBUG>("RTC: cmd={0} (write)", this->cmd);
                        this->state = RECEIVING;
                    }
                }
                break;
            }
            case SENDING: {
                break;
            }
            case RECEIVING: {
                if (this->idx_byte < 8) {
                    //this->data[this->idx_byte]
                }
                break;
            }
        }
    }
}
