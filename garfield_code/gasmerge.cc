#include <iostream>
#include <string>
#include "Garfield/MediumMagboltz.hh"

using namespace Garfield;

int main(int argc, char** argv) {
  if (argc < 2) { std::cerr << "usage: gasmerge <nTasks>\n"; return 1; }
  const int nTasks = std::atoi(argv[1]);

  MediumMagboltz gas;
  if (!gas.LoadGasFile("parts_of_gas/gas_part_1.gas")) { std::cerr << "load part 1 failed\n"; return 1; }
  std::cout << "Loaded gas_part_1.gas\n";

  for (int t = 2; t <= nTasks; ++t) {
    const std::string f = "parts_of_gas/gas_part_" + std::to_string(t) + ".gas";
    if (!gas.MergeGasFile(f, false)) { std::cerr << "merge " << f << " failed\n"; return 1; }
    std::cout << "Merged " << f << "\n";
  }

  gas.WriteGasFile("gas_full.gas");
  std::cout << "Wrote gas_full.gas\n";
  return 0;
}