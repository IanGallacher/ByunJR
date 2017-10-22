#pragma once
#include "micro/ProxyManager.h"

class ProxyManager;

class Candidate {
    std::vector<int> genes_;
    int fitness_;

public:
    Candidate();
    Candidate(std::vector<int> genes);
    Candidate(std::vector<int> genes, int fitness);
    void SetFitness(int fitness);
    void SetGene(int index, int gene);
    int GetGene(int i);
    int GetFitness() const;
};

class Population {
    std::vector<Candidate> canidates_;

public :
    Population(const int size);
    void SetCanidate(int index, Candidate c);
    Candidate GetCandidate(int index);
    Candidate GetFittest();
    void SetReward(int i, int reward);
};

class GeneticAlgorithm {
    double uniform_rate_;
    double mutation_rate_;
    int tournament_size_;
    bool elitism_;
    Population population_;

    Candidate Crossover(Candidate indiv1, Candidate indiv2) const;
    void Mutate(Candidate &indiv) const;

    Candidate TournamentSelection(Population pop) const;

    public:
        GeneticAlgorithm();
        void EvolvePopulation(ProxyTrainingData & pm);
        Population* GetPopulation();
        void SetReward(int i, int reward);
};