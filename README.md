The whole implementation of my_unorderd_map class is done in the header file as it is a template class.
Some implementation explanation:
The iter_list is a member of my_unordered_map, it is a list which keeps inserted pairs{const key,value}, and the iterator of my_unordered_map is constructed with the iterators of that list, so it iterates in order of insertion.
As a Hash-table I use a vector of lists(lists keep iterators of the iter_list) named {buckets}, lists of vector cannot contain more than 10 elements(it is a custom given number). 
The insert function at first checks whether in a Hash-table is an iterator which points to the same key as in the given parameter, if there is, it will return a pair of that existing iterator and bool{false}, otherwise it will push_back the parameter to iter_list then using std::hash functor will hash the key, and according that hashed number will add the last iterator of iter_list to the buckets in right place. 
When there is a bucket which while insertion contains more than 10 elements, or inserted elements are more than 90% of buckets it will cause re_sorting.
As the elements in a bucket cannot exceed 10 and hash function works const time, the find function will also work in const time.
...
To make my_unorderd_map thread-safe I've used std::shared_mutex myMutex which while writing takes exclusive lock, while reading shared_lock.
To compare  my_unordered_map with std::unordered_map, I've used a file which contains pairs of key and value, inserted them into maps and used chrono library to measure elapsed time. 
To run the programm first download k_USA.txt file, copy the local dir and paste it in the main.cpp global var. file_path.
