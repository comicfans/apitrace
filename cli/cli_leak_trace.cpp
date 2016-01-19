#include <string.h>
#include <limits.h> // for CHAR_MAX
#include <getopt.h>
#ifndef _WIN32
#include <unistd.h> // for isatty()
#endif

#include <set>
#include <algorithm>
#include <sstream>

#include "cli.hpp"
#include "globj_records.hpp"

#include "trace_parser.hpp"
#include "trace_dump.hpp"
#include "trace_option.hpp"



static bool verbose = false;

static const char *synopsis = "check leak given trace(s) to standard output.";

static void
usage(void)
{
    std::cout
        << "usage: apitrace leak-trace [OPTIONS] TRACE_FILE...\n"
        << synopsis << "\n"
        "\n"
        "    -h, --help           show this help message and exit\n"
        "    -v, --verbose        verbose output\n"
        "    --catalogs=[=WHICH]   only detect specified catalogs[default: all]\n"
        "    --thread-ids=[=WHICH] only detect specified threas[default: all]\n"
        "    --call-nos[=BOOL]    dump call numbers[default: yes]\n"
        "\n"
    ;
}

enum {
    CATALOGS_OPT = CHAR_MAX + 1,
    THREAD_IDS_OPT,
    CALL_NOS_OPT,
};

const static char *
shortOptions = "hv";

const static struct option
longOptions[] = {
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, 0, 'v'},
    {"catalogs", required_argument, 0, CATALOGS_OPT},
    {"thread-ids", optional_argument, 0, THREAD_IDS_OPT},
    {"call-nos", optional_argument, 0, CALL_NOS_OPT},
    {0, 0, 0, 0}
};

static void
parseThreadIds(std::set<int> &traceThreads,const char *args){

    std::istringstream optArg(args);

    std::string id;
    while(std::getline(optArg,id,',')){
        traceThreads.insert(std::stoi(id));
    }
}

static void
parseCatalogs(std::set<std::string> &catalogs,const char * args){

    std::istringstream optArg(args);

    std::string catalog;
    while(std::getline(optArg,catalog,',')){
        catalogs.insert(catalog);
    }
}

static int
command(int argc, char *argv[])
{
    trace::DumpFlags dumpFlags = 0;

    std::set<int> traceThreads;
    std::set<std::string> catalogs;
    
    int opt;
    while ((opt = getopt_long(argc, argv, shortOptions, longOptions, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        case 'v':
            verbose = true;
            break;
        case CATALOGS_OPT:
            parseCatalogs(catalogs,optarg);
            break;
        case THREAD_IDS_OPT:
            parseThreadIds(traceThreads,optarg);
            break;
        case CALL_NOS_OPT:
            if (trace::boolOption(optarg)) {
                dumpFlags &= ~trace::DUMP_FLAG_NO_CALL_NO;
            } else {
                dumpFlags |= trace::DUMP_FLAG_NO_CALL_NO;
            }
            break;
        default:
            std::cerr << "error: unexpected option `" << (char)opt << "`\n";
            usage();
            return 1;
        }
    }




    

    for (int i = optind; i < argc; ++i) {
        trace::Parser p;

        if (!p.open(argv[i])) {
            return 1;
        }

        GLObjRecords records;


        int lastPrintThreadId=-1;
        std::string lastPrintType="";
        records.traceLeaks(p,catalogs,[&](
                    std::string type,unsigned int glName,
                    std::shared_ptr<trace::Call> call){

                if (call->thread_id!=lastPrintThreadId) {
                lastPrintThreadId=call->thread_id;
                std::cout<<"thread:"<<lastPrintThreadId<<" leaks:\n";
                lastPrintType="";
                }

                if (type!=lastPrintType) {

                std::cout<<" type:"<<type<<":\n";
                }

                //std::cout<<"   glname:"<<ordered.first<<"\n";
                trace::dump(*call, std::cout, dumpFlags);
                });
    }

    return 0;
}

const Command leak_trace_command = {
    "leak-trace",
    synopsis,
    usage,
    command
};

