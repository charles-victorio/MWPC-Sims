#pragma once

#include <fstream>

#include "G4VSensitiveDetector.hh"
#include "Hit.hh"

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

class PMSensitiveDetector : public G4VSensitiveDetector {
public:
    explicit PMSensitiveDetector(G4String name);
    ~PMSensitiveDetector() override;

    void Initialize(G4HCofThisEvent *hce) override;
    G4bool ProcessHits(G4Step *step, G4TouchableHistory *hist) override;
    void EndOfEvent(G4HCofThisEvent *hce) override;

private:
    G4double edep; // remember, edep is for the entire event
    std::ofstream edepLog;
};