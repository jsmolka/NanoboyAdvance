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

#include "gpio.hpp"

#include "util/logger.hpp"

using namespace Util;

namespace Core {
    auto GPIO::read(std::uint32_t address) -> std::uint8_t {
        Logger::log<LOG_DEBUG>("GPIO: read from 0x{0:X}", address);

        switch (address) {
            // TODO: implement others.
            case GPIO_DATA: {
                return readPort()&15;
            }
        }

        return 0;
    }

    void GPIO::write(std::uint32_t address, std::uint8_t value) {
        Logger::log<LOG_DEBUG>("GPIO: write to 0x{0:X} = 0x{1:X}", address, value);

        switch (address) {
            case GPIO_DATA: {
                writePort(value&15);
                break;
            }
            case GPIO_DIRECTION: {
                this->port_dir[0] = static_cast<IOPortDirection>((value>>0)&1);
                this->port_dir[1] = static_cast<IOPortDirection>((value>>1)&1);
                this->port_dir[2] = static_cast<IOPortDirection>((value>>2)&1);
                this->port_dir[3] = static_cast<IOPortDirection>((value>>3)&1);

                Logger::log<LOG_DEBUG>("GPIO: port_dir[0]={0}", this->port_dir[0]);
                Logger::log<LOG_DEBUG>("GPIO: port_dir[1]={0}", this->port_dir[1]);
                Logger::log<LOG_DEBUG>("GPIO: port_dir[2]={0}", this->port_dir[2]);
                Logger::log<LOG_DEBUG>("GPIO: port_dir[3]={0}", this->port_dir[3]);
                break;
            }
            case GPIO_CONTROL: {
                this->allow_reads = value & 1;
                if (this->allow_reads) {
                    Logger::log<LOG_DEBUG>("GPIO: enabled reading");
                }
                else {
                    Logger::log<LOG_DEBUG>("GPIO: disabled reading");
                }
                break;
            }
        }
    }
}