#pragma once

#include "dao.hpp"

class GBMemoryBankController {
public:
	virtual const std::string name() const = 0;
};

class GBCartridge
{
public:
	GBCartridge(const GBDAO& dao, const std::string& id);

	const std::string const title() const;
	size_t rom_size() const;
	size_t ram_size() const;

private:
	const std::unique_ptr<uint8_t> rom_;
	const std::unique_ptr<GBMemoryBankController> mbc_;
};

class GBConnectors {
public:
	GBConnectors(const GBDAO& dao);

	void LoadCartridge(const std::string& cart_id);

private:
	const GBDAO& dao_;

	std::unique_ptr<GBCartridge> cart_;
};

class GBGPU {
public:
	GBGPU();

	inline const uint8_t* vram() const { return vram_; }
	inline uint8_t* vram() { return vram_; }

	void DoMachineCycle();

private:
	uint8_t vram_[8 * 1024] = {};
};

class GBVirtualMemory
{
public:
	GBVirtualMemory(const uint8_t* boot_rom, GBConnectors& conns, GBGPU& gpu);

	uint8_t Get8(size_t addr) const;
	uint16_t Get16(size_t addr) const;
	void Set8(uint16_t addr, uint8_t val);
	void Set16(uint16_t addr, uint16_t val);

private:
	const uint8_t* boot_rom_;
	GBConnectors& conns_;
	GBGPU& gpu_;
};

class GBCPU {
public:
	static const double kGBClockFreq, kGBClockPeriod;

	GBCPU(const const uint8_t* boot_rom, GBConnectors& conns, GBGPU& gpu);

	void Start();
	void DoMachineCycle();

private:
	// TODO: Make compatible with big-endian operating systems
	union Register_ {
		uint16_t word;
		uint8_t byte;
		uint8_t bytes[2];
	};

	GBVirtualMemory mem_;

	Register_ registers_[6] = {};

	uint8_t opcode_;
	unsigned int optime_ = 0;

	void Push16_(uint16_t val);
	uint16_t Pop16_();

	uint8_t Next8_();
	uint16_t Next16_();

	unsigned int BIT_(uint8_t pos, uint8_t op);

	unsigned int CALL_D16_();

	unsigned int EI_();

	unsigned int INC_R8_(uint8_t& op);

	unsigned int JR_AR8_();
	unsigned int JR_Cond_AR8_(bool cond);

	unsigned int LD_AD8_R8_(uint8_t src);
	unsigned int LD_AR8_R8_(uint8_t destAddr, uint8_t src);
	unsigned int LD_AR16_R8_(uint16_t destAddr, uint8_t src);
	unsigned int LD_R8_AR16_(uint8_t& dest, uint16_t srcAddr);
	unsigned int LD_R8_D8_(uint8_t& dest);
	unsigned int LD_R16_D16_(uint16_t& dest);

	unsigned int LDD_AR16_D8_(uint16_t& destAddr, uint8_t src);
	unsigned int LDD_D8_AR16_(uint8_t& dest, uint16_t& srcAddr);

	unsigned int LDI_AR16_D8_(uint16_t& destAddr, uint8_t src);
	unsigned int LDI_D8_AR16_(uint8_t& dest, uint16_t& srcAddr);

	unsigned int NOP_();

	unsigned int XOR_R8_(uint8_t op);

	unsigned int CB_Prefix_();

	// Returns the amount of time the instruction takes
	unsigned int Exec_(uint8_t opcode);
};

class DMGGameBoy {
public:
	DMGGameBoy(const GBDAO& dao);

	void Start();
	long long Update(long long dclock);

	void LoadCartridge(const std::string& cart_id);

private:
	std::unique_ptr<uint8_t> boot_rom_;
	GBConnectors conns_;
	GBGPU gpu_;
	GBCPU cpu_;

	long long
		time_ = 0,
		ccycles_ = 0;
};
