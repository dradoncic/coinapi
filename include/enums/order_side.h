#pragma once

using Price = double;
using Volume = double;

struct Level {
    Price price;
    Volume size;
};

enum Side {
    BID,
    ASK
};
