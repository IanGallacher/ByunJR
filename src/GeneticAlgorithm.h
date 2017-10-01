#pragma once
#include "common.h"
#include "ProxyManager.h"

class ProxyManager;

class Candidate {
    std::vector<int> m_genes;
    int m_fitness;

public:
    Candidate();
    Candidate(std::vector<int> genes);
    Candidate(std::vector<int> genes, int fitness);
    void SetFitness(int fitness);
    void setGene(int index, int gene);
    int getGene(int i);
    int getFitness();
};

class Population {
    std::vector<Candidate> m_canidates;

public :
    Population(const int size);
    void setCanidate(int index, Candidate c);
    Candidate getCandidate(int index);
    Candidate getFittest();
    void setReward(int i, int reward);
};

class GeneticAlgorithm {
    double uniformRate;
    double mutationRate;
    int tournamentSize;
    bool elitism;
    Population m_population;

    Candidate crossover(Candidate indiv1, Candidate indiv2);
    void mutate(Candidate &indiv);

    Candidate tournamentSelection(Population pop);

    public:
        GeneticAlgorithm();
        Population evolvePopulation(ProxyTrainingData & pm);
        Population* getPopulation();
        void setReward(int i, int reward);
};