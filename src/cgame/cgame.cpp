#include <iostream>
#include <fstream>

#include <celutil/directory.h>
#include <celutil/filetype.h>
#include "cgame.h"


namespace Core
{
    /* Remove body and his satellites.
     
       NOTE: It can be cause to segfault if you use TimeLine in .ssc
       because Timeline::markChanged() want to markChanged() of all his phases->getFrameTree()
       which call body::markChanged() of body which is already removed.
       For example cassini.ssc:Huygens, where was 2 phases:
       first frame refered to cassini (which is parent and cann't cause segfault),
       second frame refered to Sol/Saturn, which can cause segfault if Sol/Saturn already removed 
       before. 
       Full timeLine support not main goal for now.
       TODO: make ~timeline ~FrameTree safely
    */
    void removeBody(Body *body)
    {
        PlanetarySystem* planetary;
        planetary = body->getSatellites();
        if (planetary)
            while(planetary->getSystemSize() > 0)
            {
                removeBody(planetary->getBody(0));
            }
        //set reference to parent if body is simulator ref point
        if (celAppCore->getSimulation()->getFrame()->getRefObject() == body)
        {
            planetary = body->getSystem();
            if (planetary)
                if (planetary->getPrimaryBody())
                {
                    celAppCore->getSimulation()->setSelection(planetary->getPrimaryBody());
                }
                else // no primary? star will be primary!
                    if (planetary->getStar())
                    {
                        celAppCore->getSimulation()->setSelection(planetary->getStar());
                    }
            celAppCore->getSimulation()->follow();
        }
        delete body;
    }

    /* Remove all satellites from planetary 
     
       Note: It can be sagfault if you use timeline and already
       remove some body refered in timeline's phase. See notes in removeBody       
    */
    void clearPlanetarySystem(PlanetarySystem *planetary)
    {
        if (planetary)
            while(planetary->getSystemSize() > 0)
            {
                // remove reverse to prevent timeline remove bug
                removeBody(planetary->getBody(planetary->getSystemSize()-1));
            }
    }

    // .ssc loader
    class SolSystemLoader : public EnumFilesHandler
    {
    public:
        Universe* universe;
        ProgressNotifier* notifier;
        SolSystemLoader(Universe* u, ProgressNotifier* pn) : universe(u), notifier(pn) {};

        bool process(const string& filename)
            {
                if (DetermineFileType(filename) == Content_CelestiaCatalog)
                {
                    string fullname = getPath() + '/' + filename;
                    cout << _("Loading solar system catalog: ") << fullname << '\n';
                    if (notifier)
                        notifier->update(filename);

                    ifstream solarSysFile(fullname.c_str(), ios::in);
                    if (solarSysFile.good())
                    {
                        LoadSolarSystemObjects(solarSysFile,
                                               *universe,
                                               getPath());
                    }
                }
                return true;
            };
    };

    void removeAllSolSys()
    {
        SolarSystemCatalog* solarSystems = celAppCore->getSimulation()->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter;
        for (iter = solarSystems->begin();
             iter != solarSystems->end(); iter++)
        {
            Core::clearPlanetarySystem(iter->second->getPlanets());
            delete iter->second->getFrameTree();
            delete iter->second->getPlanets();
            delete iter->second;
        }
        solarSystems->clear();
    }

    void reloadAllSolSys()
    {
        removeAllSolSys();
        /***** Reload the solar system catalogs *****/
        // First read the solar system files listed individually in the
        // config file.
        cout << "Reload .ssc\n";
        CelestiaConfig *config = celAppCore->getConfig();
        {
            for (vector<string>::const_iterator iter = config->solarSystemFiles.begin();
                 iter != config->solarSystemFiles.end();
                 iter++)
            {
                cout << _("Loading solar system catalog: ") << iter->c_str() << '\n';   
                ifstream solarSysFile(iter->c_str(), ios::in);
                if (!solarSysFile.good())
                {
                    cout << _("Error opening solar system catalog.\n");
                }
                else
                {
                    LoadSolarSystemObjects(solarSysFile, 
                                           *celAppCore->getSimulation()->getUniverse(), "");
                }
            }
        }

        // Next, read all the solar system files in the extras directories
        {
            for (vector<string>::const_iterator iter = config->extrasDirs.begin();
                 iter != config->extrasDirs.end(); iter++)
            {
                if (*iter != "")
                {
                    Directory* dir = OpenDirectory(*iter);

                    SolSystemLoader loader(celAppCore->getSimulation()->getUniverse(), NULL);
                    loader.pushDir(*iter);
                    dir->enumFiles(loader, true);

                    delete dir;
                }
            }
        }
        cout << "Done.\n";
    }
}
