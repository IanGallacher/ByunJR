#pragma once
#include <vector>
#include <iostream>

typedef std::vector<std::vector<bool>> vvb;
typedef std::vector<std::vector<int>>  vvi;
typedef std::vector<std::vector<float>> vvf;

enum class PlayerArrayIndex {Self = 0, Enemy = 1, Neutral = 2, Error = -1};