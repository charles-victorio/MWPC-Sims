#include <fstream>
#include <iostream>
#include <cmath>
#include <vector>
#include <limits>

#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/TrackHeed.hh"

using namespace Garfield;

// ---------------------------------------------------------------------------
// Signal-vs-time study for the MWPC, using DriftLineRKF.
//
// Why DriftLineRKF rather than AvalancheMC for the SIGNAL:
//   - The ion tail is default-ON (it's the dominant long-time feature, and
//     AvalancheMC made you drift every ion by hand and crashed on memory).
//   - Gain is an INPUT via SetGainFluctuationsPolya(theta, mean), so we feed
//     the mean gain measured with the AvalancheMicroscopic runs. Microscopic
//     gave the gain; RKF uses it for the shape. Consistent by construction.
//   - Much lighter than microscopic avalanching at MWPC gains.
//
// The pulse-SHAPE / timing features (first arrival, slowest primary, ion-tail
// tau, time to 1% of peak) do NOT depend much on the gain value, so they are
// robust even if the mean gain is approximate.
//
// Usage:  ./sigvtime_drift <voltage> <meanGain> [theta]
//   e.g.  ./sigvtime_drift 2500 10000 0.45
// Output: signal.csv  -> t_ns, total_fCns, electron_fCns, ion_fCns
//         charge_vs_shaping.csv
//         timing features to stdout.
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {

  const double voltage  = (argc > 1) ? std::stod(argv[1]) : 2500.;
  const double meanGain = (argc > 2) ? std::stod(argv[2]) : 11619.7; // 1.e4; 
  const double theta    = (argc > 3) ? std::stod(argv[3]) : 0.158; // 0.45

  MediumMagboltz gas;
  gas.LoadGasFile("../data/ar_75_co2_25.gas");
  gas.LoadIonMobility("../data/IonMobility_Ar+_Ar.txt");
  gas.SetMaxElectronEnergy(400.);
  gas.EnablePenningTransfer();

  ComponentAnalyticField cmp;
  cmp.SetMedium(&gas);
  const double wireDiameter = 20.e-4;   // [cm]
  const double gap = 0.6;
  const double pitch = 0.3;
  cmp.AddWire(0, 0, wireDiameter, voltage, "s");
  cmp.AddPlaneY(gap, 0., "top");
  cmp.AddPlaneY(-gap, 0., "bot");
  cmp.SetPeriodicityX(pitch);
  cmp.AddReadout("s");

  // Ion tail runs to microseconds; window must be long enough or tau is wrong.
  // 1 ns bins x 5000 = 5 us. Code warns if the tail is still alive at the end.
  Sensor sensor(&cmp);
  sensor.AddElectrode(&cmp, "s");
  const double tstep = 1.0;          // [ns]
  const std::size_t nbins = 5000;    // 5 us
  const double tmin = 0.;
  sensor.SetTimeWindow(tmin, tstep, nbins);
  sensor.ClearSignal();

  TrackHeed track(&sensor);
  track.SetParticle("muon");
  track.SetEnergy(170.e9);

  // --- DriftLineRKF with signal + gain ------------------------------------
  DriftLineRKF drift(&sensor);
  drift.SetGainFluctuationsPolya(theta, meanGain);
  // Ion tail and signal calc are default on

  // A representative near-vertical track through the middle of the cell.
  const double x0 = 0.05;
  const double y0 = gap - 1.e-4;
  track.NewTrack(x0, y0, 0, 0, 0, -1, 0);

  std::cout << "clusters: " << track.GetClusters().size() << std::endl;

  // Drift every primary electron. Each accumulates onto the SAME sensor
  // signal (we don't clear between electrons), so the final waveform is the
  // whole track's induced current, electrons + their avalanche ion tails.
  for (const auto& cluster : track.GetClusters()) {
    for (const auto& e : cluster.electrons) {
      drift.DriftElectron(e.x, e.y, e.z, e.t);
    }
  }

  // --- Read the waveform out ----------------------------------------------
  const std::string label = "s";
  std::vector<double> t(nbins), tot(nbins), el(nbins), ion(nbins);
  for (std::size_t i = 0; i < nbins; ++i) {
    t[i]   = (i + 0.5) * tstep;                  // bin-centre time [ns]
    tot[i] = sensor.GetSignal(label, i);
    el[i]  = sensor.GetElectronSignal(label, i);
    ion[i] = sensor.GetIonSignal(label, i);
  }

  std::ofstream out("signal.csv");
  out << "t_ns,total_fCns,electron_fCns,ion_fCns\n";
  for (std::size_t i = 0; i < nbins; ++i)
    out << t[i] << "," << tot[i] << "," << el[i] << "," << ion[i] << "\n";
  out.close();


  auto mag = [](double v){ return std::fabs(v); };
  std::ofstream sh("charge_vs_shaping.csv");
  sh << "shaping_ns,collected_fC\n";
  for (double T : {10., 30., 100., 300., 1000., 3000.}) {
    double q = 0.;
    for (std::size_t i = 0; i < nbins && t[i] <= T; ++i)
      q += mag(tot[i]) * tstep;
    sh << T << "," << q << "\n";
  }
  sh.close();

  std::cout << "\nwrote signal.csv and charge_vs_shaping.csv\n";
  return 0;
}