#if 0
#include <cstdlib>

#include <iostream>
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/AvalancheMicroscopic.hh"

using namespace Garfield;

int main() {
    // --- Gas ---
    MediumMagboltz gas;
    gas.SetComposition("ar", 75., "co2", 25.);
    gas.SetTemperature(293.15);   // K, room temp
    gas.SetPressure(760.);        // Torr, NTP
    gas.SetMaxElectronEnergy(20000.);   // must be high enough for avalanche electrons
    // This runs Magboltz internally to get transport coefficients
    // Takes a few minutes the first time, caches results after
    gas.Initialise(true);

    // After Initialise(), query the Townsend coefficient at the wire surface field
    // Query alpha directly at a given E field magnitude
    // E in V/cm, B=0, angles=0
    double alpha = 0., eta = 0.;
    gas.ElectronTownsend(400000., 0., 0., 0., 0., 0., alpha);
    gas.ElectronAttachment(400000., 0., 0., 0., 0., 0., eta);
    std::cout << "alpha at 400kV/cm = " << alpha << std::endl;

    // // Check drift velocity at a moderate field — should be nonzero if table is filled
    // double vx, vy, vz;
    // gas.ElectronVelocity(1000., 0., 0., 0., 0., 0., vx, vy, vz);
    // std::cout << "Drift velocity at 1kV/cm = " << vx << " cm/ns" << std::endl;

    // --- Geometry: MWPC analytic field ---
    // This is the exact solution for an infinite wire array
    // between two parallel cathode planes
    ComponentAnalyticField cmp;
    cmp.SetMedium(&gas);

    // Wire: radius 15um, at y=0, voltage 4000V
    // pitch 3mm so wires at x = 0, 3mm, 6mm etc
    // AddWire(x, y, diameter, voltage, label)
    cmp.AddWire(0.,   0., 2*15e-4, 4000., "s");  // units are cm
    // cmp.AddWire(0.3,  0., 2*15e-4, 4000., "s");  // 3mm = 0.3cm
    // cmp.AddWire(-0.3, 0., 2*15e-4, 4000., "s");

    // Cathode planes at y = +/- 3mm (gap = 6mm, half-gap = 3mm) // should be 12mm?
    // AddPlaneY(y, voltage, label)
    cmp.AddPlaneY( 0.6, 0., "top");
    cmp.AddPlaneY(-0.6, 0., "bot");

    // Tell it the wire periodicity (pitch)
    cmp.SetPeriodicityX(0.3);   // 3mm pitch

    // // --- Check field at wire surface ---
    // double r_wire = 15e-4;      // 15 um in cm
    // double ex, ey, ez;
    // Medium* medium = nullptr;
    // int status;
    // cmp.ElectricField(r_wire * 1.01, 0., 0., ex, ey, ez, medium, status);
    // double emag = std::sqrt(ex*ex + ey*ey + ez*ez);
    // std::cout << "E at wire surface (V/cm) = " << emag << std::endl;

    // --- Sensor ---
    Sensor sensor;
    sensor.AddComponent(&cmp);
    sensor.SetArea(-0.15, -0.31, -0.5,
                    0.15,  0.31,  0.5);
    // sensor.SetTimeWindow(0., 0.5, 200);  // start, step (ns), bins

    
    // --- Avalanche simulation ---
    AvalancheMicroscopic aval;
    aval.SetSensor(&sensor);
    aval.EnableSignalCalculation();
    aval.SetCollisionSteps(100);
    // aval.EnableDebugging();  // verbose — lets you see where it's getting stuck

    // 1. Confirm field at wire surface is ~400 kV/cm
    double ex, ey, ez;
    Medium* med = nullptr; int status;
    cmp.ElectricField(15e-4 * 1.001, 0., 0., ex, ey, ez, med, status);
    std::cout << "E at wire surface = " << std::sqrt(ex*ex+ey*ey) << " V/cm\n";
    // Expect: ~300,000 – 500,000 V/cm

    // 2. Confirm drift velocity is sensible at 1 kV/cm
    double vx, vy, vz;
    gas.ElectronVelocity(1000., 0., 0., 0., 0., 0., vx, vy, vz);
    std::cout << "Drift velocity = " << vx << " cm/ns\n";
    // Expect: ~3–5 cm/µs = 0.003–0.005 cm/ns

    // 3. Run just 1 avalanche with full endpoint printing before the batch
    aval.AvalancheElectron(0., 0.15, 0., 0., 0.1);
    int ne, ni; aval.GetAvalancheSize(ne, ni);
    std::cout << "Test avalanche: ne=" << ne << " ni=" << ni << "\n";
    // Expect: ne in thousands to hundreds of thousands


    /*
    // Run many single-electron avalanches starting from just outside
    // the wire surface, drifting inward — this is what builds up gain stats
    int nAvalanches = 1;//000;
    std::vector<double> gains;

    for (int i = 0; i < nAvalanches; i++) {

        // Start one electron at the wire surface + small offset
        // x=15um + 1um, y=0, z=0, t=0, energy=0.1eV
        double x0 = (15e-4 + 1e-4);  // cm
        double y0 = 0.01 + 0.28 * (double)rand() / RAND_MAX;
        aval.AvalancheElectron(0., y0, 0., 0., 0.1);
        // aval.AvalancheElectron(x0, y0, z0, t0, e0, dx, dy, dz);
        // x,y,z: start position (cm); t0: time (ns); e0: initial energy (eV)
        // dx,dy,dz: initial direction (or 0,0,0 for isotropic)

        int ne, ni; // electrons and ions in avalanche
        aval.GetAvalancheSize(ne, ni);
        if (ne <= 1) {
            std::cerr << "Warning: avalanche " << i << " did not develop\n";
            continue;
        }
        gains.push_back((double)ne);

        if (i % 100 == 0)
            std::cout << "avalanche " << i << " gain = " << ne << std::endl;
    }

    // Mean gain
    double meanGain = 0., meanGainSq = 0.;
    for (auto g : gains) {
        meanGain += g;
        meanGainSq += g * g;
    }
    meanGain /= gains.size();
    meanGainSq /= gains.size();

    double variance = meanGainSq - meanGain * meanGain;
    double fFactor = variance / meanGain;

    std::cout << "\nMean gain = " << meanGain << std::endl;
    std::cout << "log10(G)  = " << std::log10(meanGain) << std::endl;
    std::cout << "Polya f = " << fFactor << "\n";
    */

    return 0;
}
#endif

// #include <cstdlib>

#include <fstream>
#include <iostream>
#include <vector>

#include <TCanvas.h>

#include "Garfield/MediumMagboltz.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/ViewMedium.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/ViewDrift.hh"
#include "Garfield/ViewCell.hh"
#include "Garfield/ViewSignal.hh"


using namespace Garfield;

// This function gets the transfer function 
// The function is defined emperically, measured from the Monitored Drift Tube in CERN's ATLAS muon spectromoter
// input of the transfer function: the current signal on the wires (physics)
// output of the transfer function: the voltage signal that appears, accounting for how the electronics distort the signal. This voltage is what you would see on an oscilloscope
// it simulates the "analog distortion". it distorts the signal the way that, emperically, the signal gets distorted by the preamp's transimpedence gain, the shaper's frequency filtering, any cable effects, input impedance, etc.
// However it the transfer function does not convert the analog to digital conversion (discriminator threshold, adc time sampling, adc quantization to digital value)
// readTransferFunction() just parses the 2 column table
bool readTransferFunction(Sensor &sensor) {
    std::ifstream infile;
    infile.open("../data/mdt_elx_delta.txt");
    if (!infile) {
        std::cerr << "Could not read delta response function.\n";
        return false;
    }
    std::vector<double> times;
    std::vector<double> values;
    while (!infile.eof()) {
        double t = 0., f = 0.;
        infile >> t >> f;
        if (infile.eof() || infile.fail()) break;
        times.push_back(1.e3 * t);
        values.push_back(f);
    }
    infile.close();
    sensor.SetTransferFunction(times, values);
    return true;
}

int main() {
    // Only need to do once
    // MediumMagboltz gas("ar", 93., "co2", 7.);
    // gas.SetTemperature(293.15);
    // gas.SetPressure(3 * 760.);
    // gas.SetFieldGrid(100., 100000., 20, true);
    // gas.GenerateGasTable(10);
    // gas.WriteGasFile("ar_93_co2_7.gas");
    MediumMagboltz gas;
    gas.LoadGasFile("../data/ar_93_co2_7.gas");
    gas.LoadIonMobility("../data/IonMobility_Ar+_Ar.txt");

    // ViewMedium mediumView(&gas);
    // mediumView.PlotElectronVelocity('e');
    // mediumView.GetCanvas()->SaveAs("../output/electron_drift_velocity.png");

    ComponentAnalyticField cmp;
    cmp.SetMedium(&gas);
    const double wireDiameter = 25.e-4; // 20.e-4; // [cm]
    const double tubeRadius = 0.71;
    const double wireVoltage = 0.1; // 2730.;
    const double tubeVoltage = 0.;
    cmp.AddWire(0, 0, wireDiameter, wireVoltage, "s");
    cmp.AddTube(tubeRadius, tubeVoltage, 0);

    Sensor sensor(&cmp);
    sensor.AddElectrode(&cmp, "s");
    const double tstep = 0.5;
    const double tmin = -0.5 * tstep;
    const std::size_t nbins = 1000;
    sensor.SetTimeWindow(tmin, tstep, nbins);
    // Set the delta response function
    if (!readTransferFunction(sensor)) return 0;
    sensor.ClearSignal();
    
    // Set up Heed
    TrackHeed track(&sensor);
    track.SetParticle("muon");
    track.SetEnergy(170.e9);

    // RKF integration
    //DriftLineRKF drift(&sensor);
    //drift.SetGainFluctuationsPolya(0., 20000.);
    //drift.EnableIonTail(true); // this is default on. not calling this function means ion tail will be enabled

    AvalancheMicroscopic aval(&sensor);

    TCanvas *canvas = new TCanvas("canvas", "", 600, 600);
    ViewDrift driftView;
    driftView.SetCanvas(canvas);
    aval.EnablePlotting(&driftView);
    track.EnablePlotting(&driftView);
    driftView.SetColourElectrons(3);
    driftView.SetColourIons(4);

    ViewCell cellView;
    cellView.SetCanvas(canvas);
    cellView.SetComponent(&cmp);

    // Simulate a track
    const double trackRadius = 0.3; // track (particle's trajectory) takes it to 3mm from the wire
    const double x0 = trackRadius;
    const double y0 = -sqrt(tubeRadius * tubeRadius - trackRadius * trackRadius);
    track.NewTrack(x0, y0, 0, 0, 0, 1, 0);
    std::size_t ne = 0;
    for (const auto &cluster : track.GetClusters()) {
        for (const auto &electron : cluster.electrons) {
            //drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
            std::cout << "drifting electron at time " << electron.t << std::endl;
            ne += 1;
            
            aval.AvalancheElectron(electron.x, electron.y, electron.z, electron.t, 0, 0, 0, 0);
            int neA, niA;
            aval.GetAvalancheSize(neA, niA);
            std::cout << "ne: " << neA << ", ni: " << niA << std::endl; 
        }
    }
    std::cout << "number of electrons: " << ne << std::endl;
    // sensor.GetIonSignal();

    // draw the drift tube with the track and the drifting
    sensor.ClearSignal(); canvas->Clear();
    cellView.Plot2d();
    constexpr bool twod = true;
    constexpr bool drawaxis = false;
    driftView.Plot(twod, drawaxis);
    driftView.GetCanvas()->SaveAs("../output/drift_lines.png");

    bool accountForFeElectronics = false;
    if (accountForFeElectronics) {
        sensor.ConvoluteSignals();
        int nt = 0;
        // if (!sensor.ComputeThresholdCrossings(-2., "s", nt)) continue;
        if (sensor.ComputeThresholdCrossings(-2., "s", nt)) {
            ViewSignal signalView(&sensor);
            signalView.PlotSignal("s");
            signalView.GetCanvas()->SaveAs("../output/signal.png");
        }
    } else {
        ViewSignal signalView(&sensor);
        signalView.PlotSignal("s");
        signalView.GetCanvas()->SaveAs("../output/signal.png");
    }
}

// questions
// [x] why are there drift lines on the right side? are those ions
// sensor.ComputeThresholdCrossings
// ViewSignal signalView(C&sensor).PlotSignal("s") vs sensor.PlotSignal()
// TCanvas

// then
// understand the gas coefficients (eg gas.ElectronTownsend)
// use avalanchemicroscopic! Next step:
// copy this code, edit it to change it to avalanche microscopic
// extract gain, induced change, drift time spread. as shown by claude
// then find out how to get Q_vs_tau

// wait maybe I should understand / take notes on
// - geometry params (eg pitch)
// - simulation params (eg q_vs_tau)
// - electronics params (eg preamp gain, trigger latency)


/*
**Wire geometry**
- Wire diameter (anode wire thickness) — typically 10–50 μm, affects field at wire surface and therefore gain
- Wire pitch — spacing between anode wires
- Wire length — the z dimension, affects capacitance per wire

**Gap geometry**
- Anode-cathode gap
- Guard wire configuration — wires at the edge of the array at different voltage to correct edge field distortions
- Field wire presence and pitch — some MWPCs interleave field wires between sense wires to shape the field

**Material**
- Wire material — gold-plated tungsten
- Cathode material and thickness — mylar
- frame - acrylic
- Gas purity — O₂ and H₂O contamination kill performance via electron attachment

**Gas system**
- Gas composition and ratio
- Gas pressure — usually 1 atm but some detectors run slightly overpressure to prevent air leaks
- Gas temperature — affects drift velocity and gain
// - Gas flow rate — how fast you flush the volume

**Electrical**
- Anode voltage
- Cathode voltage (usually ground, but not always)
- Guard wire voltage
- Per-wire capacitance — affects electronics noise
- Detector total capacitance

**Volume**
- Effective detector area?
- output: number of wires
- margin (or auto 3cm)
- 


**Derived / performance parameters** (outputs of simulation, not inputs)
- Gas gain
- Primary ionization per track
- Maximum drift time
- Position resolution
- Energy resolution
- Rate capability
*/

// gas flow rate only affects gain?
// do we need guard wires? just one on each side?
// guard wire voltage same as sense wire voltage?
// what determines per-wire capacitance
// what determines total detector capacitance
