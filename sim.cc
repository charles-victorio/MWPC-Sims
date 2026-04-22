#include <iostream>

#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "PMPhysicsList.hh"
#include "PMDetectorConstruction.hh"
#include "PMActionInitialization.hh"

int main(int argc, char** argv) {
    #ifdef G4MULTITHREADED
        G4MTRunManager *runManager = new G4MTRunManager();
        runManager->SetNumberOfThreads(1); // easier to handle writing to file
    #else
        G4RunManager *runManager = new G4RunManager();
    #endif
    
    runManager->SetUserInitialization(new PMPhysicsList());
    runManager->SetUserInitialization(new PMDetectorConstruction());
    runManager->SetUserInitialization(new PMActionInitialization());
    
    G4UImanager *UImanager = G4UImanager::GetUIpointer();

    if (argc == 1) { // no arguments = interactive mode
        G4UIExecutive *ui = new G4UIExecutive(argc, argv);
        G4VisManager *visManager = new G4VisExecutive();
        visManager->Initialize();


        UImanager->ApplyCommand("/control/execute vis.mac");

        ui->SessionStart();

        delete ui;
        delete visManager;
    } else { // custom macro
        G4String macro = argv[1];
        UImanager->ApplyCommand("/control/execute " + macro);
    }

    delete runManager;
    return 0;
}