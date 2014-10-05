#include "HCUBE_Defines.h"
#include "JGTL_CommandLineParser.h"


#include "HCUBE_ExperimentRun.h"

#include "Experiments/HCUBE_Experiment.h"
//Tal: 3-10 add next line
#include "Experiments/HCUBE_SoftbotsExperiment.h"

//#include "Experiments/HCUBE_FindClusterExperiment.h"
//#include "Experiments/HCUBE_CheckersExperiment.h"

//Tal: removed ONAMA 3-10

#define PRINT_GENCHAMPFITNESSVIAPOSTHOCEVAL (0)

//Tal: next commented 3-10
//#define EPOCHS_PER_PRINT (100000)

//Tal: next function not used? 3-10
void printNetwork(NEAT::FastNetwork<double> &testNetwork)
{
	return;
	
	cout << "Network links:\n";
	cout << testNetwork.getLink("Input0","Hidden1")->weight << endl;
	cout << testNetwork.getLink("Input0","Hidden2")->weight << endl;
	cout << endl;
	
	cout << testNetwork.getLink("Input1","Hidden1")->weight << endl;
	cout << testNetwork.getLink("Input1","Hidden2")->weight << endl;
	cout << endl;
	
	cout << testNetwork.getLink("Input2","Hidden1")->weight << endl;
	cout << testNetwork.getLink("Input2","Hidden2")->weight << endl;
	cout << endl;
	
	cout << testNetwork.getLink("Hidden0","Output0")->weight << endl;
	cout << testNetwork.getLink("Hidden1","Output0")->weight << endl;
	cout << testNetwork.getLink("Hidden2","Output0")->weight << endl;
	cout << endl;
	
	CREATE_PAUSE("");
}
using  namespace HCUBE;
int HyperNEAT_main(int argc,char **argv)
{
	CommandLineParser commandLineParser(argc,argv);
	try
	{
		if(commandLineParser.HasSwitch("-ORG"))
		{
			cout << "Switch -ORG\n";
			// get the required arguments
			//StringType org1, org2;
			string org1 = commandLineParser.GetSafeArgument("-ORG", 0,"org1");
			cout << "first=" << org1 << endl;
			string org2 = commandLineParser.GetSafeArgument("-ORG", 1,"org2");
			cout << "second=" << org2 << endl;
			//test org1 and org2
			// if org1 and org2 are absent -> Individual
			if (org1.empty() && org2.empty()) 
			{
				cout << "new individual";
			}
			// if org2 is absent -> Individual + Mutation
			if (!org1.empty() && org2.empty())
			{
				cout << "mutated individual";
			}
			// if org1 and org2 are nonempty -> parents Xover plus mutation
			if (!org1.empty() && !org2.empty())
			{
				cout << "new offspring";
			}
		}
		if(commandLineParser.HasSwitch("-I")) //Tal: for parameter[dat] file 3-10
	    {
		   string paramFileName = commandLineParser.GetSafeArgument("-I",0,"input.dat");
		   ifstream paramFile;
		   paramFile.open(paramFileName.c_str());
		   if(!paramFile)
		   {
			   cout << "The following parameter file does not exist: " << paramFileName << endl;
			   exit(3);
		   }
		   else
			   cout << "Using the following parameter file: " << paramFileName << endl;
		}
		if(commandLineParser.HasSwitch("-O"))
		{
			string inputFile = commandLineParser.GetSafeArgument("-I",0,"input.dat"); //ACTION-1: 2 names for the parameter file 
			string outputFilePrefix = commandLineParser.GetSafeArgument("-O",0,"output.xml").c_str();
													 														 
			NEAT::Globals::init(inputFile);
			NEAT::Globals::getSingleton()->setOutputFilePrefix(outputFilePrefix); //set the name of the outputPrefixFile
		}
		if(commandLineParser.HasSwitch("-MATS"))
		{
			string materialTypes = commandLineParser.GetSafeArgument("-MATS",0,"MotorSensorSoft").c_str();
			NEAT::Globals::getSingleton()->setMaterialTypes(materialTypes);
		}
	
		if(commandLineParser.HasSwitch("-R"))
		{
			NEAT::Globals::getSingleton()->seedRandom(stringTo<unsigned int>(commandLineParser.GetSafeArgument("-R",0,"0")));
		}

		if(commandLineParser.HasSwitch("-L"))
		{
			NEAT::Globals::getSingleton()->setParameterValue("BoundingBoxLength",stringTo<unsigned int>(commandLineParser.GetSafeArgument("-L",0,"0")));
		}

		if(commandLineParser.HasSwitch("-M"))
		{
			NEAT::Globals::getSingleton()->setParameterValue("NumMaterials",stringTo<unsigned int>(commandLineParser.GetSafeArgument("-M",0,"0")));
		}

		if(commandLineParser.HasSwitch("-E"))
		{
			NEAT::Globals::getSingleton()->setParameterValue("PenaltyExp",stringTo<unsigned int>(commandLineParser.GetSafeArgument("-E",0,"0")));
		}

		if(commandLineParser.HasSwitch("-H"))
		{
			NEAT::Globals::getSingleton()->setParameterValue("MaxStiffness",stringTo<unsigned int>(commandLineParser.GetSafeArgument("-H",0,"0")));
		}

		if(commandLineParser.HasSwitch("-P"))
		{
			NEAT::Globals::getSingleton()->setParameterValue("PenaltyType",stringTo<unsigned int>(commandLineParser.GetSafeArgument("-P",0,"0")));
		}

		if(commandLineParser.HasSwitch("-D"))
		{
			NEAT::Globals::getSingleton()->setParameterValue("HasObstacles",stringTo<unsigned int>(commandLineParser.GetSafeArgument("-D",0,"0")));
		}

		if(commandLineParser.HasSwitch("-G")){
			SET_PARAMETER("MaxGenerations", atoi(commandLineParser.GetSafeArgument("-G",0,"0").c_str()));
		}
		
		if(commandLineParser.HasSwitch("-T")){
			SET_PARAMETER("TerminalFitness",atof(commandLineParser.GetSafeArgument("-T",0,"0").c_str()));
		}
	
		int experimentType = int(GET_PARAMETER("ExperimentType")+0.001);
		
		cout << "Loading Experiment: " << experimentType << endl;
		cout << "With Generations: " << GET_PARAMETER("MaxGenerations") <<endl;
		
		//jmc: remove the file to make the way for the creation of a new one for this run
		ofstream output_file;        

		std::ostringstream tmpName;
		tmpName << "Softbots--" << NEAT::Globals::getSingleton()->getOutputFilePrefix() << "---gen-Genchamp-AvgFit.txt";
	  	string outoutFileName = tmpName.str();

		output_file.open (outoutFileName.c_str(), ios::trunc );
		output_file.close(); 
		
		HCUBE::ExperimentRun experimentRun;
		
		string outputFilePrefix = commandLineParser.GetSafeArgument("-O",0,"output.xml").c_str();
		experimentRun.setupExperiment(experimentType, outputFilePrefix);
		
		cout << "Experiment set up\n";
		
		experimentRun.createPopulation();
		
		experimentRun.setCleanup(true);
		
		cout << "Population Created\n";
		
		experimentRun.start();

	}

	catch (string s)
	{
		cout << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
		cout << "An exception has occured: " << s << endl;
	}
	catch(LocatedException e) {
		cout << "CAUGHT Exception: " << e.what() << endl;
	}
	catch (...)
	{
		cout << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
		cout << "An error has occured!\n";
	}
	
	NEAT::Globals::deinit();
	
	return 0;
	}
//main function

	int main(int argc,char **argv)
	{
		if (argc<5)
		{
			cout << "You must pass the parameters and the output file as command parameters!\n";
			cout << "Syntax for passing command-line options to HyperNEAT:[..] are optional\n";
			cout << "./HyperNEAT -I (parameterfile) -O (output/individual-file) [-ORG org1 org2] \n";
			cout << "-ORG org1 org2 - if -ORG or both org1 and org2 are absent new individual is produced \n";
			cout << "-ORG org1 org2 - if -ORG org1 this individual is mutated \n";
			cout << "-ORG org1 org2 - if -ORG org1 org2 than they are considered as parents anf after Xover and mutation they produce one offspring \n";
		}
		else	
			HyperNEAT_main(argc,argv);
	}
