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

                if (generations > 0) {
                    // TODO: replace OR with experiments[currentSubExperiment].getExperimentName() == "HYBRID"
                    if (experiments[currentSubExperiment]->getExperimentName() == "HYBRID" && switchSubExperiment(generations))
                        //					if((experimentType == EXPERIMENT_LEGSWING_HYBRID || experimentType == EXPERIMENT_BITMIRRORING_HYBRID || experimentType == EXPERIMENT_TARGETWEIGHTS_HYBRID) && switchSubExperiment(generations))
                    {
                        cout << "\n\n"
                                << "************************\n"
                                << "Switching SubExperiment:  Hyper -> FT\n"
                                << "************************\n\n";
                        // copy population from (currentSubExperiment-1)%totalSubExperiments to currentSubExperiment using HyperNEAT to P-NEAT converter
                        population = shared_ptr<NEAT::GeneticPopulation>(experiments[currentSubExperiment]->createInitialPopulation(population, experiments[(currentSubExperiment + 1) % totalSubExperiments]));
                        //TODO: check that this copying works

                        //THESE CAN COME OUT when Hyper -> NEAT works
                        cout << "changing the following three parameter settings from the HyperNEAT settings of:" << endl;
                        cout << "MutateAddNodeProbability: " << NEAT::Globals::getSingleton()->getParameterValue("MutateAddNodeProbability") << endl;
                        cout << "MutateAddLinkProbability: " << NEAT::Globals::getSingleton()->getParameterValue("MutateAddLinkProbability") << endl;
                        cout << "MutateDemolishLinkProbability: " << NEAT::Globals::getSingleton()->getParameterValue("MutateDemolishLinkProbability") << endl;
                        NEAT::Globals::getSingleton()->setParameterValue("MutateAddNodeProbability", 0.0);
                        NEAT::Globals::getSingleton()->setParameterValue("MutateAddLinkProbability", 0.0);
                        NEAT::Globals::getSingleton()->setParameterValue("MutateDemolishLinkProbability", 0.0);
                        cout << endl << "to the FT-NEAT values of: " << endl;
                        cout << "MutateAddNodeProbability: " << NEAT::Globals::getSingleton()->getParameterValue("MutateAddNodeProbability") << endl;
                        cout << "MutateAddLinkProbability: " << NEAT::Globals::getSingleton()->getParameterValue("MutateAddLinkProbability") << endl;
                        cout << "MutateDemolishLinkProbability: " << NEAT::Globals::getSingleton()->getParameterValue("MutateDemolishLinkProbability") << endl << endl;

                        if (experiments[currentSubExperiment]->getHybrid_FTMutateOnlyProbability() != -1.0)
                            NEAT::Globals::getSingleton()->setParameterValue("MutateOnlyProbability", experiments[currentSubExperiment]->getHybrid_FTMutateOnlyProbability());

                        if (experiments[currentSubExperiment]->getHybrid_FTMutateLinkProbability() != -1.0)
                            NEAT::Globals::getSingleton()->setParameterValue("MutateLinkProbability", experiments[currentSubExperiment]->getHybrid_FTMutateLinkProbability());

                        //THESE CAN COME OUT ONCE THIS WORKS
                        cout << "MutateOnlyProbability" << NEAT::Globals::getSingleton()->getParameterValue("MutateOnlyProbability") << endl;
                        cout << "MutateLinkProbability" << NEAT::Globals::getSingleton()->getParameterValue("MutateLinkProbability") << endl;
                    } else {
                        mutex::scoped_lock scoped_lock(*populationMutex);
                        cout << "\nPRODUCING NEXT GENERATION\n";
                        produceNextGeneration();
                    }
                }

                if (experiments[currentSubExperiment]->performUserEvaluations()) {
                    throw CREATE_LOCATEDEXCEPTION_INFO("ERROR: TRIED TO USE INTERACTIVE EVOLUTION WITH NO GUI!");
                } else {
                    while (!running) {
                        // boost::xtime xt;
                        // boost::xtime_get(&xt, boost::TIME_UTC_); //(nac: changed for latest version of boost)
                        // xt.sec += 1;
                        // boost::this_thread::sleep(xt); // Sleep for 1/2 second
                        // boost::this_thread::sleep_for(std::chrono::seconds(1));
                        sleep(1);
                    }
#ifdef INTERACTIVELYEVOLVINGSHAPES
                    stringstream genNum;
                    genNum << setw(5) << std::setfill('0') << generations;
                    population->dumpLast(outputFileName + "_" + genNum.str() + ".xml", true, false); //print out xml file each generation
#endif


                    cout << "about to evaluatePopulation\n";
                    evaluatePopulation();
                }

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
        //Randomize population order for evaluation
        //		if(experimentType != EXPERIMENT_LEGSWING_HYBRID && experimentType != EXPERIMENT_BITMIRRORING_HYBRID)
        //generation->randomizeIndividualOrder();  //Jason ransomized these to balance loads on machines because more fit orgs took longer. It should be ok to comment it out (though it will break consistency tsets"

        int populationSize = population->getIndividualCount();

        int populationPerProcess = populationSize / NUM_THREADS;

        //		cout << "jmc: used1!\n"; exit(3);
        //boost::thread** threads = new boost::thread*[NUM_THREADS]; // comment this out to get rid of threading
        EvaluationSet** evaluationSets = new EvaluationSet*[NUM_THREADS];

        if (totalSubExperiments > 1) {
            assert(NUM_THREADS == 1); // only one thread allowed
            //Fix for uneven distribution
            int populationIteratorSize =
                    populationSize
                    - populationPerProcess * (NUM_THREADS - 1);

            evaluationSets[0] =
                    new EvaluationSet(
                    experiments[currentSubExperiment],
                    generation,
                    population->getIndividualIterator(populationPerProcess * 0),
                    populationIteratorSize
                    );
            //threads[0] = new boost::thread(boost::bind(&EvaluationSet::run,evaluationSets[0]));
            //jmc: commenting out previous line, and created next, to get rid of threading so that opengl does not pinwheel
            evaluationSets[0]->run();

        } else {
            for (int i = 0; i < NUM_THREADS; ++i) {
                if (i + 1 == NUM_THREADS) {
                    //Fix for uneven distribution
                    int populationIteratorSize =
                            populationSize
                            - populationPerProcess * (NUM_THREADS - 1);
                    evaluationSets[i] =
                            new EvaluationSet(
                            experiments[i],
                            generation,
                            population->getIndividualIterator(populationPerProcess * i),
                            populationIteratorSize
                            );
                } else {

                    evaluationSets[i] =
                            new EvaluationSet(
                            experiments[i],
                            generation,
                            population->getIndividualIterator(populationPerProcess * i),
                            populationPerProcess
                            );
                }

                //threads[i] = new boost::thread(boost::bind(&EvaluationSet::run,	evaluationSets[i]));
                //jmc: commenting out previous line, and created next, to get rid of threading so that opengl does not pinwheel
                evaluationSets[i]->run();
            }
        }
        //loop through each thread, making sure it is finished before we move on
        for (int i = 0; i < NUM_THREADS; ++i) {
            /*if (!evaluationSets[i]->isFinished())
             {
             --i;
             boost::xtime xt;
             boost::xtime_get(&xt, boost::TIME_UTC);
             xt.sec += 1;
             boost::thread::sleep(xt); // Sleep for 1/2 second
             }*/

            //threads[i]->join(); //jmc hail mary - this is where it is crashing, now that I have removed threads
        }
        //TODO: Does not deleting these cause a memory leak?		
        // hail mary continues: following five lines are uncommented with threading
        //		for (int i = 0; i < NUM_THREADS; ++i)
        //		{
        //			delete threads[i];
        //			delete evaluationSets[i];
        //		}

        //delete[] threads; //comment this out to get rid of threading
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
