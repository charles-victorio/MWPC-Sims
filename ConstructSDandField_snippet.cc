// Add these includes at the top of PMDetectorConstruction.cc
#include "PMSensitiveDetector.hh"
#include "G4SDManager.hh"

// ---------------------------------------------------------------------------
// Paste this method into PMDetectorConstruction.cc
// fLogicDetector must already be set by Construct() before this runs.

void PMDetectorConstruction::ConstructSDandField()
{
    if (!fLogicDetector) {
        G4Exception("PMDetectorConstruction::ConstructSDandField",
                    "NullPointer", FatalException,
                    "fLogicDetector is null — Construct() must run first.");
        return;
    }

    // Create the SD and register it with the SD manager.
    // The name "GasSD" is arbitrary but must be unique across all SDs.
    auto* sdManager = G4SDManager::GetSDMpointer();
    sdManager->SetVerboseLevel(1);

    PMSensitiveDetector* sd = new PMSensitiveDetector("GasSD");
    sdManager->AddNewDetector(sd);

    // Attach to the gas logical volume
    fLogicDetector->SetSensitiveDetector(sd);
}
