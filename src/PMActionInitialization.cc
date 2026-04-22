#include "PMActionInitialization.hh"

#include "CosmicMuonGenerator.hh"

PMActionInitialization::PMActionInitialization() { }

PMActionInitialization::~PMActionInitialization() { }

void PMActionInitialization::BuildForMaster() const {

}

void PMActionInitialization::Build() const {
    // PMPrimaryGenerator *generator = new PMPrimaryGenerator();
    // SetUserAction(generator);

    SetUserAction(new CosmicMuonGenerator());
}