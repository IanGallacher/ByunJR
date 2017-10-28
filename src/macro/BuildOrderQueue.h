#pragma once
#include <sc2api/sc2_unit.h>

class ByunJRBot;

struct BuildOrderItem
{
    sc2::UnitTypeID type;        // the thing we want to 'build'
    int             priority;    // the priority at which to place it in the queue
    bool            blocking;    // whether or not we block further items

    BuildOrderItem(sc2::UnitTypeID t, int p, bool b);
    bool operator<(const BuildOrderItem & x) const;
};

class BuildOrderQueue
{
    ByunJRBot & bot_;
    std::deque<BuildOrderItem> queue_;

    int lowest_priority_;
    int highest_priority_;
    int default_priority_spacing_;
    int num_skipped_items_;

public:

    BuildOrderQueue(ByunJRBot & bot);

    void ClearAll();                                                         // clears the entire build order queue
    void SkipItem();                                                         // increments skippedItems
    void QueueAsHighestPriority(const sc2::UnitTypeID unit_type, const bool blocking);        // queues something at the highest priority
    void QueueAsLowestPriority(const sc2::UnitTypeID unit_type, const bool blocking);         // queues something at the lowest priority
    void QueueItem(BuildOrderItem b);                                        // queues something with a given priority
    void QueueItem(const sc2::UnitTypeID unit_type, const int priority);
    void RemoveHighestPriorityItem();                                        // removes the highest priority item
    void RemoveCurrentHighestPriorityItem();

    size_t Size() const;                                                     // returns the size of the queue

    bool IsEmpty() const;

    BuildOrderItem & GetHighestPriorityItem();                               // returns the highest priority item
    BuildOrderItem & GetNextHighestPriorityItem();                           // returns the highest priority item

    bool CanSkipItem();
    std::string GetQueueInformation() const;

    // overload the bracket operator for ease of use
    BuildOrderItem operator [] (int i);
};
