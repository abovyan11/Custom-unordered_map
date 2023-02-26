#include<iostream>
#include<unordered_map>
#include<chrono>
#include<string>
#include<fstream>
#include"my_unordered_map.h"
using namespace std::literals;
//file_path is a path of a file in which are Key Value pairs
//this fill is downloaded from the internet
//to work properly give right local dir before compiling
const char* file_path = "C:/Users/User/Desktop/k_USA.txt";

template<typename Key,typename Value, typename Container>
void comparison(Container & param) {
    std::ifstream file;
    file.open(file_path);
    if (file.is_open()) {
        Key key{};
        Value val{};
        using namespace std::chrono;
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        while (file >> key >> val) {
            param.insert({ key,val });
        }
        param.erase(param.begin());
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        Container check_copy(param);
        high_resolution_clock::time_point t3 = high_resolution_clock::now();

        auto it = param.find("ahhzz@yahoo.com");

        high_resolution_clock::time_point t4 = high_resolution_clock::now();
        if(it!=param.end())
            std::cout<<"found: "<<it->second<<"\n";
        else
            std::cout<<"nothing is found.";
        std::cout << "elapsed time of filling: " << duration_cast<milliseconds>(t2 - t1).count() << "ms\n"
                  << "elapsed time of copying: " << duration_cast<milliseconds>(t3 - t2).count() << "ms\n"
                  << "elapsed time of finding: " << duration_cast<nanoseconds>(t4 - t3).count() << "ns\n"
                  << "elements in map: " << check_copy.size()<<"\nsizeof map: "<<sizeof(check_copy)<<"\nbuckets_count: "
                  << check_copy.bucket_count()<<std::endl;
    }
}


int main() {

    std::unordered_map<std::string, std::string> std_map;
    my_unordered_map<std::string, std::string> my_map;

    std::cout << "inserting data to std_map. Result: \n";
    comparison< std::string, std::string > (std_map);
    std::cout << "\ninserting data to my_map. Result: \n";
    comparison< std::string, std::string >(my_map);
    return 0;
}