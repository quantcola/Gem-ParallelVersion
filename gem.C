// Alpha version of the gem simulation program
// The unit of length is [cm]
#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <time.h>
#include <experimental/filesystem> 

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TGeoManager.h>
#include <TGeoMaterial.h>
#include <TGeoMedium.h>
#include <TGeoVolume.h>
#include <TGeoBBox.h>
#include <TGeoTube.h>
#include <TGeoPcon.h>
#include <TGeoHalfSpace.h>
#include <TGeoMatrix.h>
#include <TGeoCompositeShape.h>

#include "Garfield/ComponentAnsys123.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/AvalancheMC.hh"
#include "Garfield/Random.hh"
#include "Garfield/Plotting.hh"
#include "Garfield/ViewFEMesh.hh"


using namespace Garfield;
using namespace std;

int processIndex = 1;    // The index of paralled calculation

int main() 
{ 
    plottingEngine.SetDefaultStyle();
    
    string workingDirectory=std::experimental::filesystem::current_path();
    const bool debug = true;

    /************** Load the field map.************/
    ComponentAnsys123* fm = new ComponentAnsys123();
    fm->Initialise("ELIST.lis", "NLIST.lis", "MPLIST.lis", "PRNSOL.lis", "mm");
    fm->EnableMirrorPeriodicityX();
    fm->EnableMirrorPeriodicityY();
    fm->PrintRange();

    /************** Dimensions of the GEM [cm] ****************/
    const double pitch = 0.014;
    const double kapton = 50.e-4;
    const double metal = 5.e-4;



    /********** Setup the gas. ***************/
    MediumMagboltz* gas = new MediumMagboltz(); // Initialize a gas object
    gas->SetComposition("ar", 80., "co2", 20.); // Specify the gas mixture (Ar/CO2 80:20)
    gas->SetTemperature(293.15);                // Set the temperature (K) ---- Room temperature  
    gas->SetPressure(760.);                     // Set the pressure (Torr)
    gas->EnableDebugging();
    gas->Initialise();
    gas->DisableDebugging();

    //Set the Penning transfer efficiency
    const double rPenning = 0.51;
    const double lambdaPenning = 0.;
    gas->EnablePenningTransfer(rPenning, lambdaPenning, "ar");
    
    // Load the ion mobilities. 
    // (We don't have accurate data for the mixture gas, so we use the data for Ar instead)
    const string path = getenv("GARFIELD_HOME");
    gas->LoadIonMobility(path + "/Data/IonMobility_Ar+_Ar.txt"); 

    
  
    // Associate the gas with the corresponding field map material.
    const int nMaterials = fm->GetNumberOfMaterials();
    for (int i = 0; i < nMaterials; ++i) 
    {
        const double eps = fm->GetPermittivity(i);
        // If the dielectric constant equals to 1, assign the gas material to the medium
        // Garfield++ will set the gas medium to the drift medium automatically
        if (fabs(eps - 1.) < 1.e-3) 
        {
            fm->SetMedium(i, gas);
        }
    }
    fm->PrintMaterials();

    // Create the sensor.
    Sensor* sensor = new Sensor();
    sensor->AddComponent(fm);
    sensor->SetArea(-5 * pitch, -5 * pitch, -0.1,
                     5 * pitch,  5 * pitch,  0.1);

    AvalancheMicroscopic* aval = new AvalancheMicroscopic();
    AvalancheMC* drift = new AvalancheMC();
    aval->SetSensor(sensor);
    drift->SetSensor(sensor);
    drift->SetDistanceSteps(2.e-4);

    /*********** Initialize the drift line plotting system ******************/
    const bool plotDrift = true;
    ViewDrift* driftView = new ViewDrift();
    if (plotDrift) 
    {
        driftView->SetArea(-5 * pitch, -5 * pitch, -0.1,
                            5 * pitch,  5 * pitch,  0.1); 
        aval->EnablePlotting(driftView);
        drift->EnablePlotting(driftView);
    }

    /*********** Create files to which data will be written **************/
    ofstream summaray_electron;
    ofstream time_electron;
    ofstream position_ion;
    ofstream position_electron;
    ofstream gain_log;
    ofstream gain_effective_log;
    summaray_electron.open ("summaray_electron.txt",ios::app);
    time_electron.open ("time_electron.txt",ios::app);
    position_ion.open ("position_ion.txt",ios::app);
    position_electron.open ("position_electron.txt",ios::app);
    gain_log.open("gain.txt",ios::app);
    gain_effective_log.open("gain_effective.txt",ios::app);

    int gain = 0;
    int gain_effective=0;

    // Set number of run
    const int nEvents = 32;
    for (int i = nEvents; i--;)  
    {
        if (debug || i % 10 == 0)
        {
            std::cout << i << "/" << nEvents << "\n";
        } 

        float timeused = (float)clock()/CLOCKS_PER_SEC; // The time between beginning of the simulation and now.
        cout << "Avalanche " << i << " started at " << timeused << " sec" << endl;


        // Set needed constants
        double sumElectronsTotal = 0.;      // The number of all the electrons
        double sumElectronsPlastic = 0.;    // The number of electrons which hit the kapton layer
        double sumElectronsUpperMetal = 0.; // The number of electrons which hit the upper metal layer
        double sumElectronsLowerMetal = 0.; // The number of electrons which hit the lower metal layer
        double sumElectronsTransfer = 0.;   // The number of electrons which is captured by our detector
        double sumElectronsOther = 0.;      // The number of other electrons.
        int NEE = 0;
        int EMUGC = 0;
        int EMLGC = 0;
        int EMIC = 0;
        int EMUG = 0;
        int EMLG = 0;
        int EMI = 0;

        // Initial position of the electron in this simulation
        double x0 = -pitch / 2. + RndmUniform() * pitch;
        double y0 = -pitch / 2. + RndmUniform() * pitch;
        double z0 = 0.02;
        double t0 = 0.;
        double e0 = 0.1;

        // Run electron avalanche
        aval->AvalancheElectron(x0, y0, z0, t0, e0, 0., 0., 0.);
        int ne = 0, ni = 0;
        aval->GetAvalancheSize(ne, ni);
        const int np = aval->GetNumberOfElectronEndpoints();
        cout << "The number of electrons in one run is: " << np << endl;
        double xe1, ye1, ze1, te1, e1;
        double xe2, ye2, ze2, te2, e2;
        double xi1, yi1, zi1, ti1;
        double xi2, yi2, zi2, ti2;
        int status;
        int statuss;

        // Filtrate electrons endpoints data
        for (int j = np; j--;) 
        {
            aval->GetElectronEndpoint(j, xe1, ye1, ze1,te1,e1,
                                         xe2, ye2, ze2,te2,e2, status);

            position_electron << xe1 << "\t" << ye1<< "\t" << ze1<<"\t"<< te1<< "\t" << e1 << "\t"
                   << xe2 << "\t" << ye2<< "\t" << ze2<<"\t"<< te2<< "\t" << e2 << "\t" 
                   << status << "\n";

            sumElectronsTotal += 1.;
            if (ze2 > -kapton / 2. && ze2 < kapton / 2.) 
            {
                // The electron hits the kapton layer
                sumElectronsPlastic += 1.; 
            } else if (ze2 >= kapton / 2. && ze2 <= kapton / 2. + metal) 
            {
                // The electron hits the upper layer
                sumElectronsUpperMetal += 1.;
            } else if (ze2 <= -kapton / 2. && ze2 >= -kapton / 2. - metal) 
            {
                // The electron hits the lower layer
                sumElectronsLowerMetal += 1.;
            } else if (ze2 < -0.09) 
            {
                // The electron enters the induction region
                sumElectronsTransfer += 1.;
                time_electron << te2 << "\n";
                NEE+= 1;
                if (ze1 > 0 && ze1 < kapton / 2. + metal) 
                {
                    // electron multiplication in upper gem collected
                    EMUGC+= 1;
                }
                if (ze1 > -kapton / 2. - metal && ze1 <= 0) 
                {
                    // electron multiplication in lower gem collected
                    EMLGC+= 1;
                }
                if (ze1 <= -kapton / 2. - metal && ze1 > -0.09) 
                {
                    // electron multiplication in induction collected
                    EMIC+= 1;
                }
            } else 
            {
                sumElectronsOther += 1.;
            }   

            if (ze1 > 0 && ze1 < kapton / 2. + metal) 
            {
                // electron multiplication in upper gem
                EMUG+= 1;
            }
            if (ze1 > -kapton / 2. - metal && ze1 <= 0) 
            {
                // electron multiplication in lower gem
                EMLG+= 1;
            }
            if (ze1 <= -kapton / 2. - metal && ze1 > -0.13) 
            {
                // electron multiplication in induction
                EMI+= 1;
            }

            drift->DriftIon(xe1, ye1, ze1, te1);
            drift->GetIonEndpoint(0, xi1, yi1, zi1, ti1,
                                     xi2, yi2, zi2, ti2, statuss);

            position_ion << xi1 << "\t" << yi1 << "\t" << zi1 << "\t" << ti1 << "\t" 
                   << xi2 << "\t" << yi2 << "\t" << zi2 << "\t" << ti2 << "\t" 
                   << statuss << "\n";

            
        }

        gain+=np;
        gain_effective += sumElectronsTransfer;

        summaray_electron << "\t" << sumElectronsTotal      << "\t" << "\t" 
                        << sumElectronsTransfer   << "\t" << "\t"  
                        << sumElectronsOther      << "\t" << "\t"  
                        << sumElectronsPlastic    << "\t" << "\t"  
                        << sumElectronsUpperMetal << "\t" << "\t"  
                        << sumElectronsLowerMetal << "\n";

        cout << "Event"  << "\t" 
             << "TotalE" << "\t" << "\t" 
             << "TransE" << "\t" << "\t" 
             << "OtherE" << "\t" << "\t" 
             << "PlastE" << "\t" << "\t" 
             << "UpMetE" << "\t" << "\t" 
             << "LoMetE \n";

        cout << i << "\t" << sumElectronsTotal      << "\t" << "\t" 
                          << sumElectronsTransfer   << "\t" << "\t" 
                          << sumElectronsOther      << "\t" << "\t" 
                          << sumElectronsPlastic    << "\t" << "\t" 
                          << sumElectronsUpperMetal << "\t" << "\t" 
                          << sumElectronsLowerMetal << "\n";
    }

    gain/=nEvents;
    cout << "The gain of this gem is: " << gain << endl;
    gain_log << gain << endl;
    gain_effective/=nEvents;
    cout << "The effective gain of this gem is: " << gain_effective << endl;
    gain_effective_log << gain_effective << endl;

    // Close data files
    summaray_electron.close();
    time_electron.close();

    position_electron.close();
    position_ion.close();
    gain_log.close();
    gain_effective_log.close();

    cout << "***************** One thread END *****************\n";

}
