#include <iostream>
#include <iomanip>
#include "ALU.h"

using namespace std;

void mul(uint32_t n1, uint32_t n2)
{
	ALU alu;
	Register multiplicand(32, n1);
	Register product(64, n2);
	ProxyRegister LOproduct(product, 0, 31);
	ProxyRegister HIproduct(product, 32, 63);
	for (int i = 0; i < 32; ++i)
	{
		cout << "*******************Iteration " << i << " of 32*******************" << endl;
		cout << "Register values: " << endl;
		cout << setw(20) << "Product: " << HIproduct.str() << ' ' << LOproduct.str() << endl;
		cout << setw(20) << "Multiplicand: " << multiplicand.str() << endl;
		bool carry = false;
		if (product.lsb())
		{
			cout << "LSB of product is 1, so add multiplicand to HIproduct" << endl;
			alu.add(HIproduct, multiplicand, HIproduct);
			carry = alu.carry();
			cout << setw(20) << "HIProduct: " << HIproduct.str() << endl;
			cout << "Out carry is " << carry << endl;
		}
		else
			cout << "LSB of product is 0, so go to the next iteration" << endl;
		cout << "Shift product right 1 bit" << endl;
		alu.shr1(product, product);
		if (carry)
		{
			cout << "Carry from addition was 1, so change HIproduct MSB to 1 ('shift' set it to 0)" << endl;
			product.setMSB(carry);
		}
	}
	cout << "*******************Last iteration, end of the algorithm*******************" << endl;
	cout << "Result as 64-bit number: " << product.str()
		<< ", decimal: " << product.value() << endl;
	cout << "Result as 32-bit number: " << LOproduct.str()
		<< ", decimal: " << LOproduct.value() << endl;
}

void div(uint32_t n1, uint32_t n2)
{
	ALU alu;
	Register divisor(32, n2);
	Register remainder(64, n1);
	ProxyRegister LOrem(remainder, 0, 31);
	ProxyRegister HIrem(remainder, 32, 63);
	for (int i = 0; i < 32; ++i)
	{
		cout << "*******************Iteration " << i << " of 32*******************" << endl;
		cout << "Register values: " << endl;
		cout << setw(20) << "Remainder: " << HIrem.str() << ' ' << LOrem.str() << endl;
		cout << setw(20) << "Divisor: " << divisor.str() << endl;

		cout << "Shift remainder left 1 bit" << endl;
		alu.shl1(remainder, remainder);
		cout << setw(20) << "Remainder: " << HIrem.str() << ' ' << LOrem.str() << endl;

		cout << "Subtract divisor from HIrem" << endl;
		alu.sub(HIrem, divisor, HIrem);
		cout << setw(20) << "HIrem: " << HIrem.str() << endl;
		cout << "Out carry is " << alu.carry() << endl;
		if (alu.carry())
		{
			cout << "Since out carry is 1, curent quotient digit is 0,"
				" which is already the LSB of remainder" << endl;
			cout << "Divisor was bigger, so restore HIrem" << endl;
			alu.add(HIrem, divisor, HIrem);
			cout << setw(20) << "HIrem: " << HIrem.str() << endl;
		}
		else
		{
			cout << "Since out carry is 0, curent quotient digit is 1,"
				" so set it as the new remainder LSB" << endl;
			remainder.setLSB(1);
		}
	}
	cout << "*******************Last iteration, end of the algorithm*******************" << endl;
	cout << setw(20) << "Quotient: " << LOrem.str()
		<< ", decimal: " << LOrem.value() << endl;
	cout << setw(20) << "Remainder: " << HIrem.str()
		<< ", decimal: " << HIrem.value() << endl;
}

// May be not correct in some edge cases (e.g. denormals)
void fmul(float fn1, float fn2)
{
	constexpr uint32_t bias = 127;

	uint32_t n1 = *reinterpret_cast<uint32_t*>(&fn1);
	uint32_t n2 = *reinterpret_cast<uint32_t*>(&fn2);

	bool n1sign = n1 & (1u << 31);
	bool n2sign = n2 & (1u << 31);

	uint64_t n1mant = (1u << 23) | (n1 & ((1u << 23) - 1));
	uint64_t n2mant = (1u << 23) | (n2 & ((1u << 23) - 1));

	uint32_t n1E = (n1 >> 23) & ((1u << 8) - 1);
	uint32_t n2E = (n2 >> 23) & ((1u << 8) - 1);

	cout << setw(20) << "First number: " << n1sign << ' ' << bitset<8>(n1E).to_string()
		<< ' ' << bitset<23>(n1mant).to_string() << endl;
	cout << setw(20) << "Second number: " << n2sign << ' ' << bitset<8>(n2E).to_string()
		<< ' ' << bitset<23>(n2mant).to_string() << endl;

	bool ressign = n1sign ^ n2sign;
	cout << "Sign of the result: " << ressign << endl;

	uint32_t resE = n1E + n2E - bias;
	cout << "Exponent (biased) before normalization: "
		<< bitset<8>(resE).to_string() << ", decimal: " << resE << endl;

	uint64_t resmant = n1mant * n2mant;
	cout << "Mantissa before normalization: "
		<< bitset<48>(resmant).to_string().insert(2, 1, '.') << endl;

	if (resmant >= (1ull << 47))
		resmant >>= 1, ++resE;
	resmant &= ~(1ull << 46);
	resmant >>= 23;
	cout << "Exponent (biased) after normalization: "
		<< bitset<8>(resE).to_string() << ", decimal: " << resE << endl;
	cout << "Mantissa after normalization and discarding: "
		<< "1." + bitset<23>(resmant).to_string() << endl;

	if (resE > (1 << 16)) // Underflow
	{
		cout << "Underflow happened, so setting number to (signed) zero" << endl;
		resE = 0;
		resmant = 0;
	}
	else if (resE >= (1 << 8) - 1) // Overflow
	{
		cout << "Overflow happened, so setting number to (signed) inf" << endl;
		resE = (1 << 8) - 1;
		resmant = 0;
	}

	uint32_t res = (uint32_t(ressign) << 31) | (resE << 23) | resmant;
	float fres = *reinterpret_cast<float*>(&res);
	cout << "Result in binary: " << bitset<32>(res).to_string()
		.insert(1, 1, ' ').insert(10, 1, ' ') << endl;
	cout << "Result as a float: " << fres << endl;
}

int main()
{
	int op;
	float fn1, fn2;
	uint32_t n1, n2;
	while (true)
	{
		try
		{
			cout << "0 - exit" << endl;
			cout << "1 - unsigned 32-bit integer multiplication" << endl;
			cout << "2 - unsigned 32-bit integer division" << endl;
			cout << "3 - 32-bit floating-point multiplication" << endl;
			cout << "Enter operation: ";
			cin >> op;
			if (op == 0)
				break;
			if (op == 1)
			{
				cout << "Enter multiplicand: ";
				cin >> n1;
				cout << "Enter multiplier: ";
				cin >> n2;
				mul(n1, n2);
			}
			else if (op == 2)
			{
				cout << "Enter dividend: ";
				cin >> n1;
				cout << "Enter divisor: ";
				cin >> n2;
				div(n1, n2);
			}
			else if (op == 3)
			{
				cout << "Enter first number: ";
				cin >> fn1;
				cout << "Enter second number: ";
				cin >> fn2;
				fmul(fn1, fn2);
			}
			else
				cout << "Unknown command, try again";
			cout << "***********************************************************" << endl;
		}
		catch (const std::exception& ex)
		{
			cout << ex.what() << endl;
		}
	}
	return 0;
}