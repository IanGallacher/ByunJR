#pragma once
#include <sc2api/sc2_unit.h>

class ByunJRBot;

struct BuildOrderItem
{
    sc2::UnitTypeID type;		// the thing we want to 'build'
    int             priority;	// the priority at which to place it in the queue
    bool            blocking;	// whether or not we block further items

    BuildOrderItem(sc2::UnitTypeID t, int p, bool b);
    bool operator<(const BuildOrderItem & x) const;
};

class BuildOrderQueue
{
    ByunJRBot & m_bot;
    std::deque<BuildOrderItem> m_queue;

    int m_lowestPriority;
    int m_highestPriority;
    int m_defaultPrioritySpacing;
    int m_numSkippedItems;

public:

    BuildOrderQueue(ByunJRBot & bot);

    void clearAll();											// clears the entire build order queue
    void skipItem();											// increments skippedItems
    void queueAsHighestPriority(sc2::UnitTypeID type, bool blocking);		// queues something at the highest priority
    void queueAsLowestPriority(sc2::UnitTypeID type, bool blocking);		// queues something at the lowest priority
    void queueItem(BuildOrderItem b);			// queues something with a given priority
    void removeHighestPriorityItem();								// removes the highest priority item
    void removeCurrentHighestPriorityItem();

    size_t size() const;													// returns the size of the queue

    bool isEmpty() const;

    BuildOrderItem & getHighestPriorityItem();	// returns the highest priority item
    BuildOrderItem & getNextHighestPriorityItem();	// returns the highest priority item

    bool canSkipItem();
    std::string getQueueInformation() const;

    // overload the bracket operator for ease of use
    BuildOrderItem operator [] (int i);
};
