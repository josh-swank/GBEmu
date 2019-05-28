#pragma once

template <typename THead>
void ConcatString(std::stringstream& ss, THead head) {
	ss << head;
}

template <typename THead, typename... Ts>
void ConcatString(std::stringstream& ss, THead head, Ts... args) {
	ss << head;
	ConcatString(ss, args...);
}

template <typename... Ts>
std::string ConcatString(Ts... args) {
	std::stringstream ss;
	ConcatString(ss, args...);
	return ss.str();
}
