#pragma once
#include "Register.h"

class ALU
{
public:
	bool carry() const
	{
		return m_carry;
	}

	void add(const IRegister& r1, const IRegister& r2, IRegister& res)
	{
		testCompatibility(r1, res);
		testCompatibility(r2, res);
		const RegisterValue r1v = r1.value(), r2v = r2.value();
		const RegisterValue sum = (r1v + r2v) & res.maxValue();
		if (sum < r1v)
			m_carry = true;
		else
			m_carry = false;
		res.setValue(sum);
	}
	void sub(const IRegister& r1, const IRegister& r2, IRegister& res)
	{
		testCompatibility(r1, res);
		testCompatibility(r2, res);
		const RegisterValue r1v = r1.value(), r2v = r2.value();
		const RegisterValue diff = (r1v - r2v) & res.maxValue();
		if (diff > r1v)
			m_carry = true;
		else
			m_carry = false;
		res.setValue(diff);
	}
	void shr1(const IRegister& r1, IRegister& res)
	{
		testCompatibility(r1, res);
		m_carry = r1.lsb();
		res.setValue(r1.value() >> 1);
	}
	void shl1(const IRegister& r1, IRegister& res)
	{
		testCompatibility(r1, res);
		m_carry = r1.msb();
		res.setValue((r1.value() << 1) & res.maxValue());
	}
private:
	void testCompatibility(const IRegister& r1, const IRegister& r2) const
	{
		if (r1.length() != r2.length())
			inputError();
	}
	[[noreturn]] void inputError() const
	{
		throw std::invalid_argument("Input register sizes incompatible");
	}

	bool m_carry = false;
};