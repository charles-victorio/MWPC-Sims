#include "PMDetectorConstruction.hh"

#include "G4UserLimits.hh"
#include "G4Region.hh"
#include "G4SDManager.hh"

#include "PMSensitiveDetector.hh"


PMDetectorConstruction::PMDetectorConstruction() {}

PMDetectorConstruction::~PMDetectorConstruction() {}

G4VPhysicalVolume *PMDetectorConstruction::Construct() {
    G4bool checkOverlaps = true;
    
    G4NistManager *nist = G4NistManager::Instance();
    G4Material *worldMat = nist->FindOrBuildMaterial("G4_AIR");
    // G4Material *leadMat = nist->FindOrBuildMaterial("G4_Pb");
    // G4Material *detMat = nist ->FindOrBuildMaterial("G4_SODIUM_IODIDE");
    G4Material *Ar = nist->FindOrBuildMaterial("G4_Ar");
    G4Material *CO2 = nist->FindOrBuildMaterial("G4_CARBON_DIOXIDE");
    
    G4double gasDensity = 1.822e-3 * g/cm3; // find ar/co2 75/25 at stp
    G4Material *arco2 = new G4Material("ArCO2", gasDensity, 2);
    arco2->AddMaterial(Ar, 75 * perCent);
    arco2->AddMaterial(CO2, 25 * perCent);
    
    G4double xWorld = 1. * m;
    G4double yWorld = 1. * m;
    G4double zWorld = 1. * m;
    
    G4Box *solidWorld = new G4Box("solidWorld", 0.5 * xWorld, 0.5 * yWorld, 0.5 * zWorld);
    G4LogicalVolume *logicWorld = new G4LogicalVolume(solidWorld, worldMat, "logicWorld");
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0, G4ThreeVector(0., 0., 0.), logicWorld, "physWorld", 0, false, 0, checkOverlaps);
    
    G4Box* gasBox = new G4Box("gas", 5*cm, 5*cm, 3*mm);
    G4LogicalVolume *gasLV = new G4LogicalVolume(gasBox, arco2, "gas");
    G4Region *gasRegion = new G4Region("GasRegion");
    gasLV->SetRegion(gasRegion);
    gasRegion->AddRootLogicalVolume(gasLV);
    gasLV->SetUserLimits(new G4UserLimits(0.5*mm));
    G4VPhysicalVolume *gasPV = new G4PVPlacement(0, G4ThreeVector(0., 0., 0.), gasLV, "gasPV", logicWorld, false, 0, checkOverlaps);
    
    logicDetector = gasLV;

    // G4double leadThickness = 2. * mm;
    // G4double leadSize = 5. * cm;
    // G4Box *solidLead = new G4Box("solidLead", 0.5 * leadSize, 0.5 * leadSize, 0.5 * leadThickness);
    // G4LogicalVolume *logicLead = new G4LogicalVolume(solidLead, leadMat, "logicLead");
    // G4VPhysicalVolume *physLead = new G4PVPlacement(0, G4ThreeVector(0., 0., 5. * cm), logicLead, "physLead", logicWorld, false, 0, checkOverlaps);
    
    // G4VisAttributes *leadVisAtt = new G4VisAttributes(G4Color(1.0, 0.0, 0.0, 0.5));
    // leadVisAtt->SetForceSolid(true);
    // logicLead->SetVisAttributes(leadVisAtt);
    
    // G4double detSize = 5.0 * cm;
    // G4Box *solidDetector = new G4Box("solidDetector", detSize, detSize, detSize);
    // logicDetector = new G4LogicalVolume(solidDetector, detMat, "logicDetector");
    // G4VPhysicalVolume *physDetector = new G4PVPlacement(0, G4ThreeVector(0., 0., 10.5 * cm), logicDetector, "physdetector", logicWorld, false, checkOverlaps);

    // G4VisAttributes *detVisAtt = new G4VisAttributes(G4Color(1.0, 1.0, 0.0, 0.5));
    // detVisAtt->SetForceSolid(true);
    // logicDetector->SetVisAttributes(detVisAtt);
    
    return physWorld;
    
}

void PMDetectorConstruction::ConstructSDandField() {
    auto *sdManager = G4SDManager::GetSDMpointer();
    sdManager->SetVerboseLevel(1);

    PMSensitiveDetector *sd = new PMSensitiveDetector("GasSD");
    sdManager->AddNewDetector(sd);
    logicDetector->SetSensitiveDetector(sd);
}