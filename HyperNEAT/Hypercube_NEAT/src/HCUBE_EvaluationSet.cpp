#include "HCUBE_EvaluationSet.h"



namespace HCUBE {

    void EvaluationSet::run() {
#ifndef _DEBUG
        try
#endif
        {
            //Process individuals sequentially
            running = true;

            vector<shared_ptr<NEAT::GeneticIndividual> >::iterator tmpIterator;

            tmpIterator = individualIterator;
            for (int a = 0; a < individualCount; a++, tmpIterator++) {

                experiment->addIndividualToGroup(*tmpIterator);

                if (experiment->getGroupSize() == experiment->getGroupCapacity()) {
                    //cout << "Processing group...\n";
                    experiment->processGroup(generation);
                    //cout << "Done Processing group\n";
                    experiment->clearGroup();
                }
            }

            finished = true;
        }
#ifndef _DEBUG
        catch (string s) {
            cout << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE(s);
        }        catch (const char *s) {
            cout << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE(s);
        }        catch (const std::exception &ex) {
            cout << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE(ex.what());
        }        catch (...) {
            cout << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE("An unknown exception has occured!");
        }
#endif
    }

}
