#include "GeneticAlgorithm.h"

Candidate::Candidate()
{

}

Candidate::Candidate(std::vector<int> genes)
{
    m_genes = genes;
}

Candidate::Candidate(std::vector<int> genes, int fitness)
{
    m_genes = genes;
    m_fitness = fitness;
}

void Candidate::SetFitness(int fitness)
{
    m_fitness = fitness;
}

void Candidate::setGene(int index, int gene)
{
    m_genes[index] = gene;
}

int Candidate::getGene(int i)
{
    return m_genes[i];
}

int Candidate::getFitness()
{
    return m_fitness;
}



void Population::setCanidate(int index, Candidate c)
{
    m_canidates[index] = c;
}

Candidate Population::getCandidate(int index)
{
    return m_canidates[index];
}

Candidate Population::getFittest() {
    Candidate fittest = m_canidates[0];

    for (int i = 0; i < 10; ++i) {
        if (fittest.getFitness() <= getCandidate(i).getFitness()) {
            fittest = getCandidate(i);
        }
    }
    return fittest;
}



GeneticAlgorithm::GeneticAlgorithm()
    : uniformRate(0.5),
    mutationRate(0.015),
    tournamentSize(5),
    elitism(true),
    m_population()
{

}

Population GeneticAlgorithm::evolvePopulation() {
    Population newPopulation = Population(); // new Population(pop.size(), false);

    int elitismOffset;
    // Keep our best candidate
    if (elitism) {
        newPopulation.setCanidate(0, m_population.getFittest());
        elitismOffset = 1;
    }
    else {
        elitismOffset = 0;
    }

    // Loop over the population size and create new individuals with
    // crossover
    for (int i = elitismOffset; i < 10; i++) {
        Candidate indiv1 = tournamentSelection(m_population);
        Candidate indiv2 = tournamentSelection(m_population);
        Candidate newIndiv = crossover(indiv1, indiv2);
        newPopulation.setCanidate(i, newIndiv);
    }

    // Mutate population
    for (int i = elitismOffset; i < 10; i++) {
        mutate(newPopulation.getCandidate(i));
    }

    return newPopulation;
}

// Crossover individuals
Candidate GeneticAlgorithm::crossover(Candidate indiv1, Candidate indiv2) {
    Candidate newSol = Candidate();
    // Loop through genes
    for (int i = 0; i < 2; i++) { //2 is how many genes there are.
        // Crossover
        if (rand() % 2 == 0) {
            newSol.setGene(i, indiv1.getGene(i));
        }
        else {
            newSol.setGene(i, indiv2.getGene(i));
        }
    }
    return newSol;
}

// Mutate an candidate
void GeneticAlgorithm::mutate(Candidate &indiv) {
    // Loop through genes
    for (int i = 0; i < 2; i++) { //2 is how many genes there are.
        if (rand() <= mutationRate) {
            // Create random gene
            int gene = indiv.getGene(i) + rand() % 2;
            indiv.setGene(i, gene);
        }
    }
}

Population* GeneticAlgorithm::getPopulation()
{
    return &m_population;
}


// Select individuals for crossover
Candidate GeneticAlgorithm::tournamentSelection(Population pop) 
{
    // Create a tournament population
    Population tournament;
    // For each place in the tournament get a random individual
    for (int i = 0; i < tournamentSize; i++) {
        int randomId = rand() % 10; //pop.size());
        tournament.setCanidate(i, pop.getCandidate(randomId));
    }

    // Get the fittest
    Candidate fittest = tournament.getFittest();
    return fittest;
}