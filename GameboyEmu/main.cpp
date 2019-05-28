#include "stdafx.hpp"

#include "gameboy.hpp"

const std::string
	kDMGBootFilepath = "dmg_boot.bin",
	kCartridgeROMPath = "../Roms/";

class DAO : public GBDAO {
public:
	std::unique_ptr<uint8_t> GetDMGBootROM(size_t* size = nullptr) const override {
		return std::unique_ptr<uint8_t>(ReadFile_(kDMGBootFilepath, size));
	}

	std::unique_ptr<uint8_t> GetCartridgeROM(const std::string& id, size_t* size = nullptr) const override {
		return std::unique_ptr<uint8_t>(ReadFile_(kCartridgeROMPath + id, size));
	}

private:
	static uint8_t* ReadFile_(const std::string& filename, size_t* size) {
		std::ifstream in;
		in.open(filename, std::fstream::in | std::fstream::binary | std::fstream::ate);
		auto n = in.tellg();
		auto data = new uint8_t[n];

		in.seekg(0);
		in.read((char*)data, n);
		in.close();

		if (size != nullptr)
			* size = n;
		return data;
	}
};

class App {
public:
	App() : dao_(), gb_(dao_) {}

	int Run(char** argv, int argc) {
		typedef std::chrono::steady_clock Clock;
		typedef std::chrono::time_point<Clock> TimePoint;

		gb_.LoadCartridge("Dr. Mario (World).gb");
		gb_.Start();

		TimePoint now, last;
		now = last = Clock::now();
		while (true) {
			now = Clock::now();

			std::chrono::nanoseconds duration = now - last;
			auto dtime = duration.count();

			gb_.Update(dtime);

			last = now;
		}

		return EXIT_SUCCESS;
	}

private:
	DAO dao_;
	DMGGameBoy gb_;
};

int main(char** argv, int argc)
{
	try {
		App app;
		return app.Run(argv, argc);
	} catch (const std::exception& e) {
		BOOST_LOG_TRIVIAL(error) << e.what();
	}
}
