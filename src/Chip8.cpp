#include "Chip8.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

// Font data to be loaded within the reserved portion of memory.
const std::array<uint8_t, 80> gFontData =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

std::mt19937 mRng{ std::random_device{}() };
std::uniform_int_distribution<uint16_t> mRandDist{0, 255};

constexpr std::array<uint16_t, 16> Chip::mOpcodeMasks = {
    0xFFFF, // 0x0
    0xF000, // 0x1
    0xF000, // 0x2
    0xF000, // 0x3
    0xF000, // 0x4
    0xF00F, // 0x5
    0xF000, // 0x6
    0xF000, // 0x7
    0xF00F, // 0x8
    0xF000, // 0x9
    0xF000, // 0xA
    0xF000, // 0xB
    0xF000, // 0xC
    0xF000, // 0xD
    0xF0FF, // 0xE
    0xF0FF  // 0xF
};

Chip::Chip()
    : mOpcodeBindings {
    {0x00E0, &Chip::Op_ClearScreen},
    {0x00EE, &Chip::Op_PopSubroutine},
    {0x1000, &Chip::Op_Jump},
    {0x2000, &Chip::Op_PushSubroutine},
    {0x3000, &Chip::Op_SkipIfVxNnEqual},
    {0x4000, &Chip::Op_SkipIfVxNnNotEqual},
    {0x5000, &Chip::Op_SkipIfVxVyEqual},
    {0x6000, &Chip::Op_SetVxToNn},
    {0x7000, &Chip::Op_AddNnToVx},
    {0x8000, &Chip::Op_SetVxToVy},
    {0x8001, &Chip::Op_BinaryOR},
    {0x8002, &Chip::Op_BinaryAND},
    {0x8003, &Chip::Op_LogicalXOR},
    {0x8004, &Chip::Op_AddWithCarry},
    {0x8005, &Chip::Op_SubtractVyFromVx},
    {0x8006, &Chip::Op_ShiftRight},
    {0x8007, &Chip::Op_SubtractVxfromVy},
    {0x800E, &Chip::Op_ShiftLeft},
    {0x9000, &Chip::Op_SkipIfVxVyNotEqual},
    {0xA000, &Chip::Op_SetIndexRegister},
    {0xB000, &Chip::Op_JumpWithOffset},
    {0xC000, &Chip::Op_Random},
    {0xE09E, &Chip::Op_SkipIfKeyPressed},
    {0xE0A1, &Chip::Op_SkipIfKeyNotPressed},
    {0xF007, &Chip::Op_CacheDelayTimer},
    {0xF015, &Chip::Op_SetDelayTimer},
    {0xF018, &Chip::Op_SetSoundTimer},
    {0xF01E, &Chip::Op_AddToIndexRegister},
    {0xF00A, &Chip::Op_GetKey},
    {0xF029, &Chip::Op_SetFontCharacter},
    {0xF033, &Chip::Op_BinaryToDecimal},
    {0xF055, &Chip::Op_StoreMemory},
    {0xF065, &Chip::Op_LoadMemory},
    {0xD000, &Chip::Op_Draw}
    }
{
    // Load font into memory.
    memcpy(&mHeap[0x50], &gFontData, sizeof(gFontData));
}

Chip::~Chip()
{
}

void Chip::Process()
{
    Fetch();
    Execute(Decode());
}

void Chip::Fetch()
{
    // Shift L part of instruction into the left half, then bitwise with R part.
    mInstruction = (mHeap[mProgramCounter] << 8) | mHeap[mProgramCounter + 1];
    
    // Increment program counter past instruction.
    mProgramCounter += 2;
}

uint16_t Chip::Decode() const
{
    // Determine which opcode to run by applying mask based on the first nibble.
    const uint16_t mask = mOpcodeMasks[mInstruction >> 12];
    return mInstruction & mask;
}

void Chip::Execute(uint16_t opcode)
{
    if (mOpcodeBindings.contains(opcode))
    {
        // Execute instruction.
        (this->*mOpcodeBindings[opcode])();
    }
}

void Chip::DecrementTimers()
{
    if (mDelayTimer > 0) mDelayTimer--;
    if (mSoundTimer > 0) mSoundTimer--;
}

void Chip::LoadROM(const std::string& filename)
{
    // Open the ROM file.
    std::ifstream file(filename, std::ios::binary);
    assert(!file.fail(), "Filepath invalid");

    mQuirks.LoadConfig(filename);
    
    // Read the file into memory, starting at address 0x200
    file.read(reinterpret_cast<char*>(mHeap.data() + mProgramCounter), sizeof(mHeap) - mProgramCounter);
    
    assert(file.gcount() != 0, "Invalid ROM data");
    
    std::cout << "ROM loaded successfully." << std::endl;
}

void Chip::Op_ClearScreen()
{
    std::fill(mDisplayOutput.begin(), mDisplayOutput.end(), 0);
}

void Chip::Op_PopSubroutine()
{
    mProgramCounter = mStack.top();
    mStack.pop();
}

void Chip::Op_Jump()
{
    mProgramCounter = GetNNN();
}

void Chip::Op_PushSubroutine()
{
    mStack.push(mProgramCounter);
    mProgramCounter = GetNNN();
}

void Chip::Op_SkipIfVxNnEqual()
{
    mProgramCounter += (mVariableRegisters[GetX()] == GetNN()) * 2;
}

void Chip::Op_SkipIfVxNnNotEqual()
{
    mProgramCounter += (mVariableRegisters[GetX()] != GetNN()) * 2;
}

void Chip::Op_SkipIfVxVyEqual()
{
    mProgramCounter += (mVariableRegisters[GetX()] == mVariableRegisters[GetY()]) * 2;
}

void Chip::Op_SetVxToNn()
{
    mVariableRegisters[GetX()] = GetNN();
}

void Chip::Op_AddNnToVx()
{
    mVariableRegisters[GetX()] += GetNN();
}

void Chip::Op_SetVxToVy()
{
    mVariableRegisters[GetX()] = mVariableRegisters[GetY()];
}

void Chip::Op_BinaryOR()
{
    mVariableRegisters[GetX()] = mVariableRegisters[GetX()] | mVariableRegisters[GetY()];
}

void Chip::Op_BinaryAND()
{
    mVariableRegisters[GetX()] = mVariableRegisters[GetX()] & mVariableRegisters[GetY()];
}

void Chip::Op_LogicalXOR()
{
    mVariableRegisters[GetX()] = mVariableRegisters[GetX()] ^ mVariableRegisters[GetY()];
}

void Chip::Op_AddWithCarry()
{
    const uint8_t x = GetX();
    const uint16_t sum = mVariableRegisters[x] + mVariableRegisters[GetY()];
    mVariableRegisters[x] = static_cast<uint8_t>(sum);
    mVariableRegisters[0xF] = sum >> 8; // 1 if carry, 0 if not
}

void Chip::Op_SubtractVyFromVx()
{
    uint8_t& vx = mVariableRegisters[GetX()];
    const uint8_t& vy = mVariableRegisters[GetY()];

    vx = vx - vy;
    mVariableRegisters[0xF] = static_cast<uint8_t>(vx >= vy);
}

void Chip::Op_SubtractVxfromVy()
{
    uint8_t& vx = mVariableRegisters[GetX()];
    const uint8_t& vy = mVariableRegisters[GetY()];

    vx = vy - vx;
    mVariableRegisters[0xF] = static_cast<uint8_t>(vy >= vx);
}

void Chip::Op_ShiftRight()
{
    const uint8_t x = GetX();
    uint8_t value = mQuirks.mModernShift ? mVariableRegisters[x] : mVariableRegisters[GetY()];
    mVariableRegisters[0xF] = value & 0x01;
    mVariableRegisters[x] = value >> 1;
}

void Chip::Op_ShiftLeft()
{
    const uint8_t x = GetX();
    uint8_t value = mQuirks.mModernShift ? mVariableRegisters[x] : mVariableRegisters[GetY()];
    mVariableRegisters[0xF] = (value >> 7) & 0x01;
    mVariableRegisters[x] = value << 1;
}

void Chip::Op_SkipIfVxVyNotEqual()
{
    mProgramCounter += (mVariableRegisters[GetX()] != mVariableRegisters[GetY()]) * 2;
}

void Chip::Op_SetIndexRegister()
{
    mIndexRegister = GetNNN();
}

void Chip::Op_JumpWithOffset()
{
    if (mQuirks.mSuperChipJump) // SUPER-CHIP style: XNN + VX
    {
        const uint8_t x = GetX();
        mProgramCounter = (x << 8) | GetNN();
        mProgramCounter += mVariableRegisters[x];
    }
    else
    {
        mProgramCounter = GetNNN() + mVariableRegisters[0];   
    }
}

void Chip::Op_Random()
{
    const uint8_t random = static_cast<uint8_t>(mRandDist(mRng));
    mVariableRegisters[GetX()] = random & GetNN();
}

void Chip::Op_SkipIfKeyPressed()
{
    mProgramCounter += (mKeypad[mVariableRegisters[GetX()]] * 2);
}

void Chip::Op_SkipIfKeyNotPressed()
{
    mProgramCounter += (!mKeypad[mVariableRegisters[GetX()]] * 2);
}

void Chip::Op_CacheDelayTimer()
{
	mVariableRegisters[GetX()] = mDelayTimer;
}

void Chip::Op_SetDelayTimer()
{
    mDelayTimer = mVariableRegisters[GetX()];
}

void Chip::Op_SetSoundTimer()
{
	mSoundTimer = mVariableRegisters[GetX()];
}

void Chip::Op_AddToIndexRegister()
{
    mIndexRegister += mVariableRegisters[GetX()];
}

void Chip::Op_GetKey()
{
    for (int i = 0; i < mKeypad.size(); ++i)
    {
        if (mKeypad[i])
        {
            // A key is pressed — store its value and continue execution
            mVariableRegisters[GetX()] = i;
            return;
        }
    }

    // No key was pressed — repeat this instruction on next cycle
    mProgramCounter -= 2;
}

void Chip::Op_SetFontCharacter()
{
    const uint8_t characterIndex = mVariableRegisters[GetX()] * 5;
    mIndexRegister = mHeap[0x50 + characterIndex];
}

void Chip::Op_BinaryToDecimal()
{
    const uint8_t value = mVariableRegisters[GetX()];

    mHeap[mIndexRegister + 0] = value / 100;             // XXX
    mHeap[mIndexRegister + 1] = (value / 10) % 10;       // XX
    mHeap[mIndexRegister + 2] = value % 10;              // X
}

void Chip::Op_StoreMemory()
{
    const uint8_t x = GetX();
    for (uint8_t i = 0; i <= x; ++i)
    {
        mHeap[mIndexRegister + i] = mVariableRegisters[i];
    }
    
    if (!mQuirks.mModernLoadStore)
    {
        mIndexRegister += x + 1;
    }
}

void Chip::Op_LoadMemory()
{
    const uint8_t x = GetX();
    for (uint8_t i = 0; i <= x; ++i)
    {
        mVariableRegisters[i] = mHeap[mIndexRegister + i];
    }
    
    if (!mQuirks.mModernLoadStore)
    {
        mIndexRegister += x + 1;
    }
}

void Chip::Op_Draw()
{
    // Bitwise AND for wrapping.
    // Removing most significant bit from mask, cheap alternative to mod that only supports power of two resolutions. 
    uint8_t startX = mVariableRegisters[GetX()] & (OUTPUT_WIDTH - 1);
    uint8_t startY = mVariableRegisters[GetY()] & (OUTPUT_HEIGHT - 1);
    const uint8_t n = GetN();
    
    // Reset VF flag.
    mVariableRegisters[0xF] = 0;
    
    for (uint8_t row = 0; row < n; row++)
    {
        // Boundary Check Y ============================================================================================
        const uint8_t currentY = startY + row;
        if (currentY >= OUTPUT_HEIGHT) { break; }
            
        const uint8_t spriteByte = mHeap[mIndexRegister + row];
        
        // Loop through each pixel in the sprite row
        for (uint8_t col = 0; col < 8; col++)
        {
            // Boundary Check X ========================================================================================
            const uint8_t currentX = startX + col;
            if (currentX >= OUTPUT_WIDTH) { break; }

            // Isolate the desired pixel bit by shifting into the first position & masking it.
            const uint8_t pixel = (spriteByte >> (7 - col)) & 0x1;
            
            // Convert from (0 : 1) to (0 : 0xFFFFFFFF) using the two's complement negation trick: (-0 = 0, -1 = 0xFFFFFFFF)
            // "Hey integer promotion, think you dropped this 👑"
            const uint32_t spritePixel = static_cast<uint32_t>(-pixel);
            uint32_t& displayPixel = mDisplayOutput[currentY * OUTPUT_WIDTH + currentX];

            // Detect collision & shift, non-zero only if sprite & screen are both on.
            mVariableRegisters[0xF] |= static_cast<uint8_t>((spritePixel & displayPixel) >> 31);

            // Finally, apply sprite pixel to display using XOR.
            displayPixel = displayPixel ^ spritePixel;
        }
    }
}

uint8_t Chip::GetX()
{
    return (mInstruction >> 8) & 0x0F; // 2nd nibble
}

uint8_t Chip::GetY()
{
    return (mInstruction >> 4) & 0x0F; // 3rd nibble
}

uint8_t Chip::GetN()
{
    return mInstruction & 0x0F; // 4th nibble
}

uint8_t Chip::GetNN()
{
    return mInstruction & 0xFF; // second byte.
}

uint16_t Chip::GetNNN()
{
    return mInstruction & 0xFFF; // 2nd, 3rd & 4th nibbles.
}