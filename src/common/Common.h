#pragma once
#include <vector>

typedef std::vector<std::vector<bool>> vvb;
typedef std::vector<std::vector<int>>  vvi;
typedef std::vector<std::vector<float>>  vvf;

namespace Players
{
    enum {Self = 0, Enemy = 1};
}

struct TilePos
{
    int x;
    int y;
};