#pragma once

#define SUPER_CHIP_8 0

#if SUPER_CHIP_8 == 1
#define OUTPUT_WIDTH 128
#define OUTPUT_HEIGHT 64
#define HEAP_SIZE 4096
#else
#define OUTPUT_WIDTH 64
#define OUTPUT_HEIGHT 32
#define HEAP_SIZE 4096
#endif
#include <array>
#include <map>
#include <stack>
#include <bitset>
#include "QuirkStorage.h"

class Chip
{
public:
    Chip();
    ~Chip();

	void LoadROM(const std::string& filename);
    void Process();
	
    void Fetch();
    uint16_t Decode() const;
    void Execute(uint16_t opcode);
	void DecrementTimers();
    
    uint16_t mProgramCounter = 0x200; // Points to the current instruction in memory.
    uint16_t mIndexRegister = 0; // Stores a memory address used by opcodes.
    std::array<uint8_t, 16> mVariableRegisters = { 0 }; // General purpose variable registers.
    std::array<uint8_t, HEAP_SIZE> mHeap; // First 512 bytes reserved for compatibility.
    std::stack<uint16_t> mStack;

	// External implementation could lerp to new value, giving a CRT-like appearance.
	std::array<uint32_t, OUTPUT_WIDTH * OUTPUT_HEIGHT> mDisplayOutput;

	QuirkStorage mQuirks;
	
	uint16_t mInstruction = 0;
	uint8_t GetX(); // Used to lookup one of the variable registers.
	uint8_t GetY(); // Used to lookup one of the variable registers.
	uint8_t GetN(); // 4-bit immediate number.
	uint8_t GetNN(); // 8-bit immediate number.
	uint16_t GetNNN(); // Immediate memory address.

	// Decrement at 60hz.
	uint8_t mDelayTimer = 0;
	uint8_t mSoundTimer = 0;

	std::array<bool, 16> mKeypad = { 0 };
	
private:
    // Instructions ====================================================================================================
    using ChipInstructionFuncPtr = void (Chip::*)();
	static const std::array<uint16_t, 16> mOpcodeMasks;
    std::map<uint16_t, ChipInstructionFuncPtr> mOpcodeBindings;

	// Base Instruction Set ===================
	void Op_ClearScreen();				// 00E0
	void Op_PopSubroutine();			// 00EE
	void Op_Jump();						// 1NNN
	void Op_PushSubroutine();			// 2NNN
	void Op_SkipIfVxNnEqual();			// 3XNN
	void Op_SkipIfVxNnNotEqual();		// 4XNN
	void Op_SkipIfVxVyEqual();			// 5XY0
	void Op_SetVxToNn();				// 6XNN
	void Op_AddNnToVx();				// 7XNN
	void Op_SetVxToVy();				// 8XY0
	void Op_BinaryOR();					// 8XY1
	void Op_BinaryAND();				// 8XY2
	void Op_LogicalXOR();				// 8XY3
	void Op_AddWithCarry();				// 8XY4
	void Op_SubtractVyFromVx();			// 8XY5
	void Op_SubtractVxfromVy();			// 8XY7
	void Op_ShiftRight();				// 8XY6
	void Op_ShiftLeft();				// 8XYE
	void Op_SkipIfVxVyNotEqual();		// 9XY0
	void Op_SetIndexRegister();			// ANNN
	void Op_JumpWithOffset();			// BNNN
	void Op_Random();					// CXNN
	void Op_SkipIfKeyPressed();			// EX9E
	void Op_SkipIfKeyNotPressed();		// EXA1
	void Op_CacheDelayTimer();			// FX07
	void Op_SetDelayTimer();			// FX15
	void Op_SetSoundTimer();			// FX18
	void Op_AddToIndexRegister();		// FX1E
	void Op_GetKey();					// FX0A
	void Op_SetFontCharacter();			// FX29
	void Op_BinaryToDecimal();			// FX33
	void Op_StoreMemory();				// FX55
	void Op_LoadMemory();				// FX65
	void Op_Draw();						// DXYN
};
