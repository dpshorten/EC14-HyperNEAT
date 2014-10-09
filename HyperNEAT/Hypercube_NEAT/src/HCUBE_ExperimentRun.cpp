#include "HCUBE_ExperimentRun.h"

#include "Experiments/HCUBE_Experiment.h"

#include "Experiments/HCUBE_SoftbotsExperiment.h"

#ifdef COMPILE_ODE
#include "Experiments/HCUBE_LegSwingExperiment.h"
#include "Experiments/HCUBE_LegSwingExperiment-Pneat.h"
#include "Experiments/HCUBE_LegSwingExperiment-NEAT.h"

#include "Experiments/HCUBE_SpiderRobotExperiment.h"
#endif //COMPILE_ODE

#include "HCUBE_EvaluationSet.h"

#define DEBUG_EXPRUN

namespace HCUBE {

    ExperimentRun::ExperimentRun()
    :
    running(false),
    started(false),
    cleanup(false),
    currentSubExperiment(0),
    totalSubExperiments(1),
    populationMutex(new mutex()),
    frame(NULL) {
    }

    ExperimentRun::~ExperimentRun() {
        delete populationMutex;
    }

    void ExperimentRun::setupExperiment(int _experimentType, string _outputFileName) {
        experimentType = _experimentType;
        outputFileName = _outputFileName;

        cout << "SETTING UP EXPERIMENT TYPE: " << experimentType << endl;

        switch (experimentType) {
            case EXPERIMENT_SOFTBOTS:
                experiments.push_back(shared_ptr<Experiment>(new SoftbotsExperiment("")));
                break;

            default:
                cout << string("ERROR: Unknown Experiment Type!\n");
                throw CREATE_LOCATEDEXCEPTION_INFO("ERROR: Unknown Experiment Type!");
        }
        for (int a = 1; a < NUM_THREADS; a++) {
            if (totalSubExperiments > 1) {
                cerr << "ERROR: cannot have a hybrid experiment with multiple treads!\n";
                exit(1);
            }
            experiments.push_back(shared_ptr<Experiment>(experiments[0]->clone()));
        }

    }

    void ExperimentRun::createPopulation(string populationString) {
        cout << "createPopulation: " << populationString << endl;

        if (iequals(populationString, "")) {
            int popSize = (int) NEAT::Globals::getSingleton()->getParameterValue("PopulationSize");
            population = shared_ptr<NEAT::GeneticPopulation>(experiments[currentSubExperiment]->createInitialPopulation(popSize));
        } else {
            population = shared_ptr<NEAT::GeneticPopulation>(new NEAT::GeneticPopulation(populationString));
        }
    }

    void ExperimentRun::setupExperimentInProgress(string populationFileName, string _outputFileName) {
        outputFileName = _outputFileName;

        {
            TiXmlDocument doc(populationFileName);

            bool loadStatus;

            if (iends_with(populationFileName, ".gz")) {
                loadStatus = doc.LoadFileGZ();
            } else {
                loadStatus = doc.LoadFile();
            }

            if (!loadStatus) {
                cerr << "Error trying to load the XML file!" << endl;
                throw CREATE_LOCATEDEXCEPTION_INFO("Error trying to load the XML file!");
            }

            TiXmlElement *element = doc.FirstChildElement();

            NEAT::Globals* globals = NEAT::Globals::init(element);
            (void) globals; //to get rid of unused variable warning
            NEAT::Globals::getSingleton()->setOutputFilePrefix(outputFileName); //set the name of the outputPrefixFile                                                                                                                                                                                               


            //Destroy the document
        }

        int experimentType = int(NEAT::Globals::getSingleton()->getParameterValue("ExperimentType") + 0.001);

        cout << "Loading Experiment: " << experimentType << endl;

        setupExperiment(experimentType, _outputFileName);

        cout << "Experiment set up.  Creating population...\n";

        createPopulation(populationFileName);

        cout << "Population Created\n";
    }

    void ExperimentRun::setupExperimentInProgressUseDat(string _datFile, string populationFileName, string _outputFileName) {
        outputFileName = _outputFileName;
        //datFile = _datFile;
        PRINT(populationFileName);
        {
            TiXmlDocument doc(populationFileName);

            bool loadStatus;

            if (iends_with(populationFileName, ".gz")) {
                loadStatus = doc.LoadFileGZ();
            } else {
                loadStatus = doc.LoadFile();
            }

            if (!loadStatus) {
                cerr << "Error trying to load the XML file!" << endl;
                throw CREATE_LOCATEDEXCEPTION_INFO("Error trying to load the XML file!");
            }

            TiXmlElement *element = doc.FirstChildElement();

            NEAT::Globals* globals = NEAT::Globals::init(element);
            (void) globals; //to get rid of unused variable warning

            NEAT::Globals::getSingleton()->overrideParametersFromFile(_datFile);
            NEAT::Globals::getSingleton()->setOutputFilePrefix(outputFileName); //set the name of the outputPrefixFile                                                                                                                                                                                               

            //Destroy the document
        }

        int experimentType = int(NEAT::Globals::getSingleton()->getParameterValue("ExperimentType") + 0.001);

        cout << "Loading Experiment: " << experimentType << endl;

        setupExperiment(experimentType, _outputFileName);

        cout << "Experiment set up.  Creating population...\n";

        createPopulation(populationFileName);

        cout << "Population Created\n";
    }

    void ExperimentRun::start() {
        cout << "Experiment started\n";


#ifndef DEBUG_EXPRUN
        try {
#endif
            int maxGenerations = int(NEAT::Globals::getSingleton()->getParameterValue("MaxGenerations"));

            started = running = true;

            for (int generations = (population->getGenerationCount() - 1); generations < GET_PARAMETER("MaxGenerations"); generations++) {
                cout << "CURRENT SUBEXPERIMENT: " << currentSubExperiment << " Generation:" << generations << endl;
                cout << "about to evaluatePopulation\n";
                evaluatePopulation();


#ifdef DEBUG_EXPRUN
                cout << "Finishing evaluations\n";
#endif
                finishEvaluations();
                experimentRunPrintToGenChampFile();

#ifdef DEBUG_EXPRUN
                cout << "Evaluations Finished\n";
#endif
            }
            cout << "Experiment finished\n";

            cout << "Saving Dump...";
            population->dump(outputFileName + string("_pop.xml"), true, false);
            cout << "Done!\n";


            cout << "Saving best individuals...";
            string bestFileName = outputFileName.substr(0, outputFileName.length() - 4) + string("_best.xml");
            population->dumpBest(bestFileName, true, true);
            cout << "Done!\n";

            cout << "Skippped deleting backup files because of problem with boost!";
            //cout << "Deleting backup file...";
            //boost::filesystem::remove(outputFileName+string(".backup.xml.gz"));
            //cout << "Done!\n";

#ifndef DEBUG_EXPRUN
        } catch (const std::exception &ex) {
            cout << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE(ex.what());
        } catch (...) {
            cout << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE("AN UNKNOWN EXCEPTION HAS BEEN THROWN!");
        }
#endif
    }

    void ExperimentRun::evaluatePopulation() {

        shared_ptr<NEAT::GeneticGeneration> generation = population->getGeneration();

        int populationSize = population->getIndividualCount();
        int populationPerProcess = populationSize / NUM_THREADS;

        EvaluationSet** evaluationSets = new EvaluationSet*[NUM_THREADS];

        for (int i = 0; i < NUM_THREADS; ++i) {
            evaluationSets[i] =
                    new EvaluationSet(
                    experiments[i],
                    generation,
                    population->getIndividualIterator(populationPerProcess * i),
                    populationPerProcess
                    );
            evaluationSets[i]->run();
        }
        delete[] evaluationSets;
    }

    void ExperimentRun::finishEvaluations() {
#ifdef DEBUG_EXPRUN
        cout << "Cleaning up...\n";
#endif
        //int generationDumspModulo = int(NEAT::Globals::getSingleton()->getParameterValue("GenerationDumpModulo"));
        if (cleanup)
            population->cleanupOld(INT_MAX / 2);
#ifdef DEBUG_EXPRUN
        cout << "Adjusting fitness...\n";
#endif
        population->adjustFitness();
#ifdef DEBUG_EXPRUN
        cout << "Dumping best individuals...\n";
#endif

        //jmc: note this purges the generations container to keep memory issues to minimum, it also prints a back up file each gen of last gen's champ        
        //population->dumpBest(outputFileName+string(".backup.xml"),true,true);//jmc
        population->dumpLast(outputFileName + string("_previousGenPop.xml"), true, false); //print of most recently evaluated pop (note that when reloaded, only the champ is unmutated/crossed)
        //population->cleanupOld(25);
        //population->dumpBest("out/dumpBestWithGenes(backup).xml",true);


#ifdef DEBUG_EXPRUN
        cout << "Resetting generation data...\n";
#endif
        shared_ptr<NEAT::GeneticGeneration> generation = population->getGeneration();
        experiments[currentSubExperiment]->resetGenerationData(generation);

        for (int a = 0; a < population->getIndividualCount(); a++) {
            //cout << __FILE__ << ":" << __LINE__ << endl;
            experiments[currentSubExperiment]->addGenerationData(generation, population->getIndividual(a));
        }

    }

    void ExperimentRun::produceNextGeneration() {
#ifdef DEBUG_EXPRUN
        cout << "Producing next generation.\n";
#endif
        try {
            population->produceNextGeneration();
        } catch (const std::exception &ex) {
            cout << "EXCEPTION DURING POPULATION REPRODUCTION: " << endl;
            CREATE_PAUSE(ex.what());
        } catch (...) {
            cout << "Unhandled Exception\n";
        }
    }

    void ExperimentRun::experimentRunPrintToGenChampFile() {
        try {
            population->printToGenChampFile();
        } catch (const std::exception &ex) {
            cout << "EXCEPTION DURING experimentRunPrintToGenChampFile: " << endl;
            CREATE_PAUSE(ex.what());
        } catch (...) {
            cout << "Unhandled Exception\n";
        }
    }

    bool ExperimentRun::switchSubExperiment(int generation) {
        if (experiments[currentSubExperiment]->converged(generation)) {
            currentSubExperiment = (currentSubExperiment + 1) % totalSubExperiments;
            return true;
        }
        return false;
    }

}
