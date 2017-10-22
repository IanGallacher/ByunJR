#include "GeneticAlgorithm.h"
// See http://www.theprojectspot.com/tutorial-post/creating-a-genetic-algorithm-for-beginners/3 for more information on genetic algorithms.
// MIT Opencourseware on genetic algorithms: https://www.youtube.com/watch?v=kHyNqSnzP8Y
// Much of the code here was inspired by the above two links. 
Candidate::Candidate()
{

}

Candidate::Candidate(const std::vector<int> genes)
{
    genes_ = genes;
}

Candidate::Candidate(const std::vector<int> genes, const int fitness)
{
    genes_ = genes;
    fitness_ = fitness;
}

void Candidate::SetFitness(const int fitness)
{
    fitness_ = fitness;
}

void Candidate::SetGene(const int index, const int gene)
{
    genes_[index] = gene;
}

int Candidate::GetGene(const int i)
{
    return genes_[i];
}

int Candidate::GetFitness() const
{
    return fitness_;
}


Population::Population(const int size)
{
    canidates_ = std::vector<Candidate>();
    canidates_.resize(size);
}

void Population::SetCanidate(const int index, Candidate c)
{
    canidates_[index] = c;
}

Candidate Population::GetCandidate(const int index)
{
    return canidates_[index];
}

void Population::SetReward(const int index, const int reward)
{
    canidates_[index].SetFitness(reward);
}

Candidate Population::GetFittest() {
    Candidate fittest = canidates_[0];

    for (int i = 0; i < canidates_.size(); ++i) {
        if (fittest.GetFitness() <= GetCandidate(i).GetFitness()) {
            fittest = GetCandidate(i);
        }
    }
    return fittest;
}



GeneticAlgorithm::GeneticAlgorithm()
    : uniform_rate_(0.5),
    mutation_rate_(0.015),
    tournament_size_(5),
    elitism_(true),
    population_(10)
{

}

void GeneticAlgorithm::EvolvePopulation(ProxyTrainingData & pm) {
    Population new_population(10); // new Population(pop.size(), false);

    int elitism_offset;
    // Keep our best candidate
    if (elitism_) {
        new_population.SetCanidate(0, population_.GetFittest());
        elitism_offset = 1;
    }
    else {
        elitism_offset = 0;
    }

    // Loop over the population size and create new individuals with
    // crossover
    for (int i = elitism_offset; i < 10; i++) {
        const Candidate indiv1 = TournamentSelection(population_);
        const Candidate indiv2 = TournamentSelection(population_);
        Candidate new_indiv = Crossover(indiv1, indiv2);

        // make sure we have the closest valid proxy location.
        const sc2::Point2D point = pm.GetNearestUntestedProxyLocation( new_indiv.GetGene(0), new_indiv.GetGene(1) );
        const auto genes = std::vector<int> { static_cast<int>(point.x), static_cast<int>(point.y) };
        auto final_indiv = Candidate(genes);
        new_population.SetCanidate(i, new_indiv);
    }

    // Mutate population
    for (int i = elitism_offset; i < 10; i++) {
        Mutate(new_population.GetCandidate(i));
    }

    population_ = new_population;
}

// Crossover individuals
Candidate GeneticAlgorithm::Crossover(Candidate indiv1, Candidate indiv2) const
{
    auto genes = std::vector<int>();
    genes.resize(2);
    // Loop through genes
    for (int i = 0; i < 2; i++) { //2 is how many genes there are.
        // Crossover
        if (rand() % 2 == 0) {
            genes[i] = indiv1.GetGene(i);
        }
        else {
            genes[i] = indiv2.GetGene(i);
        }
    }

    auto can = Candidate(genes);
    return can;
}

// Mutate an candidate
void GeneticAlgorithm::Mutate(Candidate &indiv) const
{
    // Loop through genes
    for (int i = 0; i < 2; i++) { //2 is how many genes there are.
        if (rand() <= mutation_rate_) {
            // Create random gene
            const int gene = indiv.GetGene(i) + rand() % 2;
            indiv.SetGene(i, gene);
        }
    }
}

Population* GeneticAlgorithm::GetPopulation()
{
    return &population_;
}

void GeneticAlgorithm::SetReward(const int i, const int reward)
{
    population_.SetReward(i, reward);
}


// Select individuals for crossover
Candidate GeneticAlgorithm::TournamentSelection(Population pop) const
{
    // Create a tournament population
    Population tournament(5);
    // For each place in the tournament get a random individual
    for (int i = 0; i < tournament_size_; i++) {
        const int random_id = rand() % 10; //pop.size());
        tournament.SetCanidate(i, pop.GetCandidate(random_id));
    }

    // Get the fittest
    Candidate fittest = tournament.GetFittest();
    return fittest;
}
