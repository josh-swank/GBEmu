#include "stdafx.hpp"

#include "gameboy.hpp"

#include "util.hpp"

// TODO: REMOVE
#define REG_AF_ registers_[0]
#define REG_AF REG_AF_.word
#define REG_BC_ registers_[1]
#define REG_BC REG_BC_.word
#define REG_DE_ registers_[2]
#define REG_DE REG_DE_.word
#define REG_HL_ registers_[3]
#define REG_HL REG_HL_.word
#define REG_SP_ registers_[4]
#define REG_SP REG_SP_.word
#define REG_PC_ registers_[5]
#define REG_PC REG_PC_.word

#define REG_A REG_AF_.bytes[0]
#define REG_F REG_AF_.bytes[1]
#define REG_B REG_BC_.bytes[0]
#define REG_C REG_BC_.bytes[1]
#define REG_D REG_DE_.bytes[0]
#define REG_E REG_DE_.bytes[1]
#define REG_H REG_HL_.bytes[0]
#define REG_L REG_HL_.bytes[1]

const uint8_t
	kZeroFlag = 0b10000000,
	kSubtractFlag = 0b01000000,
	kHalfCarryFlag = 0b00100000,
	kCarryFlag = 0b00010000;

const double
	GBCPU::kGBClockFreq = 4190000,
	GBCPU::kGBClockPeriod = (1.0f / kGBClockFreq) * 1000000000;

GBCPU::GBCPU(const uint8_t* boot_rom, GBConnectors& conns, GBGPU& gpu)
	: mem_(boot_rom, conns, gpu) {}

void GBCPU::Start() {
	REG_PC = 0;
}

void GBCPU::DoMachineCycle() {
	if (optime_ == 0) {
		opcode_ = Next8_();
		optime_ += Exec_(opcode_);
	}
	--optime_;
}

void GBCPU::Push16_(uint16_t val) {
	REG_SP -= 2;
	mem_.Set16(REG_SP, val);
}

uint16_t GBCPU::Pop16_() {
	auto val = mem_.Get16(REG_SP);
	REG_SP += 2;
	return val;
}

uint8_t GBCPU::Next8_()
{
	auto value = mem_.Get8(REG_PC);
	REG_PC += 1;
	return value;
}

uint16_t GBCPU::Next16_()
{
	auto value = mem_.Get16(REG_PC);
	REG_PC += 2;
	return value;
}

unsigned int GBCPU::BIT_(uint8_t pos, uint8_t op) {
	REG_A = op & (1 << pos);
	REG_F = (REG_A == 0 ? kZeroFlag : 0) + kHalfCarryFlag + (REG_F & kCarryFlag);
	return 2;
}

unsigned int GBCPU::CALL_D16_() {
	auto calladdr = Next16_();
	Push16_(REG_PC);
	REG_PC = calladdr;
	return 6;
}

unsigned int GBCPU::EI_() {
	// TODO
#warning TODO
	return 1;
}

unsigned int GBCPU::INC_R8_(uint8_t& op) {
	++op;
	REG_F = (REG_A == 0 ? kZeroFlag : 0) + (op & 0x00FF == 0 ? kHalfCarryFlag : 0) + (REG_F & kCarryFlag);
	return 1;
}

unsigned int GBCPU::JR_AR8_() {
	int8_t reladdr;
	auto next = Next8_();
	std::memcpy(&reladdr, &next, 1);

	REG_PC = ((int32_t)REG_PC) + reladdr;
	return 3;
}

unsigned int GBCPU::JR_Cond_AR8_(bool cond) {
	if (cond)
		return JR_AR8_();
	else
		return 2;
}

unsigned int GBCPU::LD_AD8_R8_(uint8_t src) {
	mem_.Set8(0xFF00 + Next8_(), src);
	return 3;
}

unsigned int GBCPU::LD_AR8_R8_(uint8_t destAddr, uint8_t src) {
	mem_.Set8(0xFF00 + destAddr, src);
	return 2;
}

unsigned int GBCPU::LD_AR16_R8_(uint16_t destAddr, uint8_t src) {
	mem_.Set8(destAddr, src);
	return 2;
}

unsigned int GBCPU::LD_R8_AR16_(uint8_t& dest, uint16_t srcAddr) {
	dest = mem_.Get8(srcAddr);
	return 2;
}

unsigned int GBCPU::LD_R8_D8_(uint8_t& dest) {
	dest = Next8_();
	return 2;
}

unsigned int GBCPU::LD_R16_D16_(uint16_t& dest) {
	dest = Next16_();
	return 3;
}

unsigned int GBCPU::LDD_AR16_D8_(uint16_t& destAddr, uint8_t src) {
	mem_.Set8(destAddr, src);
	--destAddr;
	return 2;
}

unsigned int GBCPU::LDD_D8_AR16_(uint8_t& dest, uint16_t& srcAddr) {
	dest = mem_.Get8(srcAddr);
	--srcAddr;
	return 2;
}

unsigned int GBCPU::LDI_AR16_D8_(uint16_t& destAddr, uint8_t src) {
	mem_.Set8(destAddr, src);
	++destAddr;
	return 2;
}

unsigned int GBCPU::LDI_D8_AR16_(uint8_t& dest, uint16_t& srcAddr) {
	dest = mem_.Get8(srcAddr);
	++srcAddr;
	return 2;
}

unsigned int GBCPU::NOP_() {
	return 1;
}

unsigned int GBCPU::XOR_R8_(uint8_t op) {
	REG_A ^= op;
	REG_F = (REG_A == 0 ? kZeroFlag : 0);
	return 1;
}

unsigned int GBCPU::CB_Prefix_() {
	auto opcode = Next8_();
	BOOST_LOG_TRIVIAL(debug) << ConcatString("Executing opcode: 0xcb 0x", std::hex, +opcode);

	switch (opcode) {
	case 0x7c: return BIT_(7, REG_H);
	default:
		throw std::runtime_error(ConcatString("Unknown opcode: 0xcb 0x", std::hex, +opcode));
	}
	return 2;
}

unsigned int GBCPU::Exec_(uint8_t opcode) {
	BOOST_LOG_TRIVIAL(debug) << ConcatString("Executing opcode: 0x", std::hex, +opcode);

	switch (opcode) {
		// CALL
	case 0xCD: return CALL_D16_();

		// EI
	case 0xFB: return EI_();

		// INC
	case 0x0C: return INC_R8_(REG_C);
	case 0x1C: return INC_R8_(REG_E);
	case 0x2C: return INC_R8_(REG_L);
	case 0x3C: return INC_R8_(REG_A);

		// JR
	case 0x20: return JR_Cond_AR8_((REG_F & kZeroFlag) == 0);

		// LD
	case 0x01: return LD_R16_D16_(REG_BC);
	case 0x11: return LD_R16_D16_(REG_DE);
	case 0x21: return LD_R16_D16_(REG_HL);
	case 0x31: return LD_R16_D16_(REG_SP);

	case 0x0A: return LD_R8_AR16_(REG_A, REG_BC);
	case 0x1A: return LD_R8_AR16_(REG_A, REG_DE);

	case 0x0E: return LD_R8_D8_(REG_C);
	case 0x1E: return LD_R8_D8_(REG_E);
	case 0x2E: return LD_R8_D8_(REG_L);
	case 0x3E: return LD_R8_D8_(REG_A);

	case 0x77: return LD_AR16_R8_(REG_HL, REG_A);

	case 0xE0: return LD_AD8_R8_(REG_A);

	case 0xE2: return LD_AR8_R8_(REG_C, REG_A);

		// LDD
	case 0x32: return LDD_AR16_D8_(REG_HL, REG_A);
	case 0x3A: return LDD_D8_AR16_(REG_A, REG_HL);

		// LDI
	case 0x22: return LDI_AR16_D8_(REG_HL, REG_A);
	case 0x2A: return LDI_D8_AR16_(REG_A, REG_HL);

		// NOP
	case 0x00: return NOP_();

		// XOR
	case 0xAF: return XOR_R8_(REG_A);

		// CB Prefixes
	case 0xCB: return CB_Prefix_();

	default:
		throw std::runtime_error(ConcatString("Unknown opcode: 0x", std::hex, +opcode));
	}
}
