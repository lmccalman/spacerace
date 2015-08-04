#include <vector>
#include <algorithm>
#include <iostream>

template <typename T>
std::vector<int> argsort(const std::vector<T> &a) 
{
    // map identity
    std::vector<int> ind(a.size());
    for (int i=0; i < ind.size(); i++)
        ind[i] = i;
    
    // sort i inplace using a comparitor looking at a
    std::sort(ind.begin(), ind.end(), 
            [&a](int i1, int i2) {return a[i1] < a[i2];}
            );
    return ind;
}
