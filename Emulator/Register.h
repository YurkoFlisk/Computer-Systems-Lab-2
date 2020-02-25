#pragma once
#include <string>
#include <bitset>
#include <stdexcept>

constexpr size_t MAX_REGISTER_BITS = 64;
using RegisterValue = uint64_t;

inline std::string toString(RegisterValue rv)
{
	return std::bitset<MAX_REGISTER_BITS>(rv).to_string();
}

class IRegister
{
public:
	virtual size_t length() const = 0;
	virtual RegisterValue maxValue() const = 0;
	virtual RegisterValue value() const = 0;
	virtual void setValue(RegisterValue) = 0;
	bool lsb() const
	{
		return value() & 1ull;
	}
	bool msb() const
	{
		return value() & (1ull << (length() - 1));
	}
	void setLSB(bool b)
	{
		setValue((value() & ~1ull) | RegisterValue(b));
	}
	void setMSB(bool b)
	{
		const size_t msbPos = length() - 1;
		setValue((value() & (~(1ull << msbPos)))
			| RegisterValue(b) << msbPos);
	}
	std::string str() const
	{
		const std::string str64 = toString(value());
		return str64.substr(str64.size() - length());
	}
};

class Register
	: public IRegister
{
public:
	Register(size_t length = MAX_REGISTER_BITS, RegisterValue value = 0ull)
		: m_length(length), m_value(value)
	{
		if (m_length == 0 || m_length > MAX_REGISTER_BITS)
			throw std::invalid_argument("Invalid register length");
	}

	size_t length() const override
	{
		return m_length;
	}
	RegisterValue maxValue() const override
	{
		// Twice shift to avoid shifting 64 bits
		return (1ull << (m_length - 1) << 1) - 1ull;
	}
	RegisterValue value() const override
	{
		return m_value;
	}
	void setValue(RegisterValue rv) override
	{
		if (rv > maxValue())
			throw std::invalid_argument("Incorrect value");
		m_value = rv;
	}
private:
	// Data is stored in m_length least significant bits of m_value
	RegisterValue m_value;
	const size_t m_length;
};

class ProxyRegister
	: public IRegister
{
public:
	ProxyRegister(Register& parent, size_t first, size_t last)
		: m_parent(parent), m_firstBit(first), m_lastBit(last)
	{
		if (m_firstBit > m_lastBit || m_lastBit >= parent.length())
			throw std::invalid_argument("Incorrect subregister bit range ["
				+ std::to_string(m_firstBit) + "; " + std::to_string(m_lastBit) + "]");
	}

	size_t length() const override
	{
		return m_lastBit - m_firstBit + 1;
	}
	RegisterValue maxValue() const override
	{
		// Twice shift to avoid shifting 64 bits
		// Works for 64-bit wide subregister as well
		return (1ull << (m_lastBit - m_firstBit) << 1) - 1ull;
	}
	RegisterValue value() const override
	{
		return (m_parent.value() & mask()) >> m_firstBit;
	}
	void setValue(RegisterValue rv) override
	{
		if (rv > maxValue())
			throw std::invalid_argument("Incorrect value");
		m_parent.setValue((m_parent.value() & ~mask()) | (rv << m_firstBit));
	}
private:
	RegisterValue mask() const
	{
		return maxValue() << m_firstBit;
	}

	// Parent register, of which this is a part of
	Register& m_parent;
	// Indexes of the first and last (inclusive) parent's bits for this register
	const size_t m_firstBit;
	const size_t m_lastBit;
};