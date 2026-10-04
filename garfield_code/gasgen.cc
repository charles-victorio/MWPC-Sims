#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include "Garfield/MediumMagboltz.hh"

using namespace Garfield;

int main(int argc, char** argv) {
  if (argc < 3) { std::cerr << "usage: gasgen <taskID(1-based)> <nTasks>\n"; return 1; }
  const int taskID = std::atoi(argv[1]);   // 1 .. nTasks
  const int nTasks = std::atoi(argv[2]);

  // ---- EDIT to your detector / gas ----
  const double emin = 100.;       // V/cm
  const double emax = 10000000.;    // V/cm // 10^7 when only 10^6 needed
  const int    ne   = 40;         // total E points in the final table // only 27 needed
  const int    numColl = 10;       // Magboltz collisions (~1e7 units): higher=smoother/slower // 5

  if (nTasks > ne) { std::cerr << "nTasks must be <= ne (" << ne << ")\n"; return 1; }

  // Full log-spaced E grid
  std::vector<double> eAll(ne);
  for (int i = 0; i < ne; ++i)
    eAll[i] = emin * std::pow(emax / emin, double(i) / (ne - 1));

  // Balanced contiguous partition -> this task's slice
  const int base = ne / nTasks, rem = ne % nTasks;
  int start = 0;
  for (int t = 1; t < taskID; ++t) start += base + (t <= rem ? 1 : 0);
  const int count = base + (taskID <= rem ? 1 : 0);
  std::vector<double> eSlice(eAll.begin() + start, eAll.begin() + start + count);

  std::cout << "Task " << taskID << "/" << nTasks << " : "
            << eSlice.front() << " - " << eSlice.back() << " V/cm ("
            << count << " pts)\n";

  // ---- Gas setup: IDENTICAL across all tasks (same binary => automatic) ----
  MediumMagboltz gas;
  gas.SetComposition("ar", 75., "co2", 25.);   // your mixture
  gas.SetTemperature(293.15);                  // K
  gas.SetPressure(760.);                       // Torr
  gas.EnablePenningTransfer();   // adjust/remove per mixture

  gas.SetFieldGrid(eSlice, {0.}, {0.});        // B = 0, single angle (no B field)
  gas.GenerateGasTable(numColl, true);

  const std::string out = "parts_of_gas/gas_part_" + std::to_string(taskID) + ".gas";
  gas.WriteGasFile(out);
  std::cout << "Wrote " << out << "\n";
  return 0;
}