#include "../average.hxx"

#include <iostream>
#include <vector>
#include <string>

struct Measurement {
	double value;
	double weight;
};

void testRegister(const std::vector<Measurement>& measurements, const std::string& testName)
{
	using cdownload::AveragingRegister;

	AveragingRegister reg;
	for (const Measurement& m: measurements) {
		reg.add(m.value, m.weight);
	}

	std::cout << "Test: " << testName
		<< " mean: " << reg.mean()
		<< " count: " << reg.count()
		<< " stddev: " << reg.stdDev() << std::endl;
}

int main(int /*argc*/, char** /*argv*/)
{

	testRegister({{1,1}}, "simple 1");
	testRegister({{1,1}, {1,1}}, "simple 2");
	return 0;
}
