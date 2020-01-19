void plotGain()
{
    TCanvas *c1=new TCanvas("c1","gain versus voltage",700,500);
    c1->SetGrid();

    //plot of gain
    const Int_t n1=11;
    Double_t voltage[]={350,370,390,410,430,450,470,490,51 0,530,550};
    Double_t gain[]={24,33,37,81,125,185,318,512,804,1273,2056};
    TGraph *gr1=new TGraph(n1,voltage,gain);
    gr1->SetMarkerColor(kBlue);
    gr1->SetMarkerStyle(20);
    gr1->SetLineColor(4);
    gr1->Draw();
    gr1->GetXaxis()->SetTitle("Voltage between gem foils [V]");
    gr1->GetYaxis()->SetTitle("gain");
    gr1->SetTitle("gain and effective gain of gem");


    //plot of effective gain
    const Int_t n2=11;
    Double_t gain_effective[]={8,11,12,25,37,53,87,133,209,311,489};
    TGraph *gr2=new TGraph(n2,voltage,gain_effective);
    gr2->SetLineWidth(3);
    gr2->SetMarkerStyle(21);
    gr2->SetLineColor(2);
    gr2->Draw("CP");

    TLegend *legend = new TLegend(.15,.7,.35,.85);
    legend->AddEntry(gr1,"gain");
    legend->AddEntry(gr2,"effective_gain");
    legend->Draw();

    gPad->SetLogy(1);

}