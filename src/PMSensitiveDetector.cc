#include "PMSensitiveDetector.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
// #include "G4cout.hh"

PMSensitiveDetector::PMSensitiveDetector(G4String name)
    : G4VSensitiveDetector(name) {
    edepLog.open("/home/charles/geant4test2/proj1/output/charges.txt");
}

void PMSensitiveDetector::Initialize(G4HCofThisEvent *hce) {
    edep = 0.0;
}

G4bool PMSensitiveDetector::ProcessHits(G4Step *step, G4TouchableHistory *hist) {
    edep += step->GetTotalEnergyDeposit();

    return true;
}

void PMSensitiveDetector::EndOfEvent(G4HCofThisEvent*) {
    G4cout << "EndOfEvent, total edep = " << edep << G4endl;
    // Skip events with no deposition (muon missed the gas volume entirely —
    // possible for large-angle tracks spawned near the detector edge)
    if (edep <= 0.0) return;

    // -----------------------------------------------------------------------
    // Convert energy deposition to primary ionization charge
    //
    // W-value for Ar/CO2 75:25: mean energy to create one ion pair = 26 eV
    // This is the effective W including excitation losses, well measured for
    // this mixture. Geant4's edep is in MeV.
    //
    // n_pairs = edep / W
    // Q [fC]  = n_pairs * e  =  edep[MeV] / 26e-6[MeV]  *  1.6e-4[fC]
    //
    // 1 electron charge = 1.602e-19 C = 1.602e-4 fC
    // -----------------------------------------------------------------------
    const G4double W_eV      = 26.0 * eV;          // W-value in Geant4 units
    G4double       nPairs    = edep / W_eV;
    G4double       charge_fC = nPairs * (1.6e-4);  // fC per electron

    // print raw
    edepLog << charge_fC << "\n";
}

PMSensitiveDetector::~PMSensitiveDetector() {
    edepLog.close();
}