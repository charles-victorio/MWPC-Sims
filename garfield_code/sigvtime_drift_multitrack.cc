#include <fstream>
#include <iostream>
#include <cmath>
#include <random>

#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/TrackHeed.hh"

using namespace Garfield;

// ---------------------------------------------------------------------------
// Multi-track signal generator (DriftLineRKF).
//
// Shoots N muon tracks (random impact point + cos^3(alpha) angle, matching the
// production sampling) and writes each track's full induced-current waveform.
//
// CRITICAL: sensor.ClearSignal() is called PER TRACK. Without it every track
// accumulates onto the previous one and you'd get a sum, not a set of separate
// waveforms.
//
// Output: signal_multi.csv  (long format, one row per time-bin per track)
//   columns: trackID, t_ns, total_fCns, electron_fCns, ion_fCns
//
// Usage:  ./signal_multi <voltage> <meanGain> [theta] [nTracks]
//   e.g.  ./signal_multi 2500 10000 0.45 200
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {

    
  const double voltage  = (argc > 1) ? std::stod(argv[1]) : 2500.;
  const double meanGain = (argc > 2) ? std::stod(argv[2]) : 11619.7; // 1.e4;
  const double theta    = (argc > 3) ? std::stod(argv[3]) : 0.158; // 0.45;
  const int    nTracks  = (argc > 4) ? std::atoi(argv[4]) : 200;
  const std::string output_dir = "../../output/sigvtime_" + std::to_string(static_cast<int>(voltage)) + "V/";
      
  std::mt19937 gen(140805);

  MediumMagboltz gas;
  gas.LoadGasFile("../data/ar_75_co2_25.gas");
  gas.LoadIonMobility("../data/IonMobility_Ar+_Ar.txt");
  gas.SetMaxElectronEnergy(400.);
  gas.EnablePenningTransfer();

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

  Sensor sensor(&cmp);
  sensor.AddElectrode(&cmp, "s");
  const double tstep = 1.0;          // [ns]
  const std::size_t nbins = 5000;    // 5 us
  const double tmin = 0.;
  sensor.SetTimeWindow(tmin, tstep, nbins);

  TrackHeed track(&sensor);
  track.SetParticle("muon");
  track.SetEnergy(170.e9);

  DriftLineRKF drift(&sensor);
  drift.SetGainFluctuationsPolya(theta, meanGain);
  // ion tail + signal calc are default-on.

  const double xmax = pitch / 2 - 0.01;
  std::uniform_real_distribution<double> xDist(-xmax, xmax);
  std::uniform_real_distribution<double> alphaDist(-M_PI/2.0, M_PI/2.0);
  std::uniform_real_distribution<double> rejDist(0.0, 1.0);

  const std::string out_fname = output_dir + "signal_multi.csv";
  std::ofstream out(out_fname);
  out << "trackID,t_ns,total_fCns,electron_fCns,ion_fCns\n";

  for (int trk = 0; trk < nTracks; ++trk) {
    sensor.ClearSignal();

    const double x0 = xDist(gen);
    const double y0 = gap - 1.e-4;
    double alpha;
    while (true) {
      alpha = alphaDist(gen);
      double c = std::cos(alpha);
      if (rejDist(gen) < c*c*c) break;
    }
    const double dx =  std::sin(alpha);
    const double dy = -std::cos(alpha);

    track.NewTrack(x0, y0, 0, 0, dx, dy, 0);
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

    if ((trk + 1) % 20 == 0)
      std::cout << "done " << (trk + 1) << " / " << nTracks << " tracks\n";
  }
  out.close();
  std::cout << "wrote signal_multi.csv (" << nTracks << " tracks)\n";
  return 0;
}