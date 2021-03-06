#include "TSystem.h"
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TClonesArray.h"
#include "TBranch.h"
#include "TLorentzVector.h"
#include "TParameter.h"
#include "TRandom3.h"
#include <iostream>
#include <cmath>
#include <exception>


using namespace std;

float dphi(float phi1, float phi2){     //define change in angle and the angles
    float delta = (phi1-phi2);
    if (delta < -1)
        delta  += 2*TMath::Pi();        //adjusts range so angles outside of 2pi
                                        //circumference are put in
    if (delta > (-1 + 2*TMath::Pi()))
        delta = delta - 2*TMath::Pi();
    return delta;
}


int main(int argc, const char** argv)
{
    const char *defaults[6] = {"readPythiaTree","test.root","in"}; //Don't know what this sets up
    if ( argc==1 ) {
        argv=defaults;
        argc=3;
    }
    TString InFileName = argv[1];
    TString OutFileName = argv[2];
    
    TFile *_file0 = TFile::Open(InFileName);        //reads tree
    TTree* tree = (TTree*)gROOT->FindObject("tree");
    
    //read collections
    TClonesArray *HardPartons = new TClonesArray("TLorentzVector");
    TBranch* bHardPartons = tree->GetBranch("HardPartons");
    bHardPartons->SetAddress(&HardPartons);
    TClonesArray *HardPartonNames = new TClonesArray("TParameter<int>");
    TBranch* bHardPartonNames = tree->GetBranch("HardPartonNames");
    bHardPartonNames->SetAddress(&HardPartonNames);
    TClonesArray *particles = new TClonesArray("TLorentzVector");
    TBranch* bparticles = tree->GetBranch("Particles");
    bparticles->SetAddress(&particles);
    TClonesArray *ParticlesandBackground = new TClonesArray("TLorentzVector");
    
    
    
    
    
    //First create the histograms that will be filled with >> command
    TString histname("hHardPartonspT");
    TH1F* hHardPartonspT = new TH1F(histname,histname,100,0,100);  //defines new histogram for transverse momentum
    histname.Prepend("sqrt(HardPartons.fP.fX*HardPartons.fP.fX+HardPartons.fP.fY*HardPartons.fP.fY)>>");
    tree->Draw(histname);           //draws histogram
    
    histname = "hHardPartonsName";
    TH1F* hHardPartonsName = new TH1F(histname,histname,40,-9.5,30.5); //defines new histogram for I don't know why
    histname.Prepend("HardPartonNames.fVal>>");
    tree->Draw(histname);
    
    //Next book the histograms that will be filled event by event
    histname = "hParticlepT";
    TH1F* hParticlepT = new TH1F(histname,histname,100,0,100);  //new histogram for particle transverse momentum
    histname = "hParticleEtaPhi";
    TH2F* hParticleEtaPhi = new TH2F(histname,histname,100,-1,1,100,-6.2,6.2); //new 2D histogram for angular and I think lengthwise distribution
    
    
    
    
    
    
    //This is my addition:
    
    
    histname = "hDeltaPhiMomentum";
    TH2F * hDeltaPhiMomentum[4];        //Defines an array to reduce code from 16 loops to four
    for(int j=0; j<4; j++){
        hDeltaPhiMomentum[j]= new TH2F(Form("hDeltaPhiMomentum%i",j),Form("hDeltaPhiMomentum%i",j),100,-1,2*TMath::Pi()-1,100,-5,5);
        ; //Form command takes care of naming, gives new name each loop
    }
    
    
    TRandom3* random = new TRandom3;            //Sets up for background
    random->SetSeed(5000);
    
    
    Int_t entries = tree->GetEntries();
    cout<<entries<<endl;
    for (Int_t iev = 0;iev<entries;iev++){   //This is where you run over events
        //Looping over number of events
        if (iev % 100 == 0)
            cout<<iev<<endl;
        
        ParticlesandBackground->Clear();        //resets value
            
            //Attempt to add background
        int ipart_bkgd = 0;
        Int_t nparticles = particles->GetEntriesFast();     //Gets information for particles
        for (int ip = 0; ip<nparticles;ip++){
            //Find each particle
            TLorentzVector* particle = (TLorentzVector*)particles->At(ip);
            new ((*ParticlesandBackground)[ipart_bkgd]) TLorentzVector(*particle);
            ipart_bkgd++;
        }

        
        
        
        
        
        int Ntracks = 120;                           //Number of tracks; i set it low but should be 1200 ultimately
        float pT0 = 0.500;
        
       
        
            double rand; double pT; double phi; double eta;
                for (int i = 0;i<Ntracks;i++){
                rand = random->Rndm();
                pT = -pT0*log(1 - rand);
                rand = random->Rndm();
                phi = TMath::Pi()*2.*rand;
                rand = random->Rndm();
                eta = 2*(rand-0.5);
                TLorentzVector* t = new TLorentzVector();
                t->SetPtEtaPhiM(pT,eta,phi,.135);          //last number is mass (.135 is pion i think)
                new ((*ParticlesandBackground)[ipart_bkgd]) TLorentzVector(*t);
                    ipart_bkgd++;
            }
            
        //Get all the particles in the event
        tree->GetEntry(iev);
        bHardPartons->GetEntry(iev);
        bHardPartonNames->GetEntry(iev);
        bparticles->GetEntry(iev);
        
        float Ptrig = -1;                       //Cat's design for finding Ptrig
        int trackn = -1;                        //set negative so when it fills for larger values, we will never miss a possible value
        int index = -1;
        
        //loop over final state particles
        for (int ip = 0; ip<(nparticles+Ntracks);ip++){
                                                            //Find each particle
            TLorentzVector* ParticleBackgroundVector = (TLorentzVector*)ParticlesandBackground->At(ip);
                                                            //Manipulate particles and fill histograms
            //hParticlepT->Fill(particle->Pt());
            hParticleEtaPhi->Fill(ParticleBackgroundVector->Eta(),ParticleBackgroundVector->Phi());
            
            
            //My addition
            //This figures out a Ptrigger, which will be the highest momentum, for comparison to the others
            
            if (ParticleBackgroundVector->Pt() > Ptrig) {
                Ptrig = ParticleBackgroundVector->Pt();
                trackn = ip;
            }
        }
        
        //Cout statements to find which event is failing (In terminal Event is failing)
    
        hParticlepT->Fill(Ptrig);
        
        
        if (Ptrig>2.5 && Ptrig<3.0)             //set up cut in trigger momentum
            index = 0;
        
        if (Ptrig>3.0 && Ptrig<4.0)
            index = 1;
        
        if (Ptrig>4.0 && Ptrig<6.0)
            index = 2;
        
        if (Ptrig>6.0 && Ptrig<10.0)
            index = 3;
        
        for (int ip = 0; ip<(nparticles+Ntracks);ip++) {
            
            
            TLorentzVector* particleA = (TLorentzVector*)ParticlesandBackground->At(ip);
            for (int jp = 0; jp<(nparticles+Ntracks);jp++) {
                
                if (index==-1)  //gets rid of -1 values
                    continue;
                
         
                TLorentzVector* particleB = (TLorentzVector*)ParticlesandBackground->At(jp);
                
                if(ip == jp)
                    continue;
                
                
                float delta = particleA->Phi() - particleB->Phi();  //redefined from above. Dont know why i did it but ir works so fine.
                
                
                if (delta<-1)                       //adjusts data that is outside of display range and puts it in (Physically, no angles are outside of the 2pi range so we're just helping the code un-confuse itself
                    delta = delta + 2*TMath::Pi();                  //fixes data outside range again
                if (delta>2*TMath::Pi()-1)
                    delta = delta - 2*TMath::Pi();
                
                hDeltaPhiMomentum[index]->Fill(delta,particleA->Pt());
                
            }}
    } //end of event loop
    
   
  
    //cout<<"Finish "<<endl;
    
    
    
    //Open output file and save the histograms
    TFile *file1 = TFile::Open(OutFileName.Data(),"RECREATE");
    
    hHardPartonspT->Write();
    hHardPartonsName->Write();
    hParticlepT->Write();
    hParticleEtaPhi->Write();
    
    //My addition
    hDeltaPhiMomentum[0]->Write();
    hDeltaPhiMomentum[1]->Write();
    hDeltaPhiMomentum[2]->Write();
    hDeltaPhiMomentum[3]->Write();
}
