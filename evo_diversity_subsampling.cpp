//
//  evo_diversity_subsampling.cpp
//  process_vcf
//
//  Created by Milan Malinsky on 13/03/2017.
//  Copyright © 2017 Milan Malinsky. All rights reserved.
//

#include "evo_diversity_subsampling.h"
#include <unordered_map>
#include <random>

#define SUBPROGRAM "RegionsSubsampledDxy"

#define DEBUG 1

static const char *SUBSAMPLED_DXY_USAGE_MESSAGE =
"Usage: " PROGRAM_BIN " " SUBPROGRAM " [OPTIONS] regions.bed variants.vcf\n"
"Calculate Dxy (and possibly other statistics) for regions defined in the .bed file\n"
"Subsample the regions from the .bed file into random sets of coordinates of a given length\n"
"\n"
"       -h, --help                              display this help and exit\n"
"       -s,   --subsample=LENGTH                (default = 100) length in bp of randomly selected bases to subsample\n"
"       --outFolder=FOLDER                      put the output in FOLDER (default .)\n"
"\n\n"
"\nReport bugs to " PACKAGE_BUGREPORT "\n\n";

static const char* shortopts = "hs:";

enum { OPT_FOLDER };

static const struct option longopts[] = {
    { "subsample",   required_argument, NULL, 's' },
    { "help",   no_argument, NULL, 'h' },
    { "outFolder", required_argument, NULL, OPT_FOLDER },
    { NULL, 0, NULL, 0 }
};

namespace opt
{
    static int subsampleLength = 100;
    static string bedFile = "";
    static string vcfFile = "";
    static string outFolder = "";
}

int subsamplingDxy(int argc, char** argv) {
    parseSubsamplingDxyOptions(argc, argv);
    
    // Load up the intervals file
    std::ifstream* bedFile = new std::ifstream(opt::bedFile.c_str());
    std::map<int, string> linearToGenomeMap;
    std::cerr << "Loading coordinates from the bed file " << opt::bedFile << std::endl;
    SimpleCoordsBed simpleCoords(bedFile, linearToGenomeMap);
    std::cerr << "Done" << std::endl;
    
    // Load up the VCF file:
    std::istream* vcfFile = createReader(opt::vcfFile.c_str());
    string line; std::unordered_map<string, Counts> vcfCountsMap;
    std::unordered_map<string, double> vcfDxyMap;
    std::cerr << "Loading variants from the VCF file " << opt::vcfFile << std::endl;
    double totalDxySum = 0;
    while (getline(*vcfFile, line)) {
        if (line[0] == '#' && line[1] == '#') {
        
        } else if (line[0] == '#' && line[1] == 'C') {
        
        } else {
            // totalVariantNumber++;
            //std::cerr << "Variant N:" << totalVariantNumber << std::endl;
            std::vector<std::string> fields = split(line, '\t');
            string scaffold = fields[0]; string pos = fields[1];
            Counts counts = getThisVariantCountsSimple(fields);
            vcfCountsMap[scaffold+"\t"+pos] = counts;
            double thisVariantDxy = calculateOverallDxy(counts);
            totalDxySum = totalDxySum + thisVariantDxy;
            vcfDxyMap[scaffold+"\t"+pos] = thisVariantDxy;
        }
    }
    std::cerr << "Done" << std::endl;
    int totalIntervalsLength = (int)linearToGenomeMap.size();
    std::cerr << "Average Dxy = " << totalDxySum/totalIntervalsLength << std::endl;
    
    // Prepare the output file for subsamples of Dxy values
    size_t suffixPos = opt::bedFile.find_last_of('.');
    string outDxyFileName;
    if (suffixPos != std::string::npos) {
        outDxyFileName = opt::outFolder + stripExtension(opt::bedFile) + "_DxyVals_l" + numToString(opt::subsampleLength) + ".txt";
    } else {
        outDxyFileName = opt::outFolder + opt::bedFile + "_DxyVals_l" + numToString(opt::subsampleLength) + ".txt";
    }
    //std::cerr << "into: " << AAFileName << std::endl;
    std::ofstream* outDxyFile = new std::ofstream(outDxyFileName.c_str());
    
    
    // Subsampling:
    int numSubsamplesToDo = totalIntervalsLength/opt::subsampleLength;
    std::mt19937_64 rng; // random number generator
    rng.seed();
    std::uniform_int_distribution<int> distribution(0,totalIntervalsLength);
    std::cerr << "Now running subsamples to be written into: " << outDxyFileName << std::endl;
    for (int i = 0; i < numSubsamplesToDo; i++) {
        if (i % 10000 == 0) {
            std::cerr << "Subsample " << i << " out of " << numSubsamplesToDo << std::endl;
        }
        double subsampleDxyTotal = 0;
        for (int j = 0; j < opt::subsampleLength; j++) {
            int linearIndex = distribution(rng);
            string genomeLocation = linearToGenomeMap[linearIndex];
            if (vcfDxyMap.count(genomeLocation) == 1) {
                subsampleDxyTotal = subsampleDxyTotal + vcfDxyMap[genomeLocation];
            }
        }
        *outDxyFile << subsampleDxyTotal/opt::subsampleLength << std::endl;
    }
    std::cerr << "Done" << std::endl;
    return 0;
}


void parseSubsamplingDxyOptions(int argc, char** argv) {
    bool die = false;
    for (char c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;)
    {
        std::istringstream arg(optarg != NULL ? optarg : "");
        switch (c)
        {
            case '?': die = true; break;
            case 's': arg >> opt::subsampleLength; break;
            case OPT_FOLDER: arg >> opt::outFolder; break;
            case 'h':
                std::cout << SUBSAMPLED_DXY_USAGE_MESSAGE;
                exit(EXIT_SUCCESS);
        }
    }
    
    if (argc - optind < 2) {
        std::cerr << "missing arguments\n";
        die = true;
    }
    else if (argc - optind > 2)
    {
        std::cerr << "too many arguments\n";
        die = true;
    }
    
    if (die) {
        std::cout << "\n" << SUBSAMPLED_DXY_USAGE_MESSAGE;
        exit(EXIT_FAILURE);
    }
    opt::bedFile = argv[optind++];
    opt::vcfFile = argv[optind++];
}