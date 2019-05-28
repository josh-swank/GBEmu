#include "stdafx.hpp"

#include "gameboy.hpp"
#include "util.hpp"

const size_t
	kKB = 1024,
	kMB = 1048576,

	kCartTitle = 0x0134,
	kCartMBCType = 0x0147,
	kCartROMSize = 0x0148,
	kCartRAMSize = 0x0149;

class MBCNone : public GBMemoryBankController {
public:
	const std::string name() const override { return "None"; }
};

size_t GetCartROMSize(const uint8_t* rom) {
	switch (rom[kCartROMSize]) {
	case 0x00:
		return 32 * kKB;
	case 0x01:
		return 64 * kKB;
	case 0x02:
		return 128 * kKB;
	case 0x03:
		return 256 * kKB;
	case 0x04:
		return 512 * kKB;
	case 0x05:
		return kMB;
	case 0x06:
		return 2 * kMB;
	case 0x07:
		return 4 * kMB;
	case 0x08:
		return 8 * kMB;
	case 0x52:
		return 1.125 * kMB;
	case 0x53:
		return 1.25 * kMB;
	case 0x54:
		return 1.5 * kMB;
	default:
		throw std::runtime_error("Invalid cartridge ROM size");
	}
}

std::unique_ptr<GBMemoryBankController> CreateMBC(const uint8_t* rom) {
	GBMemoryBankController* mbc;
	switch (rom[kCartMBCType]) {
	case 0x00:
		mbc = new MBCNone();
		break;
	default:
		throw std::runtime_error("Unsupported cartridge memory bank controller");
	}
	return std::unique_ptr<GBMemoryBankController>(mbc);
}

std::unique_ptr<uint8_t> LoadCartridgeROM(const GBDAO& dao, const std::string& id) {
	BOOST_LOG_TRIVIAL(trace) << "Loading cartridge ROM: " << id;
	size_t size;
	auto rom = dao.GetCartridgeROM(id, &size);

	if (size != GetCartROMSize(rom.get()))
		throw std::runtime_error("Cartridge ROM's size doesn't match header");

	BOOST_LOG_TRIVIAL(debug) << "Cartridge ROM loaded";
	return rom;
}

GBCartridge::GBCartridge(const GBDAO& dao, const std::string& id) :
	rom_(LoadCartridgeROM(dao, id)), mbc_(CreateMBC(rom_.get())) {}

const std::string GBCartridge::title() const { return std::string((char*)rom_.get() + kCartTitle, 16); }
size_t GBCartridge::rom_size() const { return GetCartROMSize(rom_.get()); }

size_t GBCartridge::ram_size() const {
	switch (rom_.get()[kCartRAMSize]) {
	case 0x00:
		return 0;
	case 0x01:
		return 2 * kKB;
	case 0x02:
		return 8 * kKB;
	case 0x03:
		return 32 * kKB;
	case 0x04:
		return 128 * kKB;
	case 0x05:
		return 64 * kKB;
	default:
		throw std::runtime_error("Invalid cartridge RAM size");
	}
}

GBVirtualMemory::GBVirtualMemory(const uint8_t* boot_rom, GBConnectors& conns, GBGPU& gpu) :
	boot_rom_(boot_rom), conns_(conns), gpu_(gpu) {}

uint8_t GBVirtualMemory::Get8(size_t addr) const {
	switch (addr & 0xF000) {
	case 0x0000:
		return boot_rom_[addr];
	case 0x8000: case 0x9000:
		return gpu_.vram()[addr - 0x8000];
	case 0xE000:
	case 0xF000:
		if (addr < 0xFE00)
			return gpu_.vram()[addr - 0xE000];
		else
			switch (addr) {
			case 0xFF26:
				// TODO: Sound on/off
				return 0;
			}
	default:
		throw std::runtime_error(ConcatString("Memory at ", addr, " can't be read"));
	}
}

uint16_t GBVirtualMemory::Get16(size_t addr) const {
	return Get8(addr) + (Get8(addr + 1) << 8);
}

void GBVirtualMemory::Set8(uint16_t addr, uint8_t val) {
	switch (addr & 0xF000) {
	case 0x8000: case 0x9000:
		gpu_.vram()[addr - 0x8000] = val;
		break;
	case 0xE000:
	case 0xF000:
		if (addr < 0xFE00)
			gpu_.vram()[addr - 0xE000] = val;
		else
			switch (addr) {
			case 0xFF26:
				// TODO: Sound on/off
				break;
			}
	default:
		throw std::runtime_error(ConcatString("Memory at ", addr, " can't be written"));
	}
}

void GBVirtualMemory::Set16(uint16_t addr, uint16_t val) {
	Set8(addr, val);
	Set8(addr + 1, val >> 8);
}
