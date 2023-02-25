#ifndef MY_UNORDERED_MAP
#define MY_UNORDERED_MAP

#include <iostream>
#include <list>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <functional>

template<class Key, class Value>
class my_unordered_map {
public:
    class iterator;
    class const_iterator;
private:
    using My_List = std::list<std::pair<const Key, Value>>;
    using My_List_iter = typename My_List::iterator;
    using My_List_constiter = typename My_List::const_iterator;
    using My_Bucket = std::vector<std::list<My_List_iter>>;

    //mutex is mutable to use in const functions.
    mutable std::shared_mutex myMutex;

    std::hash<Key> to_hash{};
    //buckets_count is the number of buckets in hash-table
    //it is initialized with 8 (it means the min number of buckets cannot be less than 8 even when the hash-table is empty)
    //Each bucket is consist of list<> max_bucket_size is the maximum size of that list that could be
    size_t         buckets_count    = 8;
    const size_t   max_bucket_size  = 10;
    //iter_list is used to keep inserted elements in order of insertion
    My_List iter_list;
    //variable buckets is a vector of lists(lists keeps iterators of iter_list)
    My_Bucket buckets;

    inline size_t getIndexOfBucket(const Key& key_) const noexcept{
        //buckets_count cannot be 0 so using % operator is safe
        return to_hash(key_) % buckets_count;
    }

    void re_storing();

public:
    //default constructor only creates the buckets
    //there is no need of using mutex in constructors because the object must be created before use from different threads
    my_unordered_map()
    {
        buckets.resize(buckets_count);
    }
    //constructing my_unordered_map with initializer_list
    my_unordered_map(const std::initializer_list<std::pair<Key, Value>>& init_)
    {
        //initializing buckets_count with 4/3 times more than initializer_list.size() is
        //to avoid from re_storing (but even in this case re_storing() can happen but less)
        buckets_count = static_cast<size_t>(init_.size() * 4 / 3);
        buckets.resize(buckets_count);
        //inserting items from initializer_list
        for (auto& i : init_)
            insert(i);
    }
    my_unordered_map<Key, Value>& operator=(const my_unordered_map<Key, Value>& right_) {
        //here is used exclusive lock because the members can be changed
        std::unique_lock<std::shared_mutex>(my_mutex);
        buckets_count   = right_.buckets_count;
        to_hash         = right_.to_hash;
        //creates a temporary My_Bucket obj and moves to buckets
        //it has better performance than clearing and resizing
        buckets = std::move(My_Bucket(buckets_count));
        iter_list.clear();
        //as we are sure that right_ is consist of unique keys and each bucket doesn't exceed max_bucket_size
        // so there is no need of checks
        for (const auto& i : right_.iter_list) {
            iter_list.push_back(i);
            buckets.at(getIndexOfBucket(i.first)).push_back(std::move( --iter_list.end()));
        }
        return *this;
    }
    my_unordered_map<Key, Value>& operator=(my_unordered_map<Key, Value>&& right_) {
        //here is used exclusive lock because the members can be changed
        std::unique_lock<std::shared_mutex>(my_mutex);
        //just move all parameters from given parameter
        buckets_count   = right_.buckets_count;
        to_hash         = std::move(right_.to_hash);
        iter_list       = std::move(right_.iter_list);
        buckets         = std::move(right_.buckets);
        return *this;
    }
    my_unordered_map(const my_unordered_map<Key, Value>& right_) {
        //using already defined copy assignment operator
        *this = right_;
    }
    my_unordered_map(my_unordered_map<Key, Value>&& right_) {
        //using already defined move assignment operator
        *this = std::move(right_);
    }

    std::pair<iterator,bool> insert(const std::pair<const Key, Value> &element_);
    std::pair<iterator,bool> insert( std::pair<const Key, Value> &&element_);

    iterator erase(const iterator& element_position_);

    iterator find(const Key &key_) noexcept;
    const_iterator find(const Key &key_) const noexcept;

    Value& operator[](const Key &key_)
    {
        //here isn't used lock because insert function is already synchronized
        return insert({ key_,Value() }).first->second;
    }
    //in the functions bellow are used shared_lock because only read happens
    inline iterator      begin() noexcept {
        std::shared_lock<std::shared_mutex> shared(myMutex);
        //the function is const and iterator isn't constructable with My_list::const_iterator
        //as iter_list is mutable  begin() returns iterator not const_iterator
        return iterator(iter_list.begin());
    }
    inline const_iterator begin() const noexcept {
        std::shared_lock<std::shared_mutex> shared(myMutex);
        return const_iterator(iter_list.begin());
    }
    inline iterator       end()  noexcept {
        std::shared_lock<std::shared_mutex> shared(myMutex);
        return iterator(iter_list.end());
    }
    inline const_iterator end() const noexcept {
        std::shared_lock<std::shared_mutex> shared(myMutex);
        return const_iterator(iter_list.end());
    }
    inline size_t   size()  const noexcept {
        std::shared_lock<std::shared_mutex>shared(myMutex);
        return iter_list.size();
    }
    inline size_t   bucket_count() const noexcept{
        std::shared_lock<std::shared_mutex>shared(myMutex);
        return buckets_count;
    }
};

template<class Key, class Value>
class my_unordered_map<Key,Value>::iterator {
    My_List_iter it;
public:
    friend class my_unordered_map<Key, Value>;
    iterator() = default;
    explicit iterator(My_List_iter &it_init):it(it_init) {}
    explicit iterator(My_List_iter &&it_init) :it(std::move(it_init)) {}
    //implementing postfix and prefix increment, decrement operators
    iterator operator++(int) {
        return iterator(it++);
    }
    iterator& operator++() {
        ++it;
        return *this;
    }
    iterator operator--(int) {
        return iterator(it--);
    }
    iterator& operator--() {
        --it;
        return *this;
    }

    //implementing == and != comparing operators
    inline bool operator==(const iterator& other_)   const noexcept {
        return it == other_.it;
    }
    inline bool operator!=(const iterator& other_)   const noexcept {
        return it != other_.it;
    }
    //the Key is const to prevent from changes by user
    inline std::pair<const Key, Value>& operator*()  const noexcept {
        return (*it);
    }
    inline std::pair<const Key, Value>* operator->() const noexcept {
        return &(*it);
    }
};
template<class Key, class Value>
class my_unordered_map<Key,Value>::const_iterator {
    My_List_constiter it;
public:
    friend class my_unordered_map<Key, Value>;
    const_iterator() = default;
    explicit const_iterator(My_List_constiter &it_init):it(it_init) {}
    explicit const_iterator(My_List_constiter &&it_init) :it(std::move(it_init)) {}
    //implementing postfix and prefix increment, decrement operators
    const_iterator operator++(int) {
        return const_iterator(it++);
    }
    const_iterator& operator++() {
        ++it;
        return *this;
    }
    const_iterator operator--(int) {
        return const_iterator(it--);
    }
    const_iterator& operator--() {
        --it;
        return *this;
    }

    //implementing == and != comparing operators
    inline bool operator==(const const_iterator& other_)   const noexcept {
        return it == other_.it;
    }
    inline bool operator!=(const const_iterator& other_)   const noexcept {
        return it != other_.it;
    }
    //the Key is const to prevent from changes by user
    inline const std::pair<const Key, Value>& operator*()  const noexcept {
        return (*it);
    }
    inline const std::pair<const Key, Value>* operator->() const noexcept {
        return &(*it);
    }
};

template<class Key, class Value>
void my_unordered_map<Key, Value>::re_storing()
{
    //it will exit from loop when hash_table will be created satisfying the given options
    //: each bucket will be consisted of no more than max_bucket_size
    //and 90% of buckets_count will be more than elements in hash-table
    while (true)
    {
        //increasing buckets_count with fibonacci const parameter 1.618
        buckets_count = static_cast<size_t>( buckets_count * 1.618);

        buckets = std::move(My_Bucket(buckets_count));
        //checker is for checking whether the hash-table satisfying the given options
        //if yes exits the loop.
        bool checker = true;
        for (auto i = iter_list.begin(); i != iter_list.end(); ++i) {
            auto& in_bucket = buckets[getIndexOfBucket(i->first)];
            in_bucket.push_back({ i });
            if (in_bucket.size() > max_bucket_size) {
                checker = false;
                break;
            }
        }
        if(checker)break;
    }
}

template<class Key, class Value>
std::pair<typename my_unordered_map<Key, Value>::iterator, bool>
my_unordered_map<Key, Value>::insert(const std::pair<const Key, Value> &element_)
{
    //find function takes shared_lock so myMutex takes exclusive lock after find function
    auto it_ = this->find(element_.first);
    //if the key already exists the function return pair of already existing iterator and false{the insertion doesn't take place}
    if (it_ != this->end())
        return std::pair<iterator, bool>(it_, false);
    //here is used exclusive lock because bellow the members can be changed
    std::unique_lock<std::shared_mutex> lock(myMutex);
    iter_list.push_back(element_);
    auto current_iterator = iter_list.end();
    //calling -- operator of current_iterator will not throw exception as in iter_list was pushed an element before call,
    //and it will give the added element's iterator
    --current_iterator;

    auto& in_bucket = buckets.at(getIndexOfBucket(element_.first));

    in_bucket.push_back({ current_iterator });
    //if in the bucket list exceeds the limit(max_bucket_size) or the elements in hash_table
    //are more than 90% of buckets quantity it causes re_storing
    if (in_bucket.size() > max_bucket_size || buckets_count * .9 < iter_list.size())
        re_storing();
    return std::pair<iterator, bool>(current_iterator,true);
}

template<class Key, class Value>
std::pair<typename my_unordered_map<Key, Value>::iterator, bool>
my_unordered_map<Key, Value>::insert(std::pair<const Key, Value>&& element_)
{

    auto it_ = this->find(element_.first);
    if (it_ != this->end())return std::pair<iterator, bool>( it_,false );

    std::unique_lock<std::shared_mutex> lock(myMutex);

    //here is used emplace_back to move the parameter and construct it in the end of list
    iter_list.emplace_back(std::move(element_));
    auto current_iterator = iter_list.end();
    --current_iterator;

    auto& in_bucket = buckets.at(getIndexOfBucket(current_iterator->first));

    in_bucket.push_back({ current_iterator });
    if (in_bucket.size() > max_bucket_size || buckets_count * .9 < iter_list.size())
        re_storing();
    return std::pair<iterator, bool>(current_iterator, true);
}

template<class Key, class Value>
typename my_unordered_map<Key, Value>::iterator my_unordered_map<Key, Value>::erase(const iterator &element_position_)
{
    std::unique_lock<std::shared_mutex> lock(myMutex);
    auto& in_bucket = buckets[getIndexOfBucket(element_position_->first)];
    for (auto i = in_bucket.begin(); i != in_bucket.end(); ++i)
        if ((*i)->first == element_position_->first) {
            in_bucket.erase(i);
            break;
        }
    //returns the value of the element following the removed one
    return iterator(static_cast<My_List_iter>(iter_list.erase(element_position_.it)));
}
template<class Key, class Value>
typename my_unordered_map<Key, Value>::iterator
    my_unordered_map<Key, Value>::find(const Key& key_) noexcept
{
    std::shared_lock<std::shared_mutex> shared(myMutex);
    //goes to the bucket where the key may be
    auto& find_in_bucket = buckets[getIndexOfBucket(key_)];
    //iterating in that bucket and checking the existence of the key.
    //As the max size of that bucket cannot exceed max_bucket_size the whole process will happen in const time.
    for (auto& i : find_in_bucket)
        if (i->first == key_)
            return iterator(static_cast<My_List_iter>(i));
    return end();
}
template<class Key, class Value>
typename my_unordered_map<Key, Value>::const_iterator
    my_unordered_map<Key, Value>::find(const Key& key_) const noexcept
{
    std::shared_lock<std::shared_mutex> shared(myMutex);
    //goes to the bucket where the key may be
    auto& find_in_bucket = buckets[getIndexOfBucket(key_)];
    //iterating in that bucket and checking the existence of the key.
    //As the max size of that bucket cannot exceed max_bucket_size the whole process will happen in const time.
    for (auto& i : find_in_bucket)
        if (i->first == key_)
            return iterator(static_cast<My_List_iter>(i));
    return end();
}


#endif // MY_UNORDERED_MAP



