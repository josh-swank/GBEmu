#pragma once

class GBDAO {
public:
	virtual std::unique_ptr<uint8_t> GetDMGBootROM(size_t* size = nullptr) const = 0;
	virtual std::unique_ptr<uint8_t> GetCartridgeROM(const std::string& id, size_t *size = nullptr) const = 0;
};
