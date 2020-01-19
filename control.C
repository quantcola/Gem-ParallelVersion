#include<iostream>
#include <experimental/filesystem>
#include <thread>
#include <vector>
#include <fstream>

using namespace std;

void startThread(string command)
{
    system(command.c_str());
}

int main () 
{
    cout << "Please enter the number of threads we are going to use: " << endl;
    int numberOfThreads;
    cin >> numberOfThreads;

    string workingDirectory=std::experimental::filesystem::current_path();
    system("rm -r thread*");    //delete all the folders in the last simulation
    system("rm *.txt");     //delete all the log files in the last run
    vector<thread> threads;

    for(int i=0;i<numberOfThreads;i++)
    {   
        //create folders for each thread
        string mkdirCommand="mkdir ";
        string prefix="thread";
        prefix.append(to_string(i));
        mkdirCommand.append(prefix);
        system(mkdirCommand.c_str());

        //copy field maps to each folders
        string cpCommand="cp gem ELIST.lis MPLIST.lis NLIST.lis PRNSOL.lis thread";
        cpCommand.append(to_string(i));
        system(cpCommand.c_str());

        //run ./gem in parallel
        system(("cd thread"+to_string(i)).c_str());
        string gemCommand=workingDirectory +"/thread"+to_string(i)+"/gem";
        threads.push_back(thread(startThread,gemCommand));
    }

    for(auto& thread:threads)
    {
        thread.join();
    }

    cout << "***************************************************************************************************************" << endl;
    cout << "All the paralleled process has finishes! Below is the final result: " << endl;
    double gain=0;
    double gain_effective=0;
    ifstream readFile1;
    ifstream readFile2;
    readFile1.open("gain.txt");
    readFile2.open("gain_effective.txt");

    for(int i=0;i<numberOfThreads;i++)
    {
        string content;
        readFile1 >> content;
        gain+=atof(content.c_str());
        readFile2 >> content;
        gain_effective+=atof(content.c_str());
    }
    gain/=numberOfThreads;
    cout << "The final gain is: " << gain << endl;
    gain_effective/=numberOfThreads;
    cout << "The final effective gain is: " << gain_effective << endl;
}