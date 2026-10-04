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
#include <random>
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


// // iirc, meaningless outside of the particular instrumentation that the txt file is from
// bool readTransferFunction(Sensor &sensor) {
//     std::ifstream infile;
//     infile.open("../data/mdt_elx_delta.txt");
//     if (!infile) {
//         std::cerr << "Could not read delta response function.\n";
//         return false;
//     }
//     std::vector<double> times;
//     std::vector<double> values;
//     while (!infile.eof()) {
//         double t = 0., f = 0.;
//         infile >> t >> f;
//         if (infile.eof() || infile.fail()) break;
//         times.push_back(1.e3 * t);
//         values.push_back(f);
//     }
//     infile.close();
//     sensor.SetTransferFunction(times, values);
//     return true;
// }

int main(int argc, char** argv) {

    
    const int taskID = std::atoi(argv[1]);
    const double voltage = std::stod(argv[2]);
    // const int nTasks = std::atoi(argv[2]);
    // const int label = -14 + 2 * (taskID - 1);
    const int voltageInt = std::atoi(argv[2]); // hack
    const std::string output_dir = "../../output/charges_" + std::to_string(voltageInt) + "V/";
    
    // Garfield::RandomEngine.Seed(140805);
    // std::random_device rd;
    std::mt19937 gen(140805);

    MediumMagboltz gas;
    gas.LoadGasFile("../data/ar_75_co2_25.gas");
    gas.LoadIonMobility("../data/IonMobility_Ar+_Ar.txt");
    gas.SetMaxElectronEnergy(400.);
    gas.EnablePenningTransfer();


    // ViewMedium mediumView(&gas);
    // mediumView.PlotElectronVelocity('e');
    // mediumView.GetCanvas()->SaveAs("../output/electron_drift_velocity.png");

    ComponentAnalyticField cmp;
    cmp.SetMedium(&gas);
    const double wireDiameter = 20.e-4; // [cm]
    const double wireVoltage = voltage; // 2000.; // 2730.;
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
    // if (!readTransferFunction(sensor)) return 0;
    sensor.ClearSignal();
    
    // Set up Heed
    TrackHeed track(&sensor);
    track.SetParticle("muon");
    track.SetEnergy(170.e9);

    // RKF integration
    // DriftLineRKF drift(&sensor);
    // drift.SetGainFluctuationsPolya(0., 20000.);
    // drift.EnableIonTail(true); // this is default on. not calling this function means ion tail will be enabled

    AvalancheMicroscopic aval(&sensor);
    // aval.SetIonTransport(true);
    // how to make it handle ions? does it do that automatically? I assume it does since GetAvalancheSize gives you ni and sensor.GetIonInducedCharge works

    // disable entire trajectory tracking to save memory
    /*
    ViewDrift driftView;
    // drift.EnablePlotting(&driftView);
    aval.EnablePlotting(&driftView);
    track.EnablePlotting(&driftView);
    driftView.SetColourElectrons(3);
    driftView.SetColourIons(4);

    ViewCell cellView;
    cellView.SetCanvas(driftView.GetCanvas());
    cellView.SetComponent(&cmp);
    */

    // Simulate a track
    const double xmax = pitch / 2 - 0.01; // 0.14;

    std::uniform_real_distribution<double> xDist(-xmax, xmax);
    std::uniform_real_distribution<double> alphaDist(-M_PI/2.0, M_PI/2.0);
    std::uniform_real_distribution<double> rejDist(0.0, 1.0);


    int tracks_per_executable = 20;
    int trackID = (taskID - 1) * tracks_per_executable;
    for (int i = 0; i < tracks_per_executable; i++) {
        const std::string fname = output_dir + "charges_track_" + std::to_string(voltageInt) + "V_" + std::to_string(trackID) + ".csv";
        std::ofstream charges_file(fname);
        // std::ofstream charges_file("../output3000/c1.csv");

        // const double x0 = 0.14;
        // const double x0 = -0.14 + 0.02 * static_cast<double>(taskID - 1);
        const double x0 = xDist(gen);
        const double y0 = gap - 1.e-4;

        double alpha;
        while (true) {
            alpha = alphaDist(gen);
            double c = std::cos(alpha);
            if (rejDist(gen) < c * c * c) break;  // accept with prob cos^3(alpha)
        }

        double dx =  std::sin(alpha);
        double dy = -std::cos(alpha);   // downward


        const std::string label = output_dir + "track_" + std::to_string(voltageInt) + "V_" + std::to_string(trackID) + "_info.txt";
        std::ofstream track_info_file(label.c_str());
        track_info_file << x0 << " " << y0 << " " << dx << " " << dy << " " << voltage << " " << trackID << std::endl;
        track_info_file.close();

        
        track.NewTrack(x0, y0, 0, 0, dx, dy, 0);
        std::cout << "number of clusters: " << track.GetClusters().size() << std::endl;
        int cluster_i = 0;
        for (const auto &cluster : track.GetClusters()) {
            int electron_i = 0;
            for (const auto &electron : cluster.electrons) {
                // drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);

                aval.AvalancheElectron(electron.x, electron.y, electron.z, electron.t, 0, 0, 0, 0);
                int ne, ni;
                aval.GetAvalancheSize(ne, ni);
                charges_file << trackID << "," << ne << "," << ni << std::endl;
                // track_info_file << "Cluster #" << cluster_i << " electron #" << electron_i << " has ne = " << ne << std::endl; 
                electron_i++;
            }
            cluster_i++;
        }


        // sensor.GetIonSignal("s", i); // in loop

        /*
        // draw the mwpc with the track and the drifting
        cellView.Plot2d();
        driftView.Plot(true, false);
        // std::string imgFname1 = "../output4000/drift_lines" + std::to_string(label) + ".png";
        // driftView.GetCanvas()->SaveAs(imgFname1.c_str());
        driftView.GetCanvas()->SaveAs("../output3000/drift_lines.png");
        
        ViewSignal signalView(&sensor);
        signalView.PlotSignal("s");
        std::string imgFname2 = "../output4000/signal" + std::to_string(label) + ".png";
        signalView.GetCanvas()->SaveAs(imgFname2.c_str());
        */

        // sensor.ConvoluteSignals();
        // int nt = 0;
        // if (!sensor.ComputeThresholdCrossings(-2., "s", nt)) continue;
        // if (sensor.ComputeThresholdCrossings(-2., "s", nt)) {
        //     ViewSignal signalView(&sensor);
        //     signalView.PlotSignal("s");
        //     std::string imgFname3 = "../output2/signal_convolved" + std::to_string(label) + ".png";
        //     signalView.GetCanvas()->SaveAs(imgFname3.c_str());
        // }

        
        charges_file.close();
        trackID++;
    }

}