#pragma once

#include "G4VModularPhysicsList.hh"

class PMPhysicsList : public G4VModularPhysicsList {
public:
    PMPhysicsList();
    ~PMPhysicsList() override;

    void ConstructParticle() override;
    void ConstructProcess()  override;
    void SetCuts()           override;

private:
    // Attaches G4PAIPhotModel to all relevant particles in GasRegion
    void AddPAIModel();

    // Logs step-limit reminder — actual limit set via G4UserLimits in DetectorConstruction
    void SetStepLimits();
};
