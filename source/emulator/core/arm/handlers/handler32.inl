/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

enum class DataOp {
  AND = 0,
  EOR = 1,
  SUB = 2,
  RSB = 3,
  ADD = 4,
  ADC = 5,
  SBC = 6,
  RSC = 7,
  TST = 8,
  TEQ = 9,
  CMP = 10,
  CMN = 11,
  ORR = 12,
  MOV = 13,
  BIC = 14,
  MVN = 15
};

template <bool immediate, DataOp opcode, bool set_flags, int field4>
void ARM_DataProcessing(std::uint32_t instruction) {
  int reg_dst = (instruction >> 12) & 0xF;
  int reg_op1 = (instruction >> 16) & 0xF;
  int reg_op2 = (instruction >>  0) & 0xF;

  std::uint32_t op2 = 0;
  std::uint32_t op1 = state.reg[reg_op1];

  int carry = state.cpsr.f.c;

  pipe.fetch_type = Access::Sequential;

  if constexpr (immediate) {
    int value = instruction & 0xFF;
    int shift = ((instruction >> 8) & 0xF) * 2;

    if (shift != 0) {
      carry = (value >> (shift - 1)) & 1;
      op2   = (value >> shift) | (value << (32 - shift));
    } else {
      op2 = value;
    }
  } else {
    constexpr int  shift_type = ( field4 >> 1) & 3;
    constexpr bool shift_imm  = (~field4 >> 0) & 1;

    std::uint32_t shift;

    op2 = state.reg[reg_op2];

    if constexpr (shift_imm) {
      shift = (instruction >> 7) & 0x1F;
    } else {
      shift = state.reg[(instruction >> 8) & 0xF];

      if (reg_op1 == 15) op1 += 4;
      if (reg_op2 == 15) op2 += 4;
    }

    DoShift(shift_type, op2, shift, carry, shift_imm);
  }

  auto& cpsr = state.cpsr;
  auto& result = state.reg[reg_dst];

  switch (opcode) {
    case DataOp::AND:
      result = op1 & op2;
      if constexpr (set_flags) {
        SetZeroAndSignFlag(result);
        cpsr.f.c = carry;
      }
      break;
    case DataOp::EOR:
      result = op1 ^ op2;
      if constexpr (set_flags) {
        SetZeroAndSignFlag(result);
        cpsr.f.c = carry;
      }
      break;
    case DataOp::SUB:
      result = SUB(op1, op2, set_flags);
      break;
    case DataOp::RSB:
      result = SUB(op2, op1, set_flags);
      break;
    case DataOp::ADD:
      result = ADD(op1, op2, set_flags);
      break;
    case DataOp::ADC:
      result = ADC(op1, op2, set_flags);
      break;
    case DataOp::SBC:
      result = SBC(op1, op2, set_flags);
      break;
    case DataOp::RSC:
      result = SBC(op2, op1, set_flags);
      break;
    case DataOp::TST:
      SetZeroAndSignFlag(op1 & op2);
      cpsr.f.c = carry;
      break;
    case DataOp::TEQ:
      SetZeroAndSignFlag(op1 ^ op2);
      cpsr.f.c = carry;
      break;
    case DataOp::CMP:
      SUB(op1, op2, true);
      break;
    case DataOp::CMN:
      ADD(op1, op2, true);
      break;
    case DataOp::ORR:
      result = op1 | op2;
      if (set_flags) {
        SetZeroAndSignFlag(result);
        cpsr.f.c = carry;
      }
      break;
    case DataOp::MOV:
      result = op2;
      if constexpr (set_flags) {
        SetZeroAndSignFlag(result);
        cpsr.f.c = carry;
      }
      break;
    case DataOp::BIC:
      result = op1 & ~op2;
      if constexpr (set_flags) {
        SetZeroAndSignFlag(result);
        cpsr.f.c = carry;
      }
      break;
    case DataOp::MVN:
      result = ~op2;
      if constexpr (set_flags) {
        SetZeroAndSignFlag(result);
        cpsr.f.c = carry;
      }
      break;
  }

  if (reg_dst == 15) {
    if constexpr (set_flags) {
      auto spsr = *p_spsr;

      SwitchMode(spsr.f.mode);
      state.cpsr.v = spsr.v;
    }

    if constexpr (opcode != DataOp::TST &&
                  opcode != DataOp::TEQ &&
                  opcode != DataOp::CMP &&
                  opcode != DataOp::CMN) {
      if (state.cpsr.f.thumb) {
        ReloadPipeline16();
      } else {
        ReloadPipeline32();
      }
    }
  } else {
    state.r15 += 4;
  }
}

template <bool immediate, bool use_spsr, bool to_status>
void ARM_StatusTransfer(std::uint32_t instruction) {
  if (to_status) {
    /* TODO: find out what happens if the thumb bit was altered by MSR. */
    std::uint32_t op;
    std::uint32_t mask = 0;

    /* Create mask based on fsxc-bits. */
    if (instruction & (1 << 16)) mask |= 0x000000FF;
    if (instruction & (1 << 17)) mask |= 0x0000FF00;
    if (instruction & (1 << 18)) mask |= 0x00FF0000;
    if (instruction & (1 << 19)) mask |= 0xFF000000;

    /* Decode source operand. */
    if (immediate) {
      int value = instruction & 0xFF;
      int shift = ((instruction >> 8) & 0xF) * 2;

      op = (value >> shift) | (value << (32 - shift));
    } else {
      op = state.reg[instruction & 0xF];
    }

    std::uint32_t value = op & mask;

    /* Apply masked replace to SPSR or CPSR. */
    if (!use_spsr) {
      if (mask & 0xFF) {
        SwitchMode(static_cast<Mode>(value & 0x1F));
      }
      state.cpsr.v = (state.cpsr.v & ~mask) | value;
    } else if (p_spsr != &state.cpsr) {
      p_spsr->v = (p_spsr->v & ~mask) | value;
    }
  } else {
    int dst = (instruction >> 12) & 0xF;

    if (use_spsr) {
      state.reg[dst] = p_spsr->v;
    } else {
      state.reg[dst] = state.cpsr.v;
    }
  }

  pipe.fetch_type = Access::Sequential;
  state.r15 += 4;
}

template <bool accumulate, bool set_flags>
void ARM_Multiply(std::uint32_t instruction) {
  std::uint32_t result;

  int op1 = (instruction >>  0) & 0xF;
  int op2 = (instruction >>  8) & 0xF;
  int op3 = (instruction >> 12) & 0xF;
  int dst = (instruction >> 16) & 0xF;

  TickMultiply(state.reg[op2]);

  result = state.reg[op1] * state.reg[op2];

  if (accumulate) {
    result += state.reg[op3];
    interface->Idle();
  }

  if (set_flags) {
    SetZeroAndSignFlag(result);
  }

  state.reg[dst] = result;
  pipe.fetch_type = Access::Nonsequential;
  state.r15 += 4;
}

template <bool sign_extend, bool accumulate, bool set_flags>
void ARM_MultiplyLong(std::uint32_t instruction) {
  int op1 = (instruction >> 0) & 0xF;
  int op2 = (instruction >> 8) & 0xF;

  int dst_lo = (instruction >> 12) & 0xF;
  int dst_hi = (instruction >> 16) & 0xF;

  std::int64_t result;

  interface->Idle();
  TickMultiply(state.reg[op2]);

  if (sign_extend) {
    std::int64_t a = state.reg[op1];
    std::int64_t b = state.reg[op2];

    /* Sign-extend operands */
    if (a & 0x80000000) a |= 0xFFFFFFFF00000000;
    if (b & 0x80000000) b |= 0xFFFFFFFF00000000;

    result = a * b;
  } else {
    std::uint64_t uresult = (std::uint64_t)state.reg[op1] * (std::uint64_t)state.reg[op2];

    result = (std::int64_t)uresult;
  }

  if (accumulate) {
    std::int64_t value = state.reg[dst_hi];

    /* Workaround x86 shift limitations. */
    value <<= 16;
    value <<= 16;
    value  |= state.reg[dst_lo];

    result += value;
    interface->Idle();
  }

  std::uint32_t result_hi = result >> 32;

  state.reg[dst_lo] = result & 0xFFFFFFFF;
  state.reg[dst_hi] = result_hi;

  if (set_flags) {
    state.cpsr.f.n = result_hi >> 31;
    state.cpsr.f.z = result == 0;
  }

  pipe.fetch_type = Access::Nonsequential;
  state.r15 += 4;
}

/* TODO: Check if timings are correct. */
template <bool byte>
void ARM_SingleDataSwap(std::uint32_t instruction) {
  int src  = (instruction >>  0) & 0xF;
  int dst  = (instruction >> 12) & 0xF;
  int base = (instruction >> 16) & 0xF;

  std::uint32_t tmp;

  if (byte) {
    tmp = ReadByte(state.reg[base], Access::Nonsequential);
    WriteByte(state.reg[base], (std::uint8_t)state.reg[src], Access::Nonsequential);
  } else {
    tmp = ReadWordRotate(state.reg[base], Access::Nonsequential);
    WriteWord(state.reg[base], state.reg[src], Access::Nonsequential);
  }

  state.reg[dst] = tmp;
  interface->Idle();
  pipe.fetch_type = Access::Nonsequential;
  state.r15 += 4;
}

void ARM_BranchAndExchange(std::uint32_t instruction) {
  std::uint32_t address = state.reg[instruction & 0xF];

  if (address & 1) {
    state.r15 = address & ~1;
    state.cpsr.f.thumb = 1;
    ReloadPipeline16();
  } else {
    state.r15 = address & ~3;
    ReloadPipeline32();
  }
}

template <bool pre, bool add, bool immediate, bool writeback, bool load, int opcode>
void ARM_HalfwordSignedTransfer(std::uint32_t instruction) {
  int dst  = (instruction >> 12) & 0xF;
  int base = (instruction >> 16) & 0xF;

  std::uint32_t offset;
  std::uint32_t address = state.reg[base];

  if (immediate) {
    offset = (instruction & 0xF) | ((instruction >> 4) & 0xF0);
  } else {
    offset = state.reg[instruction & 0xF];
  }

  if (pre) {
    address += add ? offset : -offset;
  }

  switch (opcode) {
    case 0: break;
    case 1:
      if (load) {
        state.reg[dst] = ReadHalfRotate(address, Access::Nonsequential);
        interface->Idle();
      } else {
        std::uint32_t value = state.reg[dst];

        if (dst == 15) {
          value += 4;
        }

        WriteHalf(address, value, Access::Nonsequential);
      }
      break;
    case 2:
      if (load) {
        state.reg[dst] = ReadByteSigned(address, Access::Nonsequential);
        interface->Idle();
      } else {
        // ARMv5 LDRD: this opcode is unpredictable on ARMv4T.
        // On ARM7TDMI-S it doesn't seem to perform any memory access,
        // so the load/store cycle probably is internal in this case.
        interface->Idle();
        interface->Idle();
      }
      break;
    case 3:
      if (load) {
        state.reg[dst] = ReadHalfSigned(address, Access::Nonsequential);
        interface->Idle();
      } else {
        // ARMv5 STRD: this opcode is unpredictable on ARMv4T.
        // On ARM7TDMI-S it doesn't seem to perform any memory access,
        // so the load/store cycle probably is internal in this case.
        interface->Idle();
      }
      break;
  }

  if ((writeback || !pre) && (!load || base != dst)) {
    if (!pre) {
      address += add ? offset : -offset;
    }
    state.reg[base] = address;
  }

  if (load && dst == 15) {
    ReloadPipeline32();
  } else {
    pipe.fetch_type = Access::Nonsequential;
    state.r15 += 4;
  }
}

template <bool link>
void ARM_BranchAndLink(std::uint32_t instruction) {
  std::uint32_t offset = instruction & 0xFFFFFF;

  if (offset & 0x800000) {
    offset |= 0xFF000000;
  }

  if (link) {
    state.r14 = state.r15 - 4;
  }

  state.r15 += offset * 4;
  ReloadPipeline32();
}

template <bool immediate, bool pre, bool add, bool byte, bool writeback, bool load>
void ARM_SingleDataTransfer(std::uint32_t instruction) {
  /* TODO: reverse-engineer special case with usermode registers and a banked base register. */
  Mode mode;
  std::uint32_t offset;

  int dst  = (instruction >> 12) & 0xF;
  int base = (instruction >> 16) & 0xF;
  std::uint32_t address = state.reg[base];

  /* Post-indexing implicitly write back to the base register.
   * In that case user mode registers will be used if the W-bit is set.
   */
  if (!pre && writeback) {
    mode = static_cast<Mode>(state.cpsr.f.mode);
    SwitchMode(MODE_USR);
  }

  /* Calculate offset relative to base register. */
  if (immediate) {
    offset = instruction & 0xFFF;
  } else {
    int carry  = state.cpsr.f.c;
    int opcode = (instruction >> 5) & 3;
    int amount = (instruction >> 7) & 0x1F;

    offset = state.reg[instruction & 0xF];
    DoShift(opcode, offset, amount, carry, true);
  }

  if (pre) {
    address += add ? offset : -offset;
  }

  if (load) {
    if (byte) {
      state.reg[dst] = ReadByte(address, Access::Nonsequential);
    } else {
      state.reg[dst] = ReadWordRotate(address, Access::Nonsequential);
    }
    interface->Idle();
  } else {
    std::uint32_t value = state.reg[dst];

    /* r15 is $+12 now due to internal prefetch cycle. */
    if (dst == 15) value += 4;

    if (byte) {
      WriteByte(address, (std::uint8_t)value, Access::Nonsequential);
    } else {
      WriteWord(address, value, Access::Nonsequential);
    }
  }

  /* Restore original mode (if it was changed) */
  if (!pre && writeback) {
    SwitchMode(mode);
  }

  /* Write address back to the base register. */
  if (!load || base != dst) {
    if (!pre) {
      state.reg[base] += add ? offset : -offset;
    } else if (writeback) {
      state.reg[base] = address;
    }
  }

  if (load && dst == 15) {
    ReloadPipeline32();
  } else {
    pipe.fetch_type = Access::Nonsequential;
    state.r15 += 4;
  }
}

template <bool _pre, bool add, bool user_mode, bool writeback, bool load>
void ARM_BlockDataTransfer(std::uint32_t instruction) {
  /* TODO: reverse-engineer special case with usermode registers and a banked base register. */
  int base = (instruction >> 16) & 0xF;
  int list = instruction & 0xFFFF;

  Mode mode;
  bool transfer_pc = list & (1 << 15);
  bool switch_mode = user_mode && (!load || !transfer_pc);
  int  first = 0;
  int  bytes = 0;
  bool pre = _pre;

  std::uint32_t address = state.reg[base];

  if (switch_mode) {
    mode = state.cpsr.f.mode;
    SwitchMode(MODE_USR);
  }

  if (list != 0) {
    /* Determine number of bytes to transfer and the first register in the list. */
    for (int i = 15; i >= 0; i--) {
      if (~list & (1 << i)) {
        continue;
      }
      first = i;
      bytes += 4;
    }
  } else {
    /* If the register list is empty, only r15 will be loaded/stored but
     * the base will be incremented/decremented as if each register was transferred.
     */
    list  = 1 << 15;
    first = 15;
    transfer_pc = true;
    bytes = 64;
  }

  std::uint32_t base_new = address;
  std::uint32_t base_old = address;

  /* The CPU always transfers registers sequentially,
   * for decrementing modes it determines the final address first and
   * then transfers the registers with incrementing addresses.
   * Due to the inverted order post-decrement becomes pre-increment and
   * pre-decrement becomes post-increment.
   */
  if (!add) {
    pre = !pre;
    address  -= bytes;
    base_new -= bytes;
  } else {
    base_new += bytes;
  }

  auto access_type = Access::Nonsequential;

  for (int i = first; i < 16; i++) {
    if (~list & (1 << i)) {
      continue;
    }

    if (pre) {
      address += 4;
    }

    if (load) {
      state.reg[i] = ReadWord(address, access_type);
      if (i == 15 && user_mode) {
        auto& spsr = *p_spsr;

        SwitchMode(spsr.f.mode);
        state.cpsr.v = spsr.v;
      }
    } else if (i == base) {
      WriteWord(address, (i == first) ? base_old : base_new, access_type);
    } else if (i == 15) {
      /* In hardware r15 is incremented in the cycle before the first transfer. */
      WriteWord(address, state.r15 + 4, access_type);
    } else {
      WriteWord(address, state.reg[i], access_type);
    }

    if (!pre) {
      address += 4;
    }

    access_type = Access::Sequential;
  }

  if (switch_mode) {
    SwitchMode(mode);
  }

  if (writeback && (!load || !(list & (1 << base)))) {
    state.reg[base] = base_new;
  }

  if (load) {
    interface->Idle();
  }

  if (load && transfer_pc) {
    if (state.cpsr.f.thumb) {
      ReloadPipeline16();
    } else {
      ReloadPipeline32();
    }
  } else {
    pipe.fetch_type = Access::Nonsequential;
    state.r15 += 4;
  }
}

void ARM_Undefined(std::uint32_t instruction) {
  /* Save return address and program status. */
  state.bank[BANK_UND][BANK_R14] = state.r15 - 4;
  state.spsr[BANK_UND].v = state.cpsr.v;

  /* Switch to UND mode and disable interrupts. */
  SwitchMode(MODE_UND);
  state.cpsr.f.mask_irq = 1;

  /* Jump to execution vector */
  state.r15 = 0x04;
  ReloadPipeline32();
}

void ARM_SWI(std::uint32_t instruction) {
  /* Save return address and program status. */
  state.bank[BANK_SVC][BANK_R14] = state.r15 - 4;
  state.spsr[BANK_SVC].v = state.cpsr.v;

  /* Switch to SVC mode and disable interrupts. */
  SwitchMode(MODE_SVC);
  state.cpsr.f.mask_irq = 1;

  /* Jump to execution vector */
  state.r15 = 0x08;
  ReloadPipeline32();
}

template <std::uint32_t instruction>
static constexpr auto GenerateHandlerARM() -> Handler32 {
    const std::uint32_t opcode = instruction & 0x0FFFFFFF;

    const bool pre = instruction & (1 << 24);
    const bool add = instruction & (1 << 23);
    const bool wb = instruction & (1 << 21);
    const bool load = instruction & (1 << 20);

    switch (opcode >> 26) {
    case 0b00:
        if (opcode & (1 << 25)) {
            // ARM.8 Data processing and PSR transfer ... immediate
            const bool set_flags = instruction & (1 << 20);
            const int  opcode = (instruction >> 21) & 0xF;

            if (!set_flags && opcode >= 0b1000 && opcode <= 0b1011) {
                const bool use_spsr = instruction & (1 << 22);
                const bool to_status = instruction & (1 << 21);

                return &ARM7TDMI::ARM_StatusTransfer<true, use_spsr, to_status>;
            } else {
                const int field4 = (instruction >> 4) & 0xF;

                return &ARM7TDMI::ARM_DataProcessing<true, static_cast<ARM7TDMI::DataOp>(opcode), set_flags, field4>;
            }
        } else if ((opcode & 0xFF000F0) == 0x1200010) {
            // ARM.3 Branch and exchange
            // TODO: Some bad instructions might be falsely detected as BX.
            // How does HW handle this?
            return &ARM7TDMI::ARM_BranchAndExchange;
        } else if ((opcode & 0x10000F0) == 0x0000090) {
            // ARM.1 Multiply (accumulate), ARM.2 Multiply (accumulate) long
            const bool accumulate = instruction & (1 << 21);
            const bool set_flags = instruction & (1 << 20);

            if (opcode & (1 << 23)) {
                const bool sign_extend = instruction & (1 << 22);

                return &ARM7TDMI::ARM_MultiplyLong<sign_extend, accumulate, set_flags>;
            } else {
                return &ARM7TDMI::ARM_Multiply<accumulate, set_flags>;
            }
        } else if ((opcode & 0x10000F0) == 0x1000090) {
            // ARM.4 Single data swap
            const bool byte = instruction & (1 << 22);

            return &ARM7TDMI::ARM_SingleDataSwap<byte>;
        } else if ((opcode & 0xF0) == 0xB0 ||
            (opcode & 0xD0) == 0xD0) {
            // ARM.5 Halfword data transfer, register offset
            // ARM.6 Halfword data transfer, immediate offset
            // ARM.7 Signed data transfer (byte/halfword)
            const bool immediate = instruction & (1 << 22);
            const int opcode = (instruction >> 5) & 3;

            return &ARM7TDMI::ARM_HalfwordSignedTransfer<pre, add, immediate, wb, load, opcode>;
        } else {
            // ARM.8 Data processing and PSR transfer
            const bool set_flags = instruction & (1 << 20);
            const int  opcode = (instruction >> 21) & 0xF;

            if (!set_flags && opcode >= 0b1000 && opcode <= 0b1011) {
                const bool use_spsr = instruction & (1 << 22);
                const bool to_status = instruction & (1 << 21);

                return &ARM7TDMI::ARM_StatusTransfer<false, use_spsr, to_status>;
            } else {
                const int field4 = (instruction >> 4) & 0xF;

                return &ARM7TDMI::ARM_DataProcessing<false, static_cast<ARM7TDMI::DataOp>(opcode), set_flags, field4>;
            }
        }
        break;
    case 0b01:
        // ARM.9 Single data transfer, ARM.10 Undefined
        if ((opcode & 0x2000010) == 0x2000010) {
            return &ARM7TDMI::ARM_Undefined;
        } else {
            const bool immediate = ~instruction & (1 << 25);
            const bool byte = instruction & (1 << 22);

            return &ARM7TDMI::ARM_SingleDataTransfer<immediate, pre, add, byte, wb, load>;
        }
        break;
    case 0b10:
        // ARM.11 Block data transfer, ARM.12 Branch
        if (opcode & (1 << 25)) {
            return &ARM7TDMI::ARM_BranchAndLink<(opcode >> 24) & 1>;
        } else {
            const bool user_mode = instruction & (1 << 22);

            return &ARM7TDMI::ARM_BlockDataTransfer<pre, add, user_mode, wb, load>;
        }
        break;
    case 0b11:
        if (opcode & (1 << 25)) {
            if (opcode & (1 << 24)) {
                // ARM.16 Software interrupt
                return &ARM7TDMI::ARM_SWI;
            } else {
                // ARM.14 Coprocessor data operation
                // ARM.15 Coprocessor register transfer
            }
        } else {
            // ARM.13 Coprocessor data transfer
        }
        break;
    }

    return &ARM7TDMI::ARM_Undefined;
}

#define DECODE0001(hash) GenerateHandlerARM<((hash & 0xFF0) << 16) | ((hash & 0xF) << 4)>(),
#define DECODE0004(hash) DECODE0001(hash + 0 *    1) DECODE0001(hash + 1 *    1) DECODE0001(hash + 2 *    1) DECODE0001(hash + 3 *    1)
#define DECODE0016(hash) DECODE0004(hash + 0 *    4) DECODE0004(hash + 1 *    4) DECODE0004(hash + 2 *    4) DECODE0004(hash + 3 *    4)
#define DECODE0064(hash) DECODE0016(hash + 0 *   16) DECODE0016(hash + 1 *   16) DECODE0016(hash + 2 *   16) DECODE0016(hash + 3 *   16)
#define DECODE0256(hash) DECODE0064(hash + 0 *   64) DECODE0064(hash + 1 *   64) DECODE0064(hash + 2 *   64) DECODE0064(hash + 3 *   64)
#define DECODE1024(hash) DECODE0256(hash + 0 *  256) DECODE0256(hash + 1 *  256) DECODE0256(hash + 2 *  256) DECODE0256(hash + 3 *  256)
#define DECODE4096(hash) DECODE1024(hash + 0 * 1024) DECODE1024(hash + 1 * 1024) DECODE1024(hash + 2 * 1024) DECODE1024(hash + 3 * 1024)

inline static std::array<Handler32, 4096> ARM7TDMI::s_opcode_lut_32 = { DECODE4096(0) };

#undef DECODE0001
#undef DECODE0004
#undef DECODE0016
#undef DECODE0064
#undef DECODE0256
#undef DECODE1024
#undef DECODE4096
