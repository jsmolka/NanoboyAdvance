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
    void GPIO::write(std::uint32_t address, std::uint8_t value) {
        Logger::log<LOG_DEBUG>("GPIO: write to 0x{0:X} = 0x{1:X}", address, value);
    }

    auto GPIO::read(std::uint32_t address) -> std::uint8_t {
        Logger::log<LOG_DEBUG>("GPIO: read from 0x{0:X}", address);
        return 0;
    }
}