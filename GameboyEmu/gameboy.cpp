#include "stdafx.hpp"

#include "gameboy.hpp"

std::unique_ptr<uint8_t> LoadBootROM(const GBDAO& dao) {
	BOOST_LOG_TRIVIAL(info) << "Loading boot ROM";
	size_t size;
	auto rom = dao.GetDMGBootROM(&size);

	if (size != 256)
		throw std::runtime_error("Boot ROM is not 256 bytes");

	BOOST_LOG_TRIVIAL(debug) << "Boot ROM loaded.";
	return rom;
}

GBConnectors::GBConnectors(const GBDAO& dao) :
	dao_(dao) {}

void GBConnectors::LoadCartridge(const std::string& cart_id) {
	cart_.reset(new GBCartridge(dao_, cart_id));
}

DMGGameBoy::DMGGameBoy(const GBDAO& dao)
	: boot_rom_(LoadBootROM(dao)),
	conns_(dao),
	gpu_(),
	cpu_(boot_rom_.get(), conns_, gpu_) {}

void DMGGameBoy::Start() {
	cpu_.Start();
}

long long DMGGameBoy::Update(long long dtime) {
	time_ += dtime;
	long long
		dccycle = (long long)(time_ / GBCPU::kGBClockPeriod) - ccycles_,
		dmcycle = dccycle / 4;
	ccycles_ += dccycle;

	for (size_t i = 0; i < dmcycle; ++i) {
		cpu_.DoMachineCycle();
	}

	return dccycle;
}

void DMGGameBoy::LoadCartridge(const std::string& cart_id) {
	conns_.LoadCartridge(cart_id);
}