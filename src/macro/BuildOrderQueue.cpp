#include <sstream>

#include "ByunJRBot.h"
#include "macro/BuildOrderQueue.h"

BuildOrderQueue::BuildOrderQueue(ByunJRBot & bot)
    : bot_(bot)
    , highest_priority_(0)
    , lowest_priority_(0)
    , default_priority_spacing_(10)
    , num_skipped_items_(0)
{

}

void BuildOrderQueue::ClearAll()
{
    // clear the queue
    queue_.clear();

    // reset the priorities
    highest_priority_ = 0;
    lowest_priority_ = 0;
}

BuildOrderItem & BuildOrderQueue::GetHighestPriorityItem()
{
    // reset the number of skipped items to zero
    num_skipped_items_ = 0;

    // the queue will be sorted with the highest priority at the back
    return queue_.back();
}

BuildOrderItem & BuildOrderQueue::GetNextHighestPriorityItem()
{
    assert(queue_.size() - 1 - num_skipped_items_ >= 0);

    // the queue will be sorted with the highest priority at the back
    return queue_[queue_.size() - 1 - num_skipped_items_];
}

void BuildOrderQueue::SkipItem()
{
    // make sure we can skip
    assert(CanSkipItem());

    // skip it
    num_skipped_items_++;
}

bool BuildOrderQueue::CanSkipItem()
{
    // does the queue have more elements
    const bool big_enough = queue_.size() > static_cast<size_t>(1 + num_skipped_items_);

    if (!big_enough)
    {
        return false;
    }

    // is the current highest priority item not blocking a skip
    const bool highest_not_blocking = !queue_[queue_.size() - 1 - num_skipped_items_].blocking;

    // this tells us if we can skip
    return highest_not_blocking;
}

void BuildOrderQueue::QueueItem(const BuildOrderItem b)
{
    // if the queue is empty, set the highest and lowest priorities
    if (queue_.empty())
    {
        highest_priority_ = b.priority;
        lowest_priority_ = b.priority;
    }

    // push the item into the queue
    if (b.priority <= lowest_priority_)
    {
        queue_.push_front(b);
    }
    else
    {
        queue_.push_back(b);
    }

    // if the item is somewhere in the middle, we have to sort again
    if ((queue_.size() > 1) && (b.priority < highest_priority_) && (b.priority > lowest_priority_))
    {
        // sort the list in ascending order, putting highest priority at the top
        std::sort(queue_.begin(), queue_.end());
    }

    // update the highest or lowest if it is beaten
    highest_priority_ = (b.priority > highest_priority_) ? b.priority : highest_priority_;
    lowest_priority_  = (b.priority < lowest_priority_)  ? b.priority : lowest_priority_;
}

void BuildOrderQueue::QueueAsHighestPriority(const sc2::UnitTypeID m, const bool blocking)
{
    // the new priority will be higher
    const int new_priority = highest_priority_ + default_priority_spacing_;

    // queue the item
    QueueItem(BuildOrderItem(m, new_priority, blocking));
}

void BuildOrderQueue::QueueAsLowestPriority(const sc2::UnitTypeID m, const bool blocking)
{
    // the new priority will be higher
    const int new_priority = lowest_priority_ - default_priority_spacing_;

    // queue the item
    QueueItem(BuildOrderItem(m, new_priority, blocking));
}

void BuildOrderQueue::RemoveHighestPriorityItem()
{
    // remove the back element of the vector
    queue_.pop_back();

    // if the list is not empty, set the highest accordingly
    highest_priority_ = queue_.empty() ? 0 : queue_.back().priority;
    lowest_priority_  = queue_.empty() ? 0 : lowest_priority_;
}

void BuildOrderQueue::RemoveCurrentHighestPriorityItem()
{
    // remove the back element of the vector
    queue_.erase(queue_.begin() + queue_.size() - 1 - num_skipped_items_);

    //assert((int)(queue.size()) < size);

    // if the list is not empty, set the highest accordingly
    highest_priority_ = queue_.empty() ? 0 : queue_.back().priority;
    lowest_priority_  = queue_.empty() ? 0 : lowest_priority_;
}

size_t BuildOrderQueue::Size() const
{
    return queue_.size();
}

bool BuildOrderQueue::IsEmpty() const
{
    return (queue_.size() == 0);
}

BuildOrderItem BuildOrderQueue::operator [] (int i)
{
    return queue_[i];
}

std::string BuildOrderQueue::GetQueueInformation() const
{
    const size_t reps = queue_.size() < 30 ? queue_.size() : 30;
    std::stringstream ss;

    // for each unit in the queue
    for (size_t i(0); i<reps; i++)
    {
        const sc2::UnitTypeID & type = queue_[queue_.size() - 1 - i].type;
        ss << sc2::UnitTypeToName(type) << std::endl;
    }

    return ss.str();
}


BuildOrderItem::BuildOrderItem(sc2::UnitTypeID t, int p, bool b)
    : type(t)
    , priority(p)
    , blocking(b)
{
}

bool BuildOrderItem::operator < (const BuildOrderItem & x) const
{
    return priority < x.priority;
}