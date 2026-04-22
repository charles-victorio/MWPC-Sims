// use avalanchemicroscopic! Next step:
// copy this code, edit it to change it to avalanche microscopic
// extract gain, induced change, drift time spread. as shown by claude
// then find out how to get Q_vs_tau

// 1. [x] change geometry to mwpc
// 2. change transport method to aval micro
//     - add a way to switch
// 3. change gas to 75 25
//     - what temperature should we set it to? probably room temp / outside temp 


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
    const double wireDiameter = 20.e-4; // [cm]
    const double wireVoltage = 2000.; // 4000.; // 2730.;
    const double gap = 0.6;
    cmp.AddWire(0, 0, wireDiameter, wireVoltage, "s");
    cmp.AddPlaneY(gap, 0., "top");
    cmp.AddPlaneY(-gap, 0., "bot");
    const double pitch = 0.3;
    cmp.SetPeriodicityX(pitch);

    Sensor sensor(&cmp);
    sensor.AddElectrode(&cmp, "s");
    const double tstep = 0.5;
    const double tmin = -0.5 * tstep;
    const std::size_t nbins = 1000;
    sensor.SetTimeWindow(tmin, tstep, nbins);
    if (!readTransferFunction(sensor)) return 0;
    sensor.ClearSignal();
    
    // Set up Heed
    TrackHeed track(&sensor);
    track.SetParticle("muon");
    track.SetEnergy(170.e9);

    // RKF integration
    DriftLineRKF drift(&sensor);
    drift.SetGainFluctuationsPolya(0., 20000.);
    drift.EnableIonTail(true); // this is default on. not calling this function means ion tail will be enabled

    // AvalancheMicroscopic aval(&sensor);
    // aval.SetIonTransport(true);
    // how to make it handle ions? does it do that automatically? I assume it does since GetAvalancheSize gives you ni and sensor.GetIonInducedCharge works

    ViewDrift driftView;
    drift.EnablePlotting(&driftView);
    // aval.EnablePlotting(&driftView);
    track.EnablePlotting(&driftView);
    driftView.SetColourElectrons(3);
    driftView.SetColourIons(4);

    ViewCell cellView;
    cellView.SetCanvas(driftView.GetCanvas());
    cellView.SetComponent(&cmp);

    // Simulate a track
    const double x0 = 0.14;
    const double y0 = gap - 1.e-4;
    track.NewTrack(x0, y0, 0, 0, 0, -1, 0);
    std::cout << "number of clusters: " << track.GetClusters().size() << std::endl;
    std::size_t nElectronsTracked = 0;
    std::size_t cutoff = 5; // only gets earlier cluster
    for (const auto &cluster : track.GetClusters()) {
        Cluster:
        for (const auto &electron : cluster.electrons) {
            drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
            
            // if (nElectronsTracked >= cutoff) {
            //     goto EnoughAlready;
            // }
            // aval.AvalancheElectron(electron.x, electron.y, electron.z, electron.t, 0, 0, 0, 0);
            // std::cout << "drifting electron " << nElectronsTracked << " at time " << electron.t << std::endl;
            // int ne, ni;
            // aval.GetAvalancheSize(ne, ni);
            // std::cout << "ne: " << ne << ", ni: " << ni << std::endl;
            // nElectronsTracked++;
            // if (ne > 1) {
            //     goto Cluster; // only show at most one representative good (big ne) electron from each cluster
            // }
        }
    }
    EnoughAlready:

    // sensor.GetIonSignal("s", i); // in loop

    // draw the mwpc with the track and the drifting
    cellView.Plot2d();
    driftView.Plot(true, false);
    std::cout << "here 1" << std::endl;
    driftView.GetCanvas()->SaveAs("../output/drift_lines.png");

    std::cout << "a" << std::endl; std::exit(0);

    ViewSignal signalView(&sensor);
    std::cout << "b" << std::endl; std::exit(0);
    signalView.PlotSignal("s");
    std::cout << "c" << std::endl; std::exit(0);
    std::cout << "here 2" << std::endl;
    std::cout << "d" << std::endl; std::exit(0);
    signalView.GetCanvas()->SaveAs("../output/signal.png");
    std::cout << "here 3" << std::endl;
    std::cout << "e" << std::endl; std::exit(0);

    // sensor.ConvoluteSignals();
    // int nt = 0;
    // // if (!sensor.ComputeThresholdCrossings(-2., "s", nt)) continue;
    // if (sensor.ComputeThresholdCrossings(-2., "s", nt)) {
    //     ViewSignal signalView(&sensor);
    //     signalView.PlotSignal("s");
    //     signalView.GetCanvas()->SaveAs("../output/signal.png");
    // }

    // if the table gets expanded, the program doesn't terminate without ctrl-c
    std::exit(0); // also doesnt work
}