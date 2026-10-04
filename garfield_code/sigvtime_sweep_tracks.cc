#include <fstream>
#include <iostream>
#include <cmath>
#include <random>
#include <vector>
#include <string>

#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/TrackHeed.hh"

using namespace Garfield;

// ---------------------------------------------------------------------------
// Fixed-geometry impact-parameter scan (DriftLineRKF).
//
// For each closest-approach distance d (horizontal offset of a VERTICAL track
// from the wire), shoot M tracks. Geometry is FIXED, so the only things that
// fluctuate are: gain Polya, diffusion, and Heed cluster statistics. This
// isolates the INTRINSIC resolution at each distance, decomposing the
// flux-averaged bimodality into a smooth curve + narrow per-point spread.
//
// Vertical tracks (dx=0, dy=-1): the impact parameter = closest-approach
// distance = x0 (the track's distance to the wire is minimal at the wire's
// height and larger above/below, so x0 is the closest-approach value).
//
// One output file per distance, in the SAME long format as signal_multi.csv,
// so plot_signal_multi.py works unchanged on each file:
//   signal_d0.0100.csv, signal_d0.0200.csv, ...
//   columns: trackID, t_ns, total_fCns, electron_fCns, ion_fCns
//
// Usage:  ./signal_scan <voltage> <meanGain> [theta] [tracksPerDist]
//   e.g.  ./signal_scan 2500 10000 0.45 200
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {

  const double voltage  = (argc > 1) ? std::stod(argv[1]) : 2500.;
  const double meanGain = (argc > 2) ? std::stod(argv[2]) : 1.e4;
  const double theta    = (argc > 3) ? std::stod(argv[3]) : 0.45;
  const int    M        = (argc > 4) ? std::atoi(argv[4]) : 200;

  const std::string output_dir = "../../output/sweep_tracks_" + std::to_string(static_cast<int>(voltage)) + "V/";

  // Finer spacing near the wire where arrival time / gain vary fastest.
  const std::vector<double> distances =
      {0.01, 0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.14};   // [cm]

  std::mt19937 gen(140805);

  // --- Gas ----------------------------------------------------------------
  MediumMagboltz gas;
  gas.LoadGasFile("../data/ar_75_co2_25.gas");
  gas.LoadIonMobility("../data/IonMobility_Ar+_Ar.txt");
  gas.SetMaxElectronEnergy(400.);
  gas.EnablePenningTransfer();

  // --- Geometry -----------------------------------------------------------
  ComponentAnalyticField cmp;
  cmp.SetMedium(&gas);
  const double wireDiameter = 20.e-4;
  const double gap = 0.6;
  const double pitch = 0.3;
  cmp.AddWire(0, 0, wireDiameter, voltage, "s");
  cmp.AddPlaneY(gap, 0., "top");
  cmp.AddPlaneY(-gap, 0., "bot");
  cmp.SetPeriodicityX(pitch);
  cmp.AddReadout("s");

  // --- Sensor + time window -----------------------------------------------
  Sensor sensor(&cmp);
  sensor.AddElectrode(&cmp, "s");
  const double tstep = 1.0;          // [ns]
  const std::size_t nbins = 5000;    // 5 us
  sensor.SetTimeWindow(0., tstep, nbins);

  // --- Heed + RKF ---------------------------------------------------------
  TrackHeed track(&sensor);
  track.SetParticle("muon");
  track.SetEnergy(170.e9);

  DriftLineRKF drift(&sensor);
  drift.SetGainFluctuationsPolya(theta, meanGain);

  for (double d : distances) {
    // build a filename like signal_d0.0100.csv
    char buf[64];
    std::string out_fname = output_dir + "signal_d%.4f.csv";
    std::snprintf(buf, sizeof(buf), out_fname.c_str(), d);
    std::ofstream out(buf);
    out << "trackID,t_ns,total_fCns,electron_fCns,ion_fCns\n";

    for (int trk = 0; trk < M; ++trk) {
      sensor.ClearSignal();                 // per track

      const double x0 = d;                  // FIXED closest-approach distance
      const double y0 = gap - 1.e-4;
      track.NewTrack(x0, y0, 0, 0, 0, -1, 0);   // vertical (dx=0, dy=-1)

      for (const auto& cluster : track.GetClusters())
        for (const auto& e : cluster.electrons)
          drift.DriftElectron(e.x, e.y, e.z, e.t);

      for (std::size_t i = 0; i < nbins; ++i) {
        out << trk << ","
            << (i + 0.5) * tstep << ","
            << sensor.GetSignal("s", i) << ","
            << sensor.GetElectronSignal("s", i) << ","
            << sensor.GetIonSignal("s", i) << "\n";
      }
    }
    out.close();
    std::cout << "wrote " << buf << " (" << M << " tracks at d=" << d << " cm)\n";
  }

  std::cout << "scan complete\n";
  return 0;
}