#include <fstream>
#include <iostream>
#include <cmath>
#include <vector>
#include <limits>

#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/AvalancheMC.hh"
#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/TrackHeed.hh"

using namespace Garfield;

// ---------------------------------------------------------------------------
// Signal-vs-time study for the MWPC.
//
// This is a SEPARATE program from the gain/charge runs. It does NOT count ne;
// it computes the induced current on the sense wire as a function of time
// (Shockley-Ramo, via the weighting field of electrode "s") so we can read off
// timing features: first electron arrival, slowest primary arrival, ion-tail
// timescale, time to peak, time to 1% of peak.
//
// Uses AvalancheMC (macroscopic) rather than AvalancheMicroscopic: the pulse
// SHAPE is set by drift velocities + geometry + the ion tail, not by the
// microscopic collision detail, and AvalancheMC with signal calc actually
// finishes at MWPC gains instead of exhausting memory.
//
// Usage:  ./sigvtime <voltage>            (e.g. ./sigvtime 2750)
// Output: signal.csv  -> columns: t_ns, total[fC/ns], electron[fC/ns], ion[fC/ns]
//         timing features printed to stdout.
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {

  const double voltage = (argc > 1) ? std::stod(argv[1]) : 2500.;

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

  // --- Sensor + time window -----------------------------------------------
  // The ion tail is SLOW. Ions drift ~1000x slower than electrons, so the
  // signal has structure out to ~microseconds. Make the window long enough to
  // capture it or you will truncate the tail and misjudge its timescale.
  //   tstep = 1 ns, nbins = 5000  -> 5 us window.
  // Widen nbins if the tail is still falling at the end (the code warns you).
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

  // --- Avalanche with signal calculation ON -------------------------------
  AvalancheMC aval(&sensor);
  aval.SetDistanceSteps(2.e-4);      // step size [cm]; tune for accuracy/speed
  aval.EnableSignalCalculation();
  
  // separate objects for ions (avoids the reset bug)
  AvalancheMC ionDrift(&sensor);
  ionDrift.SetDistanceSteps(2.e-4);
  ionDrift.EnableSignalCalculation();


  std::cout << "before NewTrack" << std::endl;
  // A representative near-vertical track through the middle of the cell.
  // One track is enough to SEE the pulse shape; the timing features are not
  // strongly track-to-track variable for the shape itself.
  const double x0 = 0.05;
  const double y0 = gap - 1.e-4;
  track.NewTrack(x0, y0, 0, 0, 0, -1, 0);

  std::cout << "clusters: " << track.GetClusters().size() << std::endl;

  // Drift+avalanche every primary electron. Each contributes to the SAME
  // accumulated signal on the sensor (we do NOT clear between electrons), so
  // the final waveform is the full track's induced current.
  int electron_i = 0;
  for (const auto& cluster : track.GetClusters()) {
    for (const auto& e : cluster.electrons) {
      aval.AvalancheElectron(e.x, e.y, e.z, e.t);
      std::cout << "electron #" << electron_i++ << " at t = " << e.t << std::endl;

      const unsigned int np = aval.GetNumberOfElectronEndpoints();
      double x1,y1,z1,t1, x2,y2,z2,t2; int status;
      for (unsigned int i = 0; i < np; ++i) {
        aval.GetElectronEndpoint(i, x1,y1,z1,t1, x2,y2,z2,t2, status);
        // ion born where the electron STARTED (x1,y1,z1,t1) — the avalanche site
        ionDrift.DriftIon(x1, y1, z1, t1);
      }
    }
  }


  std::cout << "before GetSignal" << std::endl;
  // --- Read the waveform out ----------------------------------------------
  // GetSignal returns induced current in fC/ns; electron and ion components
  // are available separately, which is exactly what lets us separate the fast
  // electron edge from the slow ion tail.
  const std::string label = "s";
  std::vector<double> t(nbins), tot(nbins), el(nbins), ion(nbins);
  for (std::size_t i = 0; i < nbins; ++i) {
    t[i]   = (i + 0.5) * tstep;                  // bin-centre time [ns]
    tot[i] = sensor.GetSignal(label, i);
    el[i]  = sensor.GetElectronSignal(label, i);
    ion[i] = sensor.GetIonSignal(label, i);
  }

  std::cout << "after GetSignal" << std::endl;

  std::ofstream out("signal.csv");
  out << "t_ns,total_fCns,electron_fCns,ion_fCns\n";
  for (std::size_t i = 0; i < nbins; ++i)
  out << t[i] << "," << tot[i] << "," << el[i] << "," << ion[i] << "\n";
  out.close();
  
  std::cout << "signGal.csv written" << std::endl;
  // -------------------------------------------------------------------------
  // Timing features.
  //
  // NOTE ON SIGN: for electrons collected on the anode the induced current is
  // negative. We work with magnitudes so "peak" means peak magnitude.
  // -------------------------------------------------------------------------
  auto mag = [](double v){ return std::fabs(v); };

  // charge collected vs shaping time, for various times
  // done by integrating |I| from 0 to t
  std::ofstream sh("charge_vs_shaping.csv");
  sh << "shaping_ns,collected_fC\n";
  for (double T : {10., 30., 100., 300., 1000., 3000.}) {
    double q = 0.;
    for (std::size_t i = 0; i < nbins && t[i] <= T; ++i)
      q += mag(tot[i]) * tstep;        // integrate fC/ns * ns = fC
    sh << T << "," << q << "\n";
  }
  sh.close();

  std::cout << "\nwrote signal.csv and charge_vs_shaping.csv\n";
  return 0;
}